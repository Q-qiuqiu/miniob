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

#include "storage/table/table_engine.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/db/db.h"

class Table;
/**
 * @brief 堆表引擎实现类
 * @details 实现了TableEngine抽象接口，负责管理以堆文件形式存储的表数据
 * 提供记录的增删改查、索引创建与维护等功能
 */
class HeapTableEngine : public TableEngine
{
public:
  friend class Table;
  /**
   * @brief 构造函数
   * @param table_meta 表的元数据
   * @param db 数据库对象指针
   * @param table 表对象指针
   */
  HeapTableEngine(TableMeta *table_meta, Db *db, Table *table) : TableEngine(table_meta), db_(db), table_(table) {}
  
  /**
   * @brief 析构函数
   * @details 释放所有资源，包括记录处理器、数据缓冲区池和索引对象
   */
  ~HeapTableEngine() override;

  /**
   * @brief 插入记录
   * @details 将记录插入到表中，并更新所有相关索引
   * @param record 要插入的记录
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC insert_record(Record &record) override;
  
  /**
   * @brief 删除记录
   * @details 从表中删除指定记录，并更新所有相关索引
   * @param record 要删除的记录
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC delete_record(const Record &record) override;
  
  /**
   * @brief 事务环境下插入记录（未实现）
   * @return 返回RC::UNSUPPORTED
   */
  RC insert_record_with_trx(Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  
  /**
   * @brief 事务环境下删除记录（未实现）
   * @return 返回RC::UNSUPPORTED
   */
  RC delete_record_with_trx(const Record &record, Trx *trx) override { return RC::UNSUPPORTED; }
  
  /**
   * @brief 事务环境下更新记录（未实现）
   * @return 返回RC::UNSUPPORTED
   */
  RC update_record_with_trx(const Record &old_record, const Record &new_record, Trx *trx) override
  {
    return RC::UNSUPPORTED;
  }
  
  /**
   * @brief 获取记录
   * @details 根据记录ID从表中获取记录数据
   * @param rid 记录ID
   * @param record 输出参数，用于存储获取到的记录
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC get_record(const RID &rid, Record &record) override;

  /**
   * @brief 创建索引
   * @details 在指定字段上创建索引，并为现有数据构建索引
   * @param trx 事务对象
   * @param field_meta 字段元数据
   * @param index_name 索引名称
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name) override;
  
  /**
   * @brief 获取记录扫描器
   * @details 创建并初始化一个记录扫描器，用于遍历表中的记录
   * @param scanner 输出参数，用于存储创建的扫描器指针
   * @param trx 事务对象
   * @param mode 读写模式
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode) override;
  
  /**
   * @brief 获取块扫描器
   * @details 初始化一个块扫描器，用于扫描表的数据块
   * @param scanner 块扫描器对象
   * @param trx 事务对象
   * @param mode 读写模式
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode) override;
  
  /**
   * @brief 访问记录
   * @details 根据记录ID访问记录，并执行回调函数
   * @param rid 记录ID
   * @param visitor 回调函数，处理访问到的记录
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC visit_record(const RID &rid, function<bool(Record &)> visitor) override;
  
  /**
   * @brief 同步数据
   * @details 将内存中的数据同步到磁盘
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC sync() override;

  /**
   * @brief 根据索引名称查找索引
   * @param index_name 索引名称
   * @return 索引对象指针，如果不存在返回nullptr
   */
  Index *find_index(const char *index_name) const override;
  
  /**
   * @brief 根据字段名称查找索引
   * @param field_name 字段名称
   * @return 索引对象指针，如果不存在返回nullptr
   */
  Index *find_index_by_field(const char *field_name) const override;
  
  /**
   * @brief 打开表引擎
   * @details 初始化记录处理器、打开数据文件并加载所有索引
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC open() override;
  
  /**
   * @brief 初始化记录处理器
   * @details 打开数据文件并初始化记录处理器
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC init() override;

private:
  /**
   * @brief 向所有索引中插入条目
   * @param record 记录数据
   * @param rid 记录ID
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC insert_entry_of_indexes(const char *record, const RID &rid);
  
  /**
   * @brief 从所有索引中删除条目
   * @param record 记录数据
   * @param rid 记录ID
   * @param error_on_not_exists 条目不存在时是否报错
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC delete_entry_of_indexes(const char *record, const RID &rid, bool error_on_not_exists);

private:
  DiskBufferPool    *data_buffer_pool_ = nullptr;  ///< 数据文件关联的缓冲区池
  RecordFileHandler *record_handler_   = nullptr;  ///< 记录操作处理器
  vector<Index *>    indexes_;                     ///< 索引集合
  Db                *db_;                          ///< 数据库对象指针
  Table             *table_;                       ///< 表对象指针
};
