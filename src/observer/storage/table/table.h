/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Wangyunlai on 2021/5/12.
//

#pragma once

#include "storage/table/table_meta.h"
#include "storage/table/table_engine.h"
#include "common/types.h"
#include "common/lang/span.h"
#include "common/lang/functional.h"

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
 * @brief 表类
 * @details 表示数据库中的一个表，负责表的创建、打开、记录的增删改查等操作
 */
class Table
{
public:
  /**
   * @brief 默认构造函数
   */
  Table() = default;
  
  /**
   * @brief 析构函数
   */
  ~Table();

  // TODO: use TableEngine replace Table
  friend class TableEngine;   ///< 表引擎类友元声明
  friend class HeapTableEngine; ///< 堆表引擎类友元声明

  /**
   * @brief 创建一个表
   * @param db 数据库指针
   * @param table_id 表ID
   * @param path 元数据保存的文件(完整路径)
   * @param name 表名
   * @param base_dir 表数据存放的路径
   * @param attributes 字段信息列表
   * @param primary_keys 主键列表
   * @param storage_format 存储格式
   * @param storage_engine 存储引擎类型
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC create(Db *db, int32_t table_id, const char *path, const char *name, const char *base_dir,
      span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format,
      StorageEngine storage_engine);

  /**
   * @brief 打开一个表
   * @param db 数据库指针
   * @param meta_file 保存表元数据的文件完整路径
   * @param base_dir 表所在的文件夹，表记录数据文件、索引数据文件存放位置
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC open(Db *db, const char *meta_file, const char *base_dir);

  /**
   * @brief 根据给定的字段生成一个记录/行
   * @details 通常是由用户传过来的字段，按照schema信息组装成一个record
   * @param value_num 字段的个数
   * @param values 每个字段的值
   * @param record 生成的记录数据
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC make_record(int value_num, const Value *values, Record &record);

  /**
   * @brief 在当前的表中插入一条记录
   * @details 在表文件和索引中插入关联数据。这里只管在表中插入数据，不关心事务相关操作
   * @param record[in/out] 传入的数据包含具体的数据，插入成功会通过此字段返回RID
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC insert_record(Record &record);
  
  /**
   * @brief 在当前的表中删除一条记录
   * @param record 要删除的记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC delete_record(const Record &record);

  /**
   * @brief 在事务上下文中插入记录
   * @param record 要插入的记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC insert_record_with_trx(Record &record, Trx *trx);
  
  /**
   * @brief 在事务上下文中删除记录
   * @param record 要删除的记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC delete_record_with_trx(const Record &record, Trx *trx);
  
  /**
   * @brief 在事务上下文中更新记录
   * @param old_record 旧记录
   * @param new_record 新记录
   * @param trx 事务对象指针
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx);
  
  /**
   * @brief 根据记录标识符获取记录
   * @param rid 记录标识符
   * @param record 输出参数，用于存储获取的记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC get_record(const RID &rid, Record &record);

  // TODO refactor
  /**
   * @brief 创建索引
   * @param trx 事务对象指针
   * @param field_meta 字段元数据指针
   * @param index_name 索引名称
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name);

  /**
   * @brief 获取记录扫描器
   * @param scanner 输出参数，用于存储获取的记录扫描器
   * @param trx 事务对象指针
   * @param mode 读写模式
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode);

  /**
   * @brief 获取块文件扫描器
   * @param scanner 块文件扫描器对象引用
   * @param trx 事务对象指针
   * @param mode 读写模式
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode);

  /**
   * @brief 可以在页面锁保护的情况下访问记录
   * @details 当前是在事务中访问记录，为了提供一个"原子性"的访问模式
   * @param rid 记录标识符
   * @param visitor 访问者函数，处理记录
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC visit_record(const RID &rid, function<bool(Record &)> visitor);

public:
  /**
   * @brief 获取表ID
   * @return 表ID
   */
  int32_t     table_id() const { return table_meta_.table_id(); }
  
  /**
   * @brief 获取表名
   * @return 表名
   */
  const char *name() const;

  /**
   * @brief 获取数据库指针
   * @return 数据库指针
   */
  Db *db() const { return db_; }

  /**
   * @brief 获取表元数据
   * @return 表元数据常量引用
   */
  const TableMeta &table_meta() const;

  /**
   * @brief 将数据同步到磁盘
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC sync();

private:
  /**
   * @brief 设置值到记录中
   * @param record_data 记录数据指针
   * @param value 要设置的值
   * @param field 字段元数据
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC set_value_to_record(char *record_data, const Value &value, const FieldMeta *field);

private:
  // RC init_record_handler(const char *base_dir);

public:
  /**
   * @brief 根据索引名称查找索引
   * @param index_name 索引名称
   * @return 索引对象指针，若不存在则返回nullptr
   */
  Index *find_index(const char *index_name) const;
  
  /**
   * @brief 根据字段名查找索引
   * @param field_name 字段名称
   * @return 索引对象指针，若不存在则返回nullptr
   */
  Index *find_index_by_field(const char *field_name) const;

private:
  Db                *db_ = nullptr;          ///< 数据库指针
  TableMeta          table_meta_;            ///< 表元数据
  // DiskBufferPool    *data_buffer_pool_ = nullptr;  /// 数据文件关联的buffer pool
  // RecordFileHandler *record_handler_   = nullptr;  /// 记录操作
  // vector<Index *>    indexes_;
  unique_ptr<TableEngine> engine_ = nullptr;  ///< 表引擎智能指针
};
