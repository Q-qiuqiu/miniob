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
// Created by Wangyunlai on 2024/02/02.
//

#pragma once

#include <stdint.h>

#include "common/types.h"          ///< 包含通用类型定义
#include "common/sys/rc.h"         ///< 包含返回码定义
#include "common/lang/span.h"      ///< 包含span工具类，用于处理内存区间
#include "common/lang/string.h"    ///< 包含字符串工具类
#include "storage/clog/log_replayer.h" ///< 包含日志重放器基类
#include "sql/parser/parse_defs.h" ///< 包含解析器定义的常量和结构

struct RID;                       ///< 记录标识符（Record ID）的前向声明
class LogHandler;                 ///< 日志处理器的前向声明
class Frame;                      ///< 页帧的前向声明
class BufferPoolManager;          ///< 缓冲池管理器的前向声明
class DiskBufferPool;             ///< 磁盘缓冲池的前向声明

/**
 * @brief 记录管理器操作相关的日志类型枚举类
 * @details 定义了记录管理器支持的各种操作类型，用于日志记录和重放
 * @ingroup CLog
 * @note 这些操作类型标识了日志中记录的不同记录管理操作
 */
class RecordOperation
{
public:
  /**
   * @brief 操作类型枚举
   * @details 定义了记录管理器支持的所有操作类型
   */
  enum class Type : int32_t
  {
    INIT_PAGE,  ///< 初始化空页面操作
    INSERT,     ///< 插入一条记录操作
    DELETE,     ///< 删除一条记录操作
    UPDATE      ///< 更新一条记录操作
  };

public:
  /**
   * @brief 构造函数，使用Type枚举值初始化
   * @param type 操作类型枚举值
   */
  explicit RecordOperation(Type type) : type_(type) {}
  
  /**
   * @brief 构造函数，使用整数值初始化
   * @param type 操作类型的整数值
   */
  explicit RecordOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  
  /**
   * @brief 析构函数
   */
  ~RecordOperation() = default;

  /**
   * @brief 获取操作类型枚举值
   * @return Type 操作类型枚举值
   */
  Type    type() const { return type_; }
  
  /**
   * @brief 获取操作类型的整数值
   * @return int32_t 操作类型的整数值
   */
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  /**
   * @brief 将操作类型转换为字符串表示
   * @return string 操作类型的字符串表示
   */
  string to_string() const;

private:
  Type type_;  ///< 操作类型枚举值
};

/**
 * @brief 记录日志头部结构
 * @details 存储记录操作日志的元数据信息，包括缓冲池ID、操作类型、页面号等
 */
struct RecordLogHeader
{
  int32_t buffer_pool_id;    ///< 缓冲池ID，标识操作所属的缓冲池
  int32_t operation_type;    ///< 操作类型，对应RecordOperation::Type的整数值
  PageNum page_num;          ///< 页面编号，标识操作发生的页面
  int32_t storage_format;    ///< 存储格式，对应StorageFormat枚举的整数值
  int32_t column_num;        ///< 列数量，用于某些操作类型
  union                      ///< 联合体，根据操作类型存储不同信息
  {
    SlotNum slot_num;        ///< 槽位编号，用于INSERT、DELETE、UPDATE操作
    int32_t record_size;     ///< 记录大小，用于INIT_PAGE操作
  };

  char data[0];              ///< 柔性数组，指向额外的数据部分

  /**
   * @brief 将日志头部信息转换为字符串表示
   * @return string 日志头部的字符串表示
   */
  string to_string() const;

  static const int32_t SIZE;  ///< 日志头部大小
};

/**
 * @brief 记录日志处理器类
 * @details 负责生成和记录与记录管理相关的日志，支持初始化页面、插入、删除和更新记录的日志记录
 */
class RecordLogHandler final
{
public:
  /**
   * @brief 默认构造函数
   */
  RecordLogHandler()  = default;
  
  /**
   * @brief 析构函数
   */
  ~RecordLogHandler() = default;

  /**
   * @brief 初始化记录日志处理器
   * @param log_handler 底层日志处理器引用
   * @param buffer_pool_id 缓冲池ID
   * @param record_size 记录大小
   * @param storage_format 存储格式
   * @return RC 操作结果状态码
   */
  RC init(LogHandler &log_handler, int32_t buffer_pool_id, int32_t record_size, StorageFormat storage_format);

  /**
   * @brief 初始化一个新的页面并记录日志
   * @details 记录一个初始化新页面的日志，用于恢复时重建页面结构
   * @note 这条日志通常伴随着一个buffer pool中创建页面的日志，这时候其实存在一个问题：
   * 通常情况下日志是这样的：
   * 1. buffer pool.allocate page
   * 2. record_log_handler.init_new_page
   * 如果第一条日志记录下来了，但是第二条日志没有记录下来，就会出现问题。就丢失了一个页面，
   * 或者页面在访问时会出现异常。
   * @param[out] frame 页帧指针，用于设置日志序列号
   * @param page_num 页面编号
   * @param data 页面数据，目前主要是 `column index`
   * @return RC 操作结果状态码
   */
  RC init_new_page(Frame *frame, PageNum page_num, span<const char> data);

  /**
   * @brief 插入一条记录并记录日志
   * @details 记录插入记录的操作，用于恢复时重新插入该记录
   * @param[out] frame 页帧指针，用于设置日志序列号
   * @param rid 记录的位置标识符
   * @param record 记录的内容
   * @return RC 操作结果状态码
   */
  RC insert_record(Frame *frame, const RID &rid, const char *record);

  /**
   * @brief 删除一条记录并记录日志
   * @details 记录删除记录的操作，用于恢复时重新执行删除
   * @param[out] frame 页帧指针，用于设置日志序列号
   * @param rid 记录的位置标识符
   * @return RC 操作结果状态码
   */
  RC delete_record(Frame *frame, const RID &rid);

  /**
   * @brief 更新一条记录并记录日志
   * @details 记录更新记录的操作，用于恢复时重新执行更新
   * @param[out] frame 页帧指针，用于设置日志序列号
   * @param rid 记录的位置标识符
   * @param record 更新后的记录内容。不需要做回滚，所以不用记录原先的数据
   * @note 更新数据时，通常只更新其中几个字段，这里记录所有数据，是可以优化的。
   * @return RC 操作结果状态码
   */
  RC update_record(Frame *frame, const RID &rid, const char *record);

private:
  LogHandler   *log_handler_    = nullptr;   ///< 底层日志处理器指针
  int32_t       buffer_pool_id_ = -1;        ///< 缓冲池ID
  int32_t       record_size_    = -1;        ///< 记录大小
  StorageFormat storage_format_ = StorageFormat::ROW_FORMAT; ///< 存储格式
};

/**
 * @brief 记录相关的日志重放器
 * @details 负责重放与记录管理相关的日志，支持恢复初始化页面、插入、删除和更新记录的操作
 * @ingroup CLog
 */
class RecordLogReplayer final : public LogReplayer
{
public:
  /**
   * @brief 构造函数
   * @param bpm 缓冲池管理器引用
   */
  RecordLogReplayer(BufferPoolManager &bpm);
  
  /**
   * @brief 析构函数
   */
  virtual ~RecordLogReplayer() = default;

  /**
   * @brief 重放单条日志条目
   * @param entry 日志条目引用
   * @return RC 操作结果状态码
   */
  virtual RC replay(const LogEntry &entry) override;

private:
  /**
   * @brief 重放初始化页面日志
   * @param buffer_pool 磁盘缓冲池引用
   * @param log_header 记录日志头部引用
   * @return RC 操作结果状态码
   */
  RC replay_init_page(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  
  /**
   * @brief 重放插入记录日志
   * @param buffer_pool 磁盘缓冲池引用
   * @param log_header 记录日志头部引用
   * @return RC 操作结果状态码
   */
  RC replay_insert(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  
  /**
   * @brief 重放删除记录日志
   * @param buffer_pool 磁盘缓冲池引用
   * @param log_header 记录日志头部引用
   * @return RC 操作结果状态码
   */
  RC replay_delete(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);
  
  /**
   * @brief 重放更新记录日志
   * @param buffer_pool 磁盘缓冲池引用
   * @param log_header 记录日志头部引用
   * @return RC 操作结果状态码
   */
  RC replay_update(DiskBufferPool &buffer_pool, const RecordLogHeader &log_header);

private:
  BufferPoolManager &bpm_;  ///< 缓冲池管理器引用
};
