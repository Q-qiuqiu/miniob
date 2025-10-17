/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/record/record_scanner.h" ///< 包含记录扫描器基类定义
#include "storage/buffer/disk_buffer_pool.h" ///< 包含磁盘缓冲池定义
#include "storage/trx/trx.h" ///< 包含事务定义

/**
 * @brief 堆文件记录扫描器实现类
 * @ingroup RecordManager
 * @details 继承自RecordScanner，实现了对堆文件中所有记录的顺序扫描功能，
 * 遍历所有页面并访问这些页面中满足条件的记录。支持事务隔离和记录过滤。
 */
class HeapRecordScanner : public RecordScanner
{
public:
  /**
   * @brief 构造函数
   * @param table 表指针，用于事务访问控制
   * @param buffer_pool 磁盘缓冲池引用，用于访问文件页面
   * @param trx 事务指针，用于记录访问的事务控制
   * @param log_handler 日志处理器引用，用于记录操作日志
   * @param mode 读写模式，表示对记录的访问权限
   * @param condition_filter 条件过滤器，用于过滤满足条件的记录
   */
  HeapRecordScanner(Table *table, DiskBufferPool &buffer_pool, Trx *trx, LogHandler &log_handler, ReadWriteMode mode,
      ConditionFilter *condition_filter)
      : table_(table),
        disk_buffer_pool_(&buffer_pool),
        trx_(trx),
        log_handler_(&log_handler),
        rw_mode_(mode),
        condition_filter_(condition_filter)
  {}
  /**
   * @brief 析构函数
   * @details 析构时自动关闭扫描器，释放相关资源
   */
  ~HeapRecordScanner() override { close_scan(); }

  /**
   * @brief 打开记录扫描器
   * @details 初始化页面迭代器和记录页面处理器，准备开始扫描
   * @return RC 操作结果状态码，成功返回SUCCESS，失败返回错误码
   */
  RC open_scan() override;

  /**
   * @brief 关闭记录扫描器
   * @details 释放所有资源，包括记录页面处理器和各种指针
   * @return RC 操作结果状态码，成功返回SUCCESS
   */
  RC close_scan() override;

  /**
   * @brief 获取下一条满足条件的记录
   * @param[out] record 输出参数，存储获取到的下一条记录
   * @return RC 操作结果状态码，成功返回SUCCESS，没有更多记录返回RECORD_EOF
   */
  RC next(Record &record) override;

private:
  /**
   * @brief 从文件中获取下一条满足条件的记录
   * @details 首先尝试在当前页面中查找，如果当前页面已遍历完，则遍历下一个页面
   * @return RC 操作结果状态码，成功返回SUCCESS，没有更多记录返回RECORD_EOF
   */
  RC fetch_next_record();

  /**
   * @brief 在当前页面内获取下一条满足条件的记录
   * @details 遍历当前页面中的记录，应用条件过滤和事务访问控制
   * @return RC 操作结果状态码，成功返回SUCCESS，页面内没有更多记录返回RECORD_EOF
   */
  RC fetch_next_record_in_page();

private:
  // TODO 对于一个纯粹的record遍历器来说，不应该关心表和事务
  Table *table_ = nullptr;  ///< 当前遍历的表指针，仅供事务函数使用

  DiskBufferPool *disk_buffer_pool_ = nullptr;  ///< 当前访问的磁盘缓冲池指针
  Trx            *trx_              = nullptr;  ///< 当前执行遍历的事务指针
  LogHandler     *log_handler_      = nullptr;  ///< 日志处理器指针
  ReadWriteMode   rw_mode_ = ReadWriteMode::READ_WRITE;  ///< 记录访问模式，表示是否可修改

  BufferPoolIterator bp_iterator_;                    ///< 缓冲池页面迭代器，用于遍历所有页面
  ConditionFilter   *condition_filter_    = nullptr;  ///< 记录过滤器，用于筛选满足条件的记录
  RecordPageHandler *record_page_handler_ = nullptr;  ///< 记录页面处理器，用于处理页面上的记录操作
  RecordPageIterator record_page_iterator_;           ///< 记录页面迭代器，用于遍历页面内所有记录
  Record             next_record_;                    ///< 缓存的下一条记录
};