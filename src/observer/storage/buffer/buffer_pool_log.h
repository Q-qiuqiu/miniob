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

#pragma once

#include "common/lang/string.h"
#include "common/types.h"
#include "common/sys/rc.h"
#include "storage/clog/log_replayer.h"

class DiskBufferPool;
class BufferPoolManager;
class LogHandler;
struct Page;

/**
 * @brief BufferPool 的日志相关操作类型
 * @ingroup CLog
 * @details 定义了BufferPool支持的日志操作类型，用于记录和重放页面分配和释放操作
 */
class BufferPoolOperation
{
public:
  /**
   * @brief 操作类型枚举
   */
  enum class Type : int32_t
  {
    ALLOCATE,   ///< 分配页面
    DEALLOCATE  ///< 释放页面
  };

public:
  /**
   * @brief 构造函数，使用操作类型枚举初始化
   * @param type 操作类型
   */
  BufferPoolOperation(Type type) : type_(type) {}
  
  /**
   * @brief 构造函数，使用整数值初始化操作类型
   * @param type 操作类型的整数值表示
   */
  explicit BufferPoolOperation(int32_t type) : type_(static_cast<Type>(type)) {}
  
  /**
   * @brief 析构函数
   */
  ~BufferPoolOperation() = default;

  /**
   * @brief 获取操作类型
   * @return 操作类型枚举值
   */
  Type type() const { return type_; }
  
  /**
   * @brief 获取操作类型的整数值表示
   * @return 操作类型的整数值
   */
  int32_t type_id() const { return static_cast<int32_t>(type_); }

  /**
   * @brief 将操作类型转换为字符串表示
   * @return 操作类型的字符串表示
   */
  string to_string() const
  {
    string ret = std::to_string(type_id()) + ":";
    switch (type_) {
      case Type::ALLOCATE: return ret + "ALLOCATE";
      case Type::DEALLOCATE: return ret + "DEALLOCATE";
      default: return ret + "UNKNOWN";
    }
  }

private:
  Type type_;  ///< 操作类型
};

/**
 * @brief BufferPool 的日志记录结构体
 * @ingroup CLog
 * @details 包含BufferPool操作的所有必要信息，用于日志持久化和重放
 */
struct BufferPoolLogEntry
{
  int32_t buffer_pool_id;  ///< buffer pool的唯一标识符
  int32_t operation_type;  ///< 操作类型，对应BufferPoolOperation::Type的整数值
  PageNum page_num;        ///< 操作涉及的页面编号

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串表示
   */
  string to_string() const;
};

/**
 * @brief BufferPool 的日志记录处理器
 * @ingroup CLog
 * @details 负责记录BufferPool的操作日志，确保日志先于数据持久化到磁盘
 */
class BufferPoolLogHandler final
{
public:
  /**
   * @brief 构造函数
   * @param buffer_pool 关联的磁盘缓冲池
   * @param log_handler 日志处理器
   */
  BufferPoolLogHandler(DiskBufferPool &buffer_pool, LogHandler &log_handler);
  
  /**
   * @brief 析构函数
   */
  ~BufferPoolLogHandler() = default;

  /**
   * @brief 记录分配页面的日志
   * @param page_num 分配的页面号
   * @param[out] lsn 分配页面的日志序列号，输出参数
   * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
   * @note TODO 可以把frame传过来，记录完日志，直接更新页面的lsn
   */
  RC allocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 记录释放页面的日志
   * @param page_num 释放的页面编号
   * @param[out] lsn 释放页面的日志序列号，输出参数
   * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC deallocate_page(PageNum page_num, LSN &lsn);

  /**
   * @brief 刷新页面到磁盘之前，需要保证页面对应的日志也已经刷新到磁盘
   * @details 如果页面刷新到磁盘了，但是日志很落后，在重启恢复时，就会出现异常，无法让所有的页面都恢复到一致的状态
   * @param page 需要刷新的页面
   * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC flush_page(Page &page);

private:
  /**
   * @brief 向日志系统追加一条日志
   * @param type 操作类型
   * @param page_num 页面编号
   * @param[out] lsn 生成的日志序列号，输出参数
   * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC append_log(BufferPoolOperation::Type type, PageNum page_num, LSN &lsn);

private:
  DiskBufferPool &buffer_pool_;  ///< 关联的磁盘缓冲池
  LogHandler     &log_handler_;  ///< 日志处理器，用于实际写入日志
};

/**
 * @brief BufferPool 的日志重放器
 * @ingroup CLog
 * @details 负责重放BufferPool相关的日志，用于数据库崩溃恢复时重建BufferPool状态
 */
class BufferPoolLogReplayer final : public LogReplayer
{
public:
  /**
   * @brief 构造函数
   * @param bp_manager 缓冲池管理器
   */
  BufferPoolLogReplayer(BufferPoolManager &bp_manager);
  
  /**
   * @brief 析构函数
   */
  virtual ~BufferPoolLogReplayer() = default;

  /**
   * @brief 重放一条日志条目
   * @param entry 要重放的日志条目
   * @return 执行结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC replay(const LogEntry &entry) override;

private:
  BufferPoolManager &bp_manager_;  ///< 缓冲池管理器，用于执行实际的重放操作
};
