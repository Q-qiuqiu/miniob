/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Wangyunlai on 2023/03/08.
//

#include "storage/index/latch_memo.h"
#include "common/lang/mutex.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/buffer/frame.h"

/**
 * @brief LatchMemoItem构造函数实现，针对页面操作
 * @param type 操作类型
 * @param frame 页面帧指针
 */
LatchMemoItem::LatchMemoItem(LatchMemoType type, Frame *frame)
{
  this->type  = type;
  this->frame = frame;
}

/**
 * @brief LatchMemoItem构造函数实现，针对共享锁操作
 * @param type 操作类型
 * @param lock 共享锁指针
 */
LatchMemoItem::LatchMemoItem(LatchMemoType type, common::SharedMutex *lock)
{
  this->type = type;
  this->lock = lock;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief LatchMemo构造函数实现
 * @param buffer_pool 磁盘缓冲区池指针
 */
LatchMemo::LatchMemo(DiskBufferPool *buffer_pool) : buffer_pool_(buffer_pool) {}

/**
 * @brief LatchMemo析构函数实现
 * @details 自动释放所有持有的资源
 */
LatchMemo::~LatchMemo() { this->release(); }

/**
 * @brief 获取指定页码的页面实现
 * @param page_num 页码
 * @param frame 输出参数，用于存储获取到的页面帧
 * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
 */
RC LatchMemo::get_page(PageNum page_num, Frame *&frame)
{
  frame = nullptr;

  RC rc = buffer_pool_->get_this_page(page_num, &frame);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  items_.emplace_back(LatchMemoType::PIN, frame);
  return RC::SUCCESS;
}

/**
 * @brief 分配新页面实现
 * @param frame 输出参数，用于存储分配的页面帧
 * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
 */
RC LatchMemo::allocate_page(Frame *&frame)
{
  frame = nullptr;

  RC rc = buffer_pool_->allocate_page(&frame);
  if (rc == RC::SUCCESS) {
    items_.emplace_back(LatchMemoType::PIN, frame);
    ASSERT(frame->pin_count() == 1, "allocate a new frame. frame=%s", frame->to_string().c_str());
  }

  return rc;
}

/**
 * @brief 标记为即将释放的页面实现
 * @param page_num 要释放的页码
 */
void LatchMemo::dispose_page(PageNum page_num) { disposed_pages_.emplace_back(page_num); }

/**
 * @brief 对指定页面加锁实现
 * @param frame 页面帧
 * @param type 锁类型
 */
void LatchMemo::latch(Frame *frame, LatchMemoType type)
{
  switch (type) {
    case LatchMemoType::EXCLUSIVE: {
      frame->write_latch();
    } break;
    case LatchMemoType::SHARED: {
      frame->read_latch();
    } break;
    default: {
      ASSERT(false, "invalid latch type: %d", static_cast<int>(type));
    }
  }

  items_.emplace_back(type, frame);
}

/**
 * @brief 对指定页面加独占锁（写锁）实现
 * @param frame 页面帧
 */
void LatchMemo::xlatch(Frame *frame) { this->latch(frame, LatchMemoType::EXCLUSIVE); }

/**
 * @brief 对指定页面加共享锁（读锁）实现
 * @param frame 页面帧
 */
void LatchMemo::slatch(Frame *frame) { this->latch(frame, LatchMemoType::SHARED); }

/**
 * @brief 尝试对指定页面加共享锁（读锁）实现
 * @param frame 页面帧
 * @return 是否成功获取锁
 */
bool LatchMemo::try_slatch(Frame *frame)
{
  bool ret = frame->try_read_latch();
  if (ret) {
    items_.emplace_back(LatchMemoType::SHARED, frame);
  }
  return ret;
}

/**
 * @brief 对指定共享锁加独占锁实现
 * @param lock 共享锁
 */
void LatchMemo::xlatch(common::SharedMutex *lock)
{
  lock->lock();
  items_.emplace_back(LatchMemoType::EXCLUSIVE, lock);
  LOG_DEBUG("lock root success");
}

/**
 * @brief 对指定共享锁加共享锁实现
 * @param lock 共享锁
 */
void LatchMemo::slatch(common::SharedMutex *lock)
{
  lock->lock_shared();
  items_.emplace_back(LatchMemoType::SHARED, lock);
}

/**
 * @brief 释放单个备忘录条目的实现
 * @param item 备忘录条目
 */
void LatchMemo::release_item(LatchMemoItem &item)
{
  switch (item.type) {
    case LatchMemoType::EXCLUSIVE: {
      if (item.frame != nullptr) {
        item.frame->write_unlatch();
      } else {
        LOG_DEBUG("release root lock");
        item.lock->unlock();
      }
    } break;
    case LatchMemoType::SHARED: {
      if (item.frame != nullptr) {
        item.frame->read_unlatch();
      } else {
        item.lock->unlock_shared();
      }
    } break;
    case LatchMemoType::PIN: {
      buffer_pool_->unpin_page(item.frame);
    } break;

    default: {
      ASSERT(false, "invalid latch type: %d", static_cast<int>(item.type));
    }
  }
}

/**
 * @brief 释放所有持有的资源实现
 */
void LatchMemo::release()
{
  int point = static_cast<int>(items_.size());
  release_to(point);

  for (PageNum page_num : disposed_pages_) {
    buffer_pool_->dispose_page(page_num);
  }
  disposed_pages_.clear();
}

/**
 * @brief 释放到指定点之前的所有资源实现
 * @param point 释放点
 */
void LatchMemo::release_to(int point)
{
  ASSERT(point >= 0 && point <= static_cast<int>(items_.size()), 
         "invalid memo point. point=%d, items size=%d",
         point, static_cast<int>(items_.size()));

  auto iter = items_.begin();
  for (int i = point - 1; i >= 0; i--, ++iter) {
    LatchMemoItem &item = items_[i];
    release_item(item);
  }
  items_.erase(items_.begin(), iter);
}
