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
// Created by Meiyi & Wangyunlai on 2021/5/13.
//

#include <limits.h>
#include <string.h>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/lang/span.h"
#include "common/lang/algorithm.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "storage/db/db.h"
#include "storage/buffer/disk_buffer_pool.h"
#include "storage/common/condition_filter.h"
#include "storage/common/meta_util.h"
#include "storage/index/bplus_tree_index.h"
#include "storage/index/index.h"
#include "storage/record/record_manager.h"
#include "storage/table/table.h"
#include "storage/trx/trx.h"
#include "storage/record/heap_record_scanner.h"
#include "storage/record/lsm_record_scanner.h"
#include "storage/table/heap_table_engine.h"
#include "storage/table/lsm_table_engine.h"

/**
 * @brief 表类析构函数
 */
Table::~Table()
{
}

/**
 * @brief 创建一个表
 * @details 创建表的元数据文件、数据文件，并初始化表引擎
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
RC Table::create(Db *db, int32_t table_id, const char *path, const char *name, const char *base_dir,
    span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format, StorageEngine storage_engine)
{
  if (table_id < 0) {
    LOG_WARN("invalid table id. table_id=%d, table_name=%s", table_id, name);
    return RC::INVALID_ARGUMENT;
  }

  if (common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }
  LOG_INFO("Begin to create table %s:%s", base_dir, name);

  if (attributes.size() == 0) {
    LOG_WARN("Invalid arguments. table_name=%s, attribute_count=%d", name, attributes.size());
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  // 使用 table_name.table记录一个表的元数据
  // 判断表文件是否已经存在
  int fd = ::open(path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
  if (fd < 0) {
    if (EEXIST == errno) {
      LOG_ERROR("Failed to create table file, it has been created. %s, EEXIST, %s", path, strerror(errno));
      return RC::SCHEMA_TABLE_EXIST;
    }
    LOG_ERROR("Create table file failed. filename=%s, errmsg=%d:%s", path, errno, strerror(errno));
    return RC::IOERR_OPEN;
  }

  close(fd);

  // 创建文件
  const vector<FieldMeta> *trx_fields = db->trx_kit().trx_fields();
  if ((rc = table_meta_.init(table_id, name, trx_fields, attributes, primary_keys, storage_format, storage_engine)) != RC::SUCCESS) {
    LOG_ERROR("Failed to init table meta. name:%s, ret:%d", name, rc);
    return rc;  // delete table file
  }

  fstream fs;
  fs.open(path, ios_base::out | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open file for write. file name=%s, errmsg=%s", path, strerror(errno));
    return RC::IOERR_OPEN;
  }

  // 记录元数据到文件中
  table_meta_.serialize(fs);
  fs.close();

  db_       = db;

  string             data_file = table_data_file(base_dir, name);
  BufferPoolManager &bpm       = db->buffer_pool_manager();
  rc                           = bpm.create_file(data_file.c_str());
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to create disk buffer pool of data file. file name=%s", data_file.c_str());
    return rc;
  }

  // rc = init_record_handler(base_dir);
  // if (rc != RC::SUCCESS) {
  //   LOG_ERROR("Failed to create table %s due to init record handler failed.", data_file.c_str());
  //   // don't need to remove the data_file
  //   return rc;
  // }

  if (table_meta_.storage_engine() == StorageEngine::HEAP) {
    engine_ = make_unique<HeapTableEngine>(&table_meta_, db_, this);
  } else if (table_meta_.storage_engine() == StorageEngine::LSM) {
    engine_ = make_unique<LsmTableEngine>(&table_meta_, db_, this);
  } else {
    rc = RC::UNSUPPORTED;
    LOG_WARN("Unsupported storage engine type: %d", table_meta_.storage_engine());
    return rc;
  }
  rc = engine_->open();
  if (rc != RC::SUCCESS) {
    LOG_WARN("Failed to open table %s due to engine open failed.", data_file.c_str());
    return rc;
  }

  LOG_INFO("Successfully create table %s:%s", base_dir, name);
  return rc;
}

/**
 * @brief 打开一个表
 * @details 从文件加载表元数据，并初始化表引擎
 * @param db 数据库指针
 * @param meta_file 保存表元数据的文件完整路径
 * @param base_dir 表所在的文件夹
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::open(Db *db, const char *meta_file, const char *base_dir)
{
  // 加载元数据文件
  fstream fs;
  string  meta_file_path = string(base_dir) + common::FILE_PATH_SPLIT_STR + meta_file;
  fs.open(meta_file_path, ios_base::in | ios_base::binary);
  if (!fs.is_open()) {
    LOG_ERROR("Failed to open meta file for read. file name=%s, errmsg=%s", meta_file_path.c_str(), strerror(errno));
    return RC::IOERR_OPEN;
  }
  if (table_meta_.deserialize(fs) < 0) {
    LOG_ERROR("Failed to deserialize table meta. file name=%s", meta_file_path.c_str());
    fs.close();
    return RC::INTERNAL;
  }
  fs.close();

  db_       = db;

  // // 加载数据文件
  // RC rc = init_record_handler(base_dir);
  // if (rc != RC::SUCCESS) {
  //   LOG_ERROR("Failed to open table %s due to init record handler failed.", base_dir);
  //   // don't need to remove the data_file
  //   return rc;
  // }
  RC rc = RC::SUCCESS;

  if (table_meta_.storage_engine() == StorageEngine::HEAP) {
    engine_ = make_unique<HeapTableEngine>(&table_meta_, db_, this);
  }  else if (table_meta_.storage_engine() == StorageEngine::LSM) {
    engine_ = make_unique<LsmTableEngine>(&table_meta_, db_, this);
  } else {
    rc = RC::UNSUPPORTED;
    LOG_ERROR("Unsupported storage engine type: %d", table_meta_.storage_engine());
    return rc;
  }

  rc = engine_->open();
  if (rc != RC::SUCCESS) {
    LOG_ERROR("Failed to open table %s due to engine open failed.", base_dir);
    return rc;
  }

  return rc;
}

/**
 * @brief 在表中插入一条记录
 * @details 委托给表引擎执行插入操作
 * @param record 要插入的记录
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::insert_record(Record &record)
{
  return engine_->insert_record(record);
}

/**
 * @brief 在页面锁保护的情况下访问记录
 * @details 委托给表引擎执行访问操作
 * @param rid 记录标识符
 * @param visitor 访问者函数
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::visit_record(const RID &rid, function<bool(Record &)> visitor)
{
  return engine_->visit_record(rid, visitor);
}

/**
 * @brief 在事务上下文中插入记录
 * @details 委托给表引擎执行事务插入操作
 * @param record 要插入的记录
 * @param trx 事务对象指针
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::insert_record_with_trx(Record &record, Trx *trx)
{
  return engine_->insert_record_with_trx(record, trx);
}

/**
 * @brief 在事务上下文中删除记录
 * @details 委托给表引擎执行事务删除操作
 * @param record 要删除的记录
 * @param trx 事务对象指针
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::delete_record_with_trx(const Record &record, Trx *trx)
{
  return engine_->delete_record_with_trx(record, trx);
}

/**
 * @brief 在事务上下文中更新记录
 * @details 委托给表引擎执行事务更新操作
 * @param old_record 旧记录
 * @param new_record 新记录
 * @param trx 事务对象指针
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::update_record_with_trx(const Record &old_record, const Record &new_record, Trx* trx)
{
  return engine_->update_record_with_trx(old_record, new_record, trx);
}

/**
 * @brief 根据记录标识符获取记录
 * @details 委托给表引擎执行获取记录操作
 * @param rid 记录标识符
 * @param record 输出参数，用于存储获取的记录
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::get_record(const RID &rid, Record &record)
{
  return engine_->get_record(rid, record);
}

/**
 * @brief 获取表名
 * @return 表名
 */
const char *Table::name() const { return table_meta_.name(); }

/**
 * @brief 获取表元数据
 * @return 表元数据常量引用
 */
const TableMeta &Table::table_meta() const { return table_meta_; }

/**
 * @brief 根据给定的字段生成一个记录
 * @details 按照表的schema信息，将用户提供的字段值组装成一个完整的记录
 * @param value_num 字段的个数
 * @param values 每个字段的值
 * @param record 生成的记录数据
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::make_record(int value_num, const Value *values, Record &record)
{
  RC rc = RC::SUCCESS;
  // 检查字段类型是否一致
  if (value_num + table_meta_.sys_field_num() != table_meta_.field_num()) {
    LOG_WARN("Input values don't match the table's schema, table name:%s", table_meta_.name());
    return RC::SCHEMA_FIELD_MISSING;
  }

  const int normal_field_start_index = table_meta_.sys_field_num();
  // 复制所有字段的值
  int   record_size = table_meta_.record_size();
  char *record_data = (char *)malloc(record_size);
  memset(record_data, 0, record_size);

  for (int i = 0; i < value_num && OB_SUCC(rc); i++) {
    const FieldMeta *field = table_meta_.field(i + normal_field_start_index);
    const Value &    value = values[i];
    if (field->type() != value.attr_type()) {
      Value real_value;
      rc = Value::cast_to(value, field->type(), real_value);
      if (OB_FAIL(rc)) {
        LOG_WARN("failed to cast value. table name:%s,field name:%s,value:%s ",
            table_meta_.name(), field->name(), value.to_string().c_str());
        break;
      }
      rc = set_value_to_record(record_data, real_value, field);
    } else {
      rc = set_value_to_record(record_data, value, field);
    }
  }
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to make record. table name:%s", table_meta_.name());
    free(record_data);
    return rc;
  }

  record.set_data_owner(record_data, record_size);
  return RC::SUCCESS;
}

/**
 * @brief 设置值到记录中
 * @details 将指定的值复制到记录中的对应字段位置
 * @param record_data 记录数据指针
 * @param value 要设置的值
 * @param field 字段元数据
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::set_value_to_record(char *record_data, const Value &value, const FieldMeta *field)
{
  size_t       copy_len = field->len();
  const size_t data_len = value.length();
  if (field->type() == AttrType::CHARS) {
    if (copy_len > data_len) {
      copy_len = data_len + 1;
    }
  }
  memcpy(record_data + field->offset(), value.data(), copy_len);
  return RC::SUCCESS;
}

/**
 * @brief 获取记录扫描器
 * @details 委托给表引擎执行获取记录扫描器操作
 * @param scanner 输出参数，用于存储获取的记录扫描器
 * @param trx 事务对象指针
 * @param mode 读写模式
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::get_record_scanner(RecordScanner *&scanner, Trx *trx, ReadWriteMode mode)
{
  return engine_->get_record_scanner(scanner, trx, mode);
}

/**
 * @brief 获取块文件扫描器
 * @details 委托给表引擎执行获取块文件扫描器操作
 * @param scanner 块文件扫描器对象引用
 * @param trx 事务对象指针
 * @param mode 读写模式
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::get_chunk_scanner(ChunkFileScanner &scanner, Trx *trx, ReadWriteMode mode)
{
  return engine_->get_chunk_scanner(scanner, trx, mode);
}

/**
 * @brief 创建索引
 * @details 委托给表引擎执行创建索引操作
 * @param trx 事务对象指针
 * @param field_meta 字段元数据指针
 * @param index_name 索引名称
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::create_index(Trx *trx, const FieldMeta *field_meta, const char *index_name)
{
  return engine_->create_index(trx, field_meta, index_name);
}

/**
 * @brief 删除记录
 * @details 委托给表引擎执行删除记录操作
 * @param record 要删除的记录
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::delete_record(const Record &record)
{
  return engine_->delete_record(record);
}

/**
 * @brief 根据索引名称查找索引
 * @details 委托给表引擎执行查找索引操作
 * @param index_name 索引名称
 * @return 索引对象指针，若不存在则返回nullptr
 */
Index *Table::find_index(const char *index_name) const
{
  return engine_->find_index(index_name);
}

/**
 * @brief 根据字段名查找索引
 * @details 委托给表引擎执行查找索引操作
 * @param field_name 字段名称
 * @return 索引对象指针，若不存在则返回nullptr
 */
Index *Table::find_index_by_field(const char *field_name) const
{
  return engine_->find_index_by_field(field_name);
}

/**
 * @brief 将数据同步到磁盘
 * @details 委托给表引擎执行数据同步操作
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC Table::sync()
{
  return engine_->sync();
}
