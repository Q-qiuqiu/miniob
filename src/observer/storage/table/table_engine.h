/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/types.h"
#include "common/lang/functional.h"
#include "storage/table/table_meta.h"

struct RID;               ///< 记录标识符
class Record;             ///< 记录类
class DiskBufferPool;     ///< 磁盘缓冲池类
class RecordFileHandler;  ///< 记录文件处理器类
class RecordScanner;      ///< 记录扫描器类
class ChunkFileScanner;   ///< 块文件扫描器类
class ConditionFilter;    ///< 条件过滤器类
class DefaultConditionFilter; ///< 默认条件过滤器类
class Index;              ///< 索引类
class IndexScanner;       ///< 索引扫描器类
class RecordDeleter;      ///< 记录删除器类
class Trx;                ///< 事务类
class Db;                 ///< 数据库类

/**
 * @brief 表引擎抽象基类
 * @details 定义了表操作的核心接口，是表存储引擎的抽象表示
 * @note 所有具体的表存储引擎需要继承此类并实现其纯虚函数
 */
class TableEngine
{
public:
  /**
   * @brief 构造函数
   * @param table_meta 表元数据指针
   */
  TableEngine(TableMeta *table_meta) : table_meta_(table_meta) {}
  
  /**
   * @brief 虚析构函数
   */
  virtual ~TableEngine() = default;

  /**
   * @brief 插入记录
   * @param record 要插入的记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC insert_record(Record &record) = 0;
  
  /**
   * @brief 删除记录
   * @param record 要删除的记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC delete_record(const Record &record) = 0;
  
  /**
   * @brief 在事务上下文中插入记录
   * @param record 要插入的记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC insert_record_with_trx(Record &record, Trx *trx) = 0;
  
  /**
   * @brief 在事务上下文中删除记录
   * @param record 要删除的记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC delete_record_with_trx(const Record &record, Trx *trx) = 0;
  
  /**
   * @brief 在事务上下文中更新记录
   * @param old_record 旧记录
   * @param new_record 新记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) = 0;
  
  /**
   * @brief 根据记录标识符获取记录
   * @param rid 记录标识符
   * @param record 输出参数，用于存储获取的记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC get_record(const RID &rid, Record &record) = 0;

  /**
   * @brief 创建索引
   * @param trx 事务对象指针
   * @param field_meta 字段元数据指针
   * @param index_name 索引名称
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) = 0;
  
  /**
   * @brief 获取记录扫描器
   * @param scanner 输出参数，用于存储获取的记录扫描器
   * @param trx 事务对象指针
   * @param mode 读写模式
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode) = 0;
  
  /**
   * @brief 获取块文件扫描器
   * @param scanner 块文件扫描器对象引用
   * @param trx 事务对象指针
   * @param mode 读写模式
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode) = 0;
  
  /**
   * @brief 访问指定记录
   * @param rid 记录标识符
   * @param visitor 访问者函数，处理记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     visit_record(const RID &rid, function<bool(Record &)> visitor) = 0;
  
  /**
   * @brief 将数据同步到磁盘
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     sync() = 0;
  
  /**
   * @brief 根据索引名称查找索引
   * @param index_name 索引名称
   * @return 索引对象指针，若不存在则返回nullptr
   */
  virtual Index *find_index(const char *index_name) const = 0;
  
  /**
   * @brief 根据字段名查找索引
   * @param field_name 字段名称
   * @return 索引对象指针，若不存在则返回nullptr
   */
  virtual Index *find_index_by_field(const char *field_name) const = 0;
  
  /**
   * @brief 打开表引擎
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC     open() = 0;
  
  /**
   * @brief 初始化表引擎
   * @note TODO: remove this function
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  virtual RC init() = 0;

protected:
  TableMeta *table_meta_ = nullptr;  ///< 表元数据指针
};
