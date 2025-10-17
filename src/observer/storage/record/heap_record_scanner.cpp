/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/**
 * @file heap_record_scanner.cpp
 * @brief 堆文件记录扫描器实现
 * @details 实现了HeapRecordScanner类的所有成员函数，提供对堆文件中记录的顺序扫描功能
 */

#include "storage/record/heap_record_scanner.h"

////////////////////////////////////////////////////////////////////////////////


/**
 * @brief 打开记录扫描器的具体实现
 * @details 初始化缓冲池迭代器，从页面1开始（页面0通常是元数据页），
 * 根据存储格式创建相应的记录页面处理器（行存储或PAX存储）
 * @return RC 操作结果状态码，成功返回SUCCESS，失败返回错误码
 */
RC HeapRecordScanner::open_scan()
{
  // 确保磁盘缓冲池和日志处理器不为空
  ASSERT(disk_buffer_pool_ != nullptr, "disk buffer pool is null");
  ASSERT(log_handler_ != nullptr, "log handler is null");

  // 初始化缓冲池迭代器，从页面1开始（跳过元数据页）
  RC rc = bp_iterator_.init(*disk_buffer_pool_, 1);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to init bp iterator. rc=%d:%s", rc, strrc(rc));
    return rc;
  }
  
  // 根据存储格式创建相应的记录页面处理器
  if (table_ == nullptr || table_->table_meta().storage_format() == StorageFormat::ROW_FORMAT) {
    record_page_handler_ = new RowRecordPageHandler();  // 行存储格式
  } else {
    record_page_handler_ = new PaxRecordPageHandler();  // PAX列存储格式
  }

  return rc;
}

/**
 * @brief 从文件中获取下一条满足条件的记录
 * @details 首先尝试在当前页面中查找有效记录，如果当前页面已遍历完，则遍历下一个页面
 * 直到找到满足条件的记录或遍历完所有页面
 * @return RC 操作结果状态码，成功返回SUCCESS，没有更多记录返回RECORD_EOF
 */
RC HeapRecordScanner::fetch_next_record()
{
  RC rc = RC::SUCCESS;
  
  // 如果当前页面迭代器有效，先尝试在当前页面中查找下一条记录
  if (record_page_iterator_.is_valid()) {
    // 当前页面还是有效的，尝试看一下是否有有效记录
    rc = fetch_next_record_in_page();
    if (rc == RC::SUCCESS || rc != RC::RECORD_EOF) {
      // 有有效记录：RC::SUCCESS
      // 或者出现了错误，rc != (RC::SUCCESS or RC::RECORD_EOF)
      // RECORD_EOF 表示当前页面已经遍历完了
      return rc;
    }
  }

  // 上个页面遍历完了，或者还没有开始遍历某个页面，那么就从一个新的页面开始遍历查找
  while (bp_iterator_.has_next()) {
    PageNum page_num = bp_iterator_.next();
    
    // 清理并初始化当前页面的记录页面处理器
    record_page_handler_->cleanup();
    rc = record_page_handler_->init(*disk_buffer_pool_, *log_handler_, page_num, rw_mode_);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to init record page handler. page_num=%d, rc=%s", page_num, strrc(rc));
      return rc;
    }

    // 初始化页面迭代器并在该页面中查找记录
    record_page_iterator_.init(record_page_handler_);
    rc = fetch_next_record_in_page();
    if (rc == RC::SUCCESS || rc != RC::RECORD_EOF) {
      // 有有效记录：RC::SUCCESS
      // 或者出现了错误，rc != (RC::SUCCESS or RC::RECORD_EOF)
      // RECORD_EOF 表示当前页面已经遍历完了
      return rc;
    }
  }

  // 所有的页面都遍历完了，没有数据了
  next_record_.rid().slot_num = -1;
  record_page_handler_->cleanup();
  return RC::RECORD_EOF;
}

/**
 * @brief 在当前页面内获取下一条满足条件的记录
 * @details 遍历当前页面中的记录，应用条件过滤和事务访问控制，找到第一条符合条件的记录
 * @return RC 操作结果状态码，成功返回SUCCESS，页面内没有更多记录返回RECORD_EOF
 */
RC HeapRecordScanner::fetch_next_record_in_page()
{
  RC rc = RC::SUCCESS;
  
  // 遍历当前页面中的所有记录
  while (record_page_iterator_.has_next()) {
    // 获取下一条记录
    rc = record_page_iterator_.next(next_record_);
    if (rc != RC::SUCCESS) {
      const auto page_num = record_page_handler_->get_page_num();
      LOG_TRACE("failed to get next record from page. page_num=%d, rc=%s", page_num, strrc(rc));
      return rc;
    }

    // 如果有过滤条件，应用过滤条件
    if (condition_filter_ != nullptr && !condition_filter_->filter(next_record_)) {
      continue;  // 记录不满足条件，继续下一条
    }

    // 如果没有事务上下文，直接返回记录
    if (trx_ == nullptr) {
      return rc;
    }

    // 让当前事务检查记录访问权限，可能涉及加锁、冲突检测等
    // TODO 把判断事务有效性的逻辑从Scanner中移除
    rc = trx_->visit_record(table_, next_record_, rw_mode_);
    if (rc == RC::RECORD_INVISIBLE) {
      // 记录对当前事务不可见（如在MVCC模式下），继续下一条
      // 这种模式仅在readonly事务下是有效的
      continue;
    }
    return rc;  // 返回事务检查结果
  }

  // 页面内没有更多记录
  next_record_.rid().slot_num = -1;
  return RC::RECORD_EOF;
}

/**
 * @brief 关闭记录扫描器的具体实现
 * @details 释放所有资源，包括记录页面处理器、重置各种指针
 * @return RC 操作结果状态码，成功返回SUCCESS
 */
RC HeapRecordScanner::close_scan()
{
  // 重置磁盘缓冲池指针
  if (disk_buffer_pool_ != nullptr) {
    disk_buffer_pool_ = nullptr;
  }

  // 重置条件过滤器指针
  if (condition_filter_ != nullptr) {
    condition_filter_ = nullptr;
  }
  
  // 清理并释放记录页面处理器
  if (record_page_handler_ != nullptr) {
    record_page_handler_->cleanup();
    delete record_page_handler_;
    record_page_handler_ = nullptr;
  }

  return RC::SUCCESS;
}

/**
 * @brief 获取下一条记录的具体实现
 * @details 调用fetch_next_record获取下一条满足条件的记录，并将其复制到输出参数中
 * @param[out] record 输出参数，存储获取到的下一条记录
 * @return RC 操作结果状态码，成功返回SUCCESS，没有更多记录返回RECORD_EOF
 */
RC HeapRecordScanner::next(Record &record)
{
  // 获取下一条满足条件的记录
  RC rc = fetch_next_record();
  if (OB_FAIL(rc)) {
    return rc;  // 如果失败或没有更多记录，直接返回
  }

  // 将缓存的记录复制到输出参数
  record = next_record_;
  return RC::SUCCESS;
}
