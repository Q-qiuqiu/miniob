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
// Created by Meiyi & Wangyunlai on 2021/5/12.
//

#include "common/lang/string.h"
#include "common/lang/algorithm.h"
#include "common/log/log.h"
#include "common/global_context.h"
#include "storage/table/table_meta.h"
#include "storage/trx/trx.h"
#include "json/json.h"

/**
 * @brief JSON序列化时使用的静态常量 - 表ID字段名
 */
static const Json::StaticString FIELD_TABLE_ID("table_id");
/**
 * @brief JSON序列化时使用的静态常量 - 表名字段名
 */
static const Json::StaticString FIELD_TABLE_NAME("table_name");
/**
 * @brief JSON序列化时使用的静态常量 - 存储格式字段名
 */
static const Json::StaticString FIELD_STORAGE_FORMAT("storage_format");
/**
 * @brief JSON序列化时使用的静态常量 - 存储引擎字段名
 */
static const Json::StaticString FIELD_STORAGE_ENGINE("storage_engine");
/**
 * @brief JSON序列化时使用的静态常量 - 字段列表字段名
 */
static const Json::StaticString FIELD_FIELDS("fields");
/**
 * @brief JSON序列化时使用的静态常量 - 索引列表字段名
 */
static const Json::StaticString FIELD_INDEXES("indexes");
/**
 * @brief JSON序列化时使用的静态常量 - 主键字段列表字段名
 */
static const Json::StaticString FIELD_PRIMARY_KEYS("primary_keys");

/**
 * @brief 拷贝构造函数实现
 * @param other 另一个TableMeta对象
 */
TableMeta::TableMeta(const TableMeta &other)
    : table_id_(other.table_id_),
      name_(other.name_),
      fields_(other.fields_),
      indexes_(other.indexes_),
      storage_format_(other.storage_format_),
      storage_engine_(other.storage_engine_),
      record_size_(other.record_size_)
{}

/**
 * @brief 交换两个TableMeta对象的内容
 * @param other 另一个TableMeta对象
 */
void TableMeta::swap(TableMeta &other) noexcept
{
  name_.swap(other.name_);
  fields_.swap(other.fields_);
  indexes_.swap(other.indexes_);
  std::swap(record_size_, other.record_size_);
}

/**
 * @brief 初始化表元数据
 * @details 设置表的基本信息，包括表ID、表名、字段、主键、存储格式和存储引擎
 * @param table_id 表ID
 * @param name 表名
 * @param trx_fields 事务相关字段
 * @param attributes 表的属性信息
 * @param primary_keys 主键字段列表
 * @param storage_format 存储格式
 * @param storage_engine 存储引擎
 * @return 成功返回RC::SUCCESS，失败返回相应的错误码
 */
RC TableMeta::init(int32_t table_id, const char *name, const vector<FieldMeta> *trx_fields,
                   span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format,
                   StorageEngine storage_engine)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (attributes.size() == 0) {
    LOG_ERROR("Invalid argument. name=%s, field_num=%d", name, attributes.size());
    return RC::INVALID_ARGUMENT;
  }

  RC rc = RC::SUCCESS;

  int field_offset  = 0;
  int trx_field_num = 0;

  if (trx_fields != nullptr) {
    trx_fields_ = *trx_fields;

    fields_.resize(attributes.size() + trx_fields->size());
    for (size_t i = 0; i < trx_fields->size(); i++) {
      const FieldMeta &field_meta = (*trx_fields)[i];
      fields_[i] = FieldMeta(field_meta.name(), field_meta.type(), field_offset, field_meta.len(), false /*visible*/, field_meta.field_id());
      field_offset += field_meta.len();
    }

    trx_field_num = static_cast<int>(trx_fields->size());
  } else {
    fields_.resize(attributes.size());
  }

  for (size_t i = 0; i < attributes.size(); i++) {
    const AttrInfoSqlNode &attr_info = attributes[i];
    // `i` is the col_id of fields[i]
    rc = fields_[i + trx_field_num].init(
      attr_info.name.c_str(), attr_info.type, field_offset, attr_info.length, true /*visible*/, i);
    if (OB_FAIL(rc)) {
      LOG_ERROR("Failed to init field meta. table name=%s, field name: %s", name, attr_info.name.c_str());
      return rc;
    }

    field_offset += attr_info.length;
  }

  primary_keys_ = primary_keys;
  record_size_ = field_offset;

  table_id_ = table_id;
  name_     = name;
  storage_format_ = storage_format;
  storage_engine_ = storage_engine;
  LOG_INFO("Sussessfully initialized table meta. table id=%d, name=%s", table_id, name);
  return RC::SUCCESS;
}

/**
 * @brief 添加索引元数据
 * @details 将索引元数据添加到表的索引列表中
 * @param index 索引元数据对象
 * @return 成功返回RC::SUCCESS
 */
RC TableMeta::add_index(const IndexMeta &index)
{
  indexes_.push_back(index);
  return RC::SUCCESS;
}

/**
 * @brief 获取表名
 * @return 表名的字符串指针
 */
const char *TableMeta::name() const { return name_.c_str(); }

/**
 * @brief 获取第一个事务字段
 * @return 事务字段的元数据指针
 */
const FieldMeta *TableMeta::trx_field() const { return &fields_[0]; }

/**
 * @brief 获取事务字段范围
 * @return 事务字段的span对象
 */
span<const FieldMeta> TableMeta::trx_fields() const
{
  return span<const FieldMeta>(fields_.data(), sys_field_num());
}

/**
 * @brief 根据索引获取字段元数据
 * @param index 字段索引
 * @return 字段元数据的常量指针
 */
const FieldMeta *TableMeta::field(int index) const { return &fields_[index]; }

/**
 * @brief 根据名称获取字段元数据
 * @param name 字段名称
 * @return 字段元数据的常量指针，若不存在则返回nullptr
 */
const FieldMeta *TableMeta::field(const char *name) const
{
  if (nullptr == name) {
    return nullptr;
  }
  for (const FieldMeta &field : fields_) {
    if (0 == strcmp(field.name(), name)) {
      return &field;
    }
  }
  return nullptr;
}

/**
 * @brief 根据偏移量查找字段元数据
 * @param offset 字段在记录中的偏移量
 * @return 字段元数据的常量指针，若不存在则返回nullptr
 */
const FieldMeta *TableMeta::find_field_by_offset(int offset) const
{
  for (const FieldMeta &field : fields_) {
    if (field.offset() == offset) {
      return &field;
    }
  }
  return nullptr;
}

/**
 * @brief 获取字段总数（包括系统字段）
 * @return 字段总数
 */
int TableMeta::field_num() const { return fields_.size(); }

/**
 * @brief 获取系统字段数量
 * @return 系统字段数量
 */
int TableMeta::sys_field_num() const { return static_cast<int>(trx_fields_.size()); }

/**
 * @brief 根据名称获取索引元数据
 * @param name 索引名称
 * @return 索引元数据的常量指针，若不存在则返回nullptr
 */
const IndexMeta *TableMeta::index(const char *name) const
{
  for (const IndexMeta &index : indexes_) {
    if (0 == strcmp(index.name(), name)) {
      return &index;
    }
  }
  return nullptr;
}

/**
 * @brief 根据字段名查找索引元数据
 * @param field 字段名称
 * @return 索引元数据的常量指针，若不存在则返回nullptr
 */
const IndexMeta *TableMeta::find_index_by_field(const char *field) const
{
  for (const IndexMeta &index : indexes_) {
    if (0 == strcmp(index.field(), field)) {
      return &index;
    }
  }
  return nullptr;
}

/**
 * @brief 根据索引获取索引元数据
 * @param i 索引位置
 * @return 索引元数据的常量指针
 */
const IndexMeta *TableMeta::index(int i) const { return &indexes_[i]; }

/**
 * @brief 获取索引数量
 * @return 索引数量
 */
int TableMeta::index_num() const { return indexes_.size(); }

/**
 * @brief 获取记录大小
 * @return 记录的字节大小
 */
int TableMeta::record_size() const { return record_size_; }

/**
 * @brief 序列化表元数据到输出流
 * @details 将表元数据转换为JSON格式并写入输出流
 * @param ss 输出流
 * @return 序列化的字节数，失败返回-1
 */
int TableMeta::serialize(ostream &ss) const
{
  Json::Value table_value;
  table_value[FIELD_TABLE_ID]   = table_id_;
  table_value[FIELD_TABLE_NAME] = name_;
  table_value[FIELD_STORAGE_FORMAT] = static_cast<int>(storage_format_);
  table_value[FIELD_STORAGE_ENGINE] = static_cast<int>(storage_engine_);

  Json::Value fields_value;
  for (const FieldMeta &field : fields_) {
    Json::Value field_value;
    field.to_json(field_value);
    fields_value.append(std::move(field_value));
  }

  table_value[FIELD_FIELDS] = std::move(fields_value);

  Json::Value indexes_value;
  for (const auto &index : indexes_) {
    Json::Value index_value;
    index.to_json(index_value);
    indexes_value.append(std::move(index_value));
  }
  table_value[FIELD_INDEXES] = std::move(indexes_value);

  Json::Value primary_keys_value;
  for (const auto &field : primary_keys_) {
    primary_keys_value.append(field);
  }
  table_value[FIELD_PRIMARY_KEYS] = std::move(primary_keys_value);

  Json::StreamWriterBuilder builder;
  Json::StreamWriter       *writer = builder.newStreamWriter();

  streampos old_pos = ss.tellp();
  writer->write(table_value, &ss);
  int ret = (int)(ss.tellp() - old_pos);

  delete writer;
  return ret;
}

/**
 * @brief 从输入流反序列化表元数据
 * @details 从输入流读取JSON格式数据并解析为表元数据
 * @param is 输入流
 * @return 反序列化的字节数，失败返回-1
 */
int TableMeta::deserialize(istream &is)
{
  Json::Value             table_value;
  Json::CharReaderBuilder builder;
  string             errors;

  streampos old_pos = is.tellg();
  if (!Json::parseFromStream(builder, is, &table_value, &errors)) {
    LOG_ERROR("Failed to deserialize table meta. error=%s", errors.c_str());
    return -1;
  }

  const Json::Value &table_id_value = table_value[FIELD_TABLE_ID];
  if (!table_id_value.isInt()) {
    LOG_ERROR("Invalid table id. json value=%s", table_id_value.toStyledString().c_str());
    return -1;
  }

  int32_t table_id = table_id_value.asInt();

  const Json::Value &table_name_value = table_value[FIELD_TABLE_NAME];
  if (!table_name_value.isString()) {
    LOG_ERROR("Invalid table name. json value=%s", table_name_value.toStyledString().c_str());
    return -1;
  }

  string table_name = table_name_value.asString();

  const Json::Value &fields_value = table_value[FIELD_FIELDS];
  if (!fields_value.isArray() || fields_value.size() <= 0) {
    LOG_ERROR("Invalid table meta. fields is not array, json value=%s", fields_value.toStyledString().c_str());
    return -1;
  }

  const Json::Value &storage_format_value = table_value[FIELD_STORAGE_FORMAT];
  if (!storage_format_value.isInt()) {
    LOG_ERROR("Invalid storage format. json value=%s", storage_format_value.toStyledString().c_str());
    return -1;
  }

  int32_t storage_format = storage_format_value.asInt();

  const Json::Value &storage_engine_value = table_value[FIELD_STORAGE_ENGINE];
  if (!storage_engine_value.isInt()) {
    LOG_ERROR("Invalid storage engine. json value=%s", storage_engine_value.toStyledString().c_str());
    return -1;
  }

  int32_t storage_engine = storage_engine_value.asInt();

  RC  rc        = RC::SUCCESS;
  int field_num = fields_value.size();

  vector<FieldMeta> fields(field_num);
  for (int i = 0; i < field_num; i++) {
    FieldMeta &field = fields[i];

    const Json::Value &field_value = fields_value[i];
    rc                             = FieldMeta::from_json(field_value, field);
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to deserialize table meta. table name =%s", table_name.c_str());
      return -1;
    }
  }

  auto comparator = [](const FieldMeta &f1, const FieldMeta &f2) { return f1.offset() < f2.offset(); };
  sort(fields.begin(), fields.end(), comparator);

  table_id_ = table_id;
  storage_format_ = static_cast<StorageFormat>(storage_format);
  storage_engine_ = static_cast<StorageEngine>(storage_engine);
  name_.swap(table_name);
  fields_.swap(fields);
  record_size_ = fields_.back().offset() + fields_.back().len() - fields_.begin()->offset();

  for (const FieldMeta &field_meta : fields_) {
    if (!field_meta.visible()) {
      trx_fields_.push_back(field_meta); // 字段加上trx标识更好
    }
  }

  const Json::Value &indexes_value = table_value[FIELD_INDEXES];
  if (!indexes_value.empty()) {
    if (!indexes_value.isArray()) {
      LOG_ERROR("Invalid table meta. indexes is not array, json value=%s", fields_value.toStyledString().c_str());
      return -1;
    }
    const int              index_num = indexes_value.size();
    vector<IndexMeta> indexes(index_num);
    for (int i = 0; i < index_num; i++) {
      IndexMeta &index = indexes[i];

      const Json::Value &index_value = indexes_value[i];
      rc                             = IndexMeta::from_json(*this, index_value, index);
      if (rc != RC::SUCCESS) {
        LOG_ERROR("Failed to deserialize table meta. table name=%s", table_name.c_str());
        return -1;
      }
    }
    indexes_.swap(indexes);
  }

  const Json::Value &primary_keys_value = table_value[FIELD_PRIMARY_KEYS];
  if (!primary_keys_value.empty()) {
    if (!primary_keys_value.isArray()) {
      LOG_ERROR("Invalid table meta. primary keys is not array, json value=%s", fields_value.toStyledString().c_str());
      return -1;
    }
    const int              primary_key_num = primary_keys_value.size();
    vector<string> primary_keys(primary_key_num);
    for (int i = 0; i < primary_key_num; i++) {
      const Json::Value &field_name_value = primary_keys_value[i];
      if (!field_name_value.isString()) {
        LOG_ERROR("Invalid table meta. primary key name is not string, json value=%s",
                  field_name_value.toStyledString().c_str());
        return -1;
      }
      string field_name = field_name_value.asString();
      primary_keys.push_back(field_name);
    }
    primary_keys_.swap(primary_keys);
  }

  return (int)(is.tellg() - old_pos);
}

/**
 * @brief 获取序列化大小
 * @return 当前返回-1，表示未实现
 */
int TableMeta::get_serial_size() const { return -1; }

/**
 * @brief 转换为字符串表示
 * @details 当前实现为空
 * @param output 输出字符串
 */
void TableMeta::to_string(string &output) const {}

/**
 * @brief 输出表元数据的详细描述
 * @param os 输出流
 */
void TableMeta::desc(ostream &os) const
{
  os << name_ << '(' << endl;
  for (const auto &field : fields_) {
    os << '\t';
    field.desc(os);
    os << endl;
  }

  for (const auto &index : indexes_) {
    os << '\t';
    index.desc(os);
    os << endl;
  }
  os << ')' << endl;
}
