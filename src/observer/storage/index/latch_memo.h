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

#pragma once

#include "common/sys/rc.h"
#include "common/lang/deque.h"
#include "common/lang/vector.h"
#include "storage/buffer/page.h"

/**
 * @brief 前向声明：页面帧类
 */
class Frame;

/**
 * @brief 前向声明：磁盘缓冲区池类
 */
class DiskBufferPool;

/**
 * @brief 命名空间：通用工具类
 */
namespace common {
/**
 * @brief 共享互斥锁类
 */
class SharedMutex;
}

/**
 * @brief 对指定页面做的操作类型枚举
 * @details 定义了页面锁和引用计数相关的操作类型
 */
enum class LatchMemoType
{
  NONE,       ///< 什么都不做
  SHARED,     ///< 共享锁（读锁）
  EXCLUSIVE,  ///< 独占锁（写锁）
  PIN,        ///< pin住页面，增加引用计数
};

/**
 * @brief 锁备忘录条目结构体
 * @details 记录对页面或锁的操作信息
 */
struct LatchMemoItem
{
  /**
   * @brief 默认构造函数
   */
  LatchMemoItem() = default;
  
  /**
   * @brief 构造函数，针对页面操作
   * @param type 操作类型
   * @param frame 页面帧指针
   */
  LatchMemoItem(LatchMemoType type, Frame *frame);
  
  /**
   * @brief 构造函数，针对共享锁操作
   * @param type 操作类型
   * @param lock 共享锁指针
   */
  LatchMemoItem(LatchMemoType type, common::SharedMutex *lock);

  LatchMemoType        type  = LatchMemoType::NONE;  ///< 操作类型
  Frame               *frame = nullptr;              ///< 页面帧指针
  common::SharedMutex *lock  = nullptr;              ///< 共享锁指针
};

/**
 * @brief 锁备忘录类
 * @details 管理对页面的锁定和引用计数操作，支持自动释放资源，确保资源管理的一致性
 * @note 此类用于在索引操作过程中，跟踪和管理获取的各种锁和页面引用，便于统一释放
 */
class LatchMemo final
{
public:
  /**
   * @brief 构造函数
   * @details 当前遇到的场景都是针对单个BufferPool的，不过从概念上讲，不一定做这个限制
   * @param buffer_pool 磁盘缓冲区池指针
   */
  LatchMemo(DiskBufferPool *buffer_pool);
  
  /**
   * @brief 析构函数
   * @details 自动释放所有持有的资源
   */
  ~LatchMemo();

  /**
   * @brief 获取指定页码的页面
   * @param page_num 页码
   * @param frame 输出参数，用于存储获取到的页面帧
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC get_page(PageNum page_num, Frame *&frame);

  /**
   * @brief 分配新页面
   * @param frame 输出参数，用于存储分配的页面帧
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC allocate_page(Frame *&frame);

  /**
   * @brief 标记为即将释放的页面
   * @param page_num 要释放的页码
   */
  void dispose_page(PageNum page_num);

  /**
   * @brief 对指定页面加锁
   * @param frame 页面帧
   * @param type 锁类型
   */
  void latch(Frame *frame, LatchMemoType type);
  
  /**
   * @brief 对指定页面加独占锁（写锁）
   * @param frame 页面帧
   */
  void xlatch(Frame *frame);
  
  /**
   * @brief 对指定页面加共享锁（读锁）
   * @param frame 页面帧
   */
  void slatch(Frame *frame);
  
  /**
   * @brief 尝试对指定页面加共享锁（读锁）
   * @param frame 页面帧
   * @return 是否成功获取锁
   */
  bool try_slatch(Frame *frame);

  /**
   * @brief 对指定共享锁加独占锁
   * @param lock 共享锁
   */
  void xlatch(common::SharedMutex *lock);
  
  /**
   * @brief 对指定共享锁加共享锁
   * @param lock 共享锁
   */
  void slatch(common::SharedMutex *lock);

  /**
   * @brief 释放所有持有的资源
   * @details 包括所有的锁和页面引用，以及等待释放的页面
   */
  void release();

  /**
   * @brief 释放到指定点之前的所有资源
   * @param point 释放点
   */
  void release_to(int point);

  /**
   * @brief 获取当前备忘录的记录点
   * @return 备忘录记录点
   */
  int memo_point() const { return static_cast<int>(items_.size()); }

private:
  /**
   * @brief 释放单个备忘录条目
   * @param item 备忘录条目
   */
  void release_item(LatchMemoItem &item);

private:
  DiskBufferPool      *buffer_pool_ = nullptr;      ///< 磁盘缓冲区池指针，用于管理页面
  deque<LatchMemoItem> items_;                      ///< 备忘录条目列表，记录所有操作
  vector<PageNum>      disposed_pages_;             ///< 等待释放的页面列表
};
