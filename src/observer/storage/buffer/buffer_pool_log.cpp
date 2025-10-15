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
// Created by wangyunlai on 2022/02/01
//

#include "storage/buffer/buffer_pool_log.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_entry.h"

/**
 * @brief 将BufferPoolLogEntry结构体转换为字符串表示
 * @return 日志条目的字符串表示，包含buffer_pool_id、page_num和operation_type
 */
string BufferPoolLogEntry::to_string() const
{
  return string("buffer_pool_id=") + std::to_string(buffer_pool_id) +
         ", page_num=" + std::to_string(page_num) +
         ", operation_type=" + BufferPoolOperation(operation_type).to_string();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief BufferPoolLogHandler构造函数
 * @param buffer_pool 关联的磁盘缓冲池
 * @param log_handler 日志处理器
 */
BufferPoolLogHandler::BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler)
    : buffer_pool_(buffer_pool), log_handler_(log_handler)
{}

/**
 * @brief 记录分配页面的日志
 * @param page_num 分配的页面号
 * @param[out] lsn 分配页面的日志序列号，输出参数
 * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
 */
RC BufferPoolLogHandler::allocate_page(PageNum page_num, LSN &lsn)
{
  // 调用append_log方法添加分配页面的日志
  return append_log(BufferPoolOperation::Type::ALLOCATE, page_num, lsn);
}

/**
 * @brief 记录释放页面的日志
 * @param page_num 释放的页面编号
 * @param[out] lsn 释放页面的日志序列号，输出参数
 * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
 */
RC BufferPoolLogHandler::deallocate_page(PageNum page_num, LSN &lsn)
{
  // 调用append_log方法添加释放页面的日志
  return append_log(BufferPoolOperation::Type::DEALLOCATE, page_num, lsn);
}

/**
 * @brief 确保页面的日志已刷新到磁盘
 * @param page 需要刷新的页面
 * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
 */
RC BufferPoolLogHandler::flush_page(Page &page)
{
  // 等待日志系统刷新到页面对应的LSN
  return log_handler_.wait_lsn(page.lsn);
}

/**
 * @brief 向日志系统追加一条BufferPool操作日志
 * @param type 操作类型（ALLOCATE或DEALLOCATE）
 * @param page_num 页面编号
 * @param[out] lsn 生成的日志序列号，输出参数
 * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
 */
RC BufferPoolLogHandler::append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn)
{
  // 创建日志条目并填充信息
  BufferPoolLogEntry log;
  log.buffer_pool_id = buffer_pool_.id();  // 设置缓冲池ID
  log.page_num = page_num;  // 设置页面编号
  log.operation_type = BufferPoolOperation(type).type_id();  // 设置操作类型

  // 调用日志处理器的append方法写入日志
  return log_handler_.append(lsn, LogModule::Id::BUFFER_POOL, span<const char>(reinterpret_cast<const char *>(&log), sizeof(log)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BufferPoolLogReplayer

/**
 * @brief BufferPoolLogReplayer构造函数
 * @param bp_manager 缓冲池管理器
 */
BufferPoolLogReplayer::BufferPoolLogReplayer(BufferPoolManager &bp_manager) : bp_manager_(bp_manager)
{}

/**
 * @brief 重放一条BufferPool操作日志
 * @param entry 要重放的日志条目
 * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
 */
RC BufferPoolLogReplayer::replay(const LogEntry &entry)
{
  // 检查日志条目大小是否正确
  if (entry.payload_size() != sizeof(BufferPoolLogEntry)) {
    LOG_ERROR("invalid buffer pool log entry. payload size=%d, expected=%d, entry=%s",
              entry.payload_size(), sizeof(BufferPoolLogEntry), entry.to_string().c_str());
    return RC::INVALID_ARGUMENT;
  }

  // 将日志数据转换为BufferPoolLogEntry结构
  auto log = reinterpret_cast<const BufferPoolLogEntry *>(entry.data());

  LOG_TRACE("replay buffer pool log. entry=%s, log=%s", entry.to_string().c_str(), log->to_string().c_str());
  
  // 获取日志中指定的缓冲池
  int32_t buffer_pool_id = log->buffer_pool_id;
  DiskBufferPool *buffer_pool = nullptr;
  RC rc = bp_manager_.get_buffer_pool(buffer_pool_id, buffer_pool);
  if (OB_FAIL(rc) || buffer_pool == nullptr) {
    LOG_ERROR("failed to get buffer pool. rc=%s, buffer pool=%p, log=%s, %s", 
              strrc(rc), buffer_pool, entry.to_string().c_str(), log->to_string().c_str());
    return rc;
  }

  // 根据操作类型执行不同的重放操作
  BufferPoolOperation operation(log->operation_type);
  switch (operation.type())
  {
    case BufferPoolOperation::Type::ALLOCATE:
      // 重放页面分配操作
      return buffer_pool->redo_allocate_page(entry.lsn(), log->page_num);
    case BufferPoolOperation::Type::DEALLOCATE:
      // 重放页面释放操作
      return buffer_pool->redo_deallocate_page(entry.lsn(), log->page_num);
    default:
      LOG_ERROR("unknown buffer pool operation. operation=%s", operation.to_string().c_str());
      return RC::INTERNAL;
  }
  return RC::SUCCESS;
}
