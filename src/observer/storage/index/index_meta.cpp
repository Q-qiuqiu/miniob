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
// Created by Wangyunlai.wyl on 2021/5/18.
//

#include "storage/index/index_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "storage/field/field_meta.h"
#include "storage/table/table_meta.h"
#include "json/json.h"

/**
 * @brief JSON序列化时的索引名称字段名
 */
const static Json::StaticString FIELD_NAME("name");

/**
 * @brief JSON序列化时的索引字段名
 */
const static Json::StaticString FIELD_FIELD_NAME("field_name");

/**
 * @brief 初始化索引元数据实现
 * @param name 索引名称
 * @param field 字段元数据对象
 * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
 * @details 验证索引名称是否有效，并设置索引名称和对应的字段名
 */
RC IndexMeta::init(const char *name, const FieldMeta &field)
{
  if (common::is_blank(name)) {
    LOG_ERROR("Failed to init index, name is empty.");
    return RC::INVALID_ARGUMENT;
  }

  name_  = name;
  field_ = field.name();
  return RC::SUCCESS;
}

/**
 * @brief 将索引元数据转换为JSON格式实现
 * @param json_value 输出参数，用于存储转换后的JSON值
 * @details 将索引名称和字段名存储到JSON对象中
 */
void IndexMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME]       = name_;
  json_value[FIELD_FIELD_NAME] = field_;
}

/**
 * @brief 从JSON格式解析索引元数据实现
 * @param table 表元数据对象
 * @param json_value 包含索引元数据的JSON值
 * @param index 输出参数，用于存储解析后的索引元数据
 * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
 * @details 从JSON对象中提取索引名称和字段名，验证字段是否存在于表中，然后初始化索引元数据
 */
RC IndexMeta::from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index)
{
  const Json::Value &name_value  = json_value[FIELD_NAME];
  const Json::Value &field_value = json_value[FIELD_FIELD_NAME];
  if (!name_value.isString()) {
    LOG_ERROR("Index name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  if (!field_value.isString()) {
    LOG_ERROR("Field name of index [%s] is not a string. json value=%s",
        name_value.asCString(), field_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  const FieldMeta *field = table.field(field_value.asCString());
  if (nullptr == field) {
    LOG_ERROR("Deserialize index [%s]: no such field: %s", name_value.asCString(), field_value.asCString());
    return RC::SCHEMA_FIELD_MISSING;
  }

  return index.init(name_value.asCString(), *field);
}

/**
 * @brief 获取索引名称实现
 * @return 索引名称的字符串指针
 */
const char *IndexMeta::name() const { return name_.c_str(); }

/**
 * @brief 获取索引对应的字段名实现
 * @return 字段名称的字符串指针
 */
const char *IndexMeta::field() const { return field_.c_str(); }

/**
 * @brief 打印索引信息到输出流实现
 * @param os 输出流对象
 * @details 将索引的名称和对应的字段名打印到指定的输出流
 */
void IndexMeta::desc(ostream &os) const { os << "index name=" << name_ << ", field=" << field_; }