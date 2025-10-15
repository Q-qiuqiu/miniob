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

#include "storage/field/field_meta.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "sql/parser/parse_defs.h"

#include "json/json.h"

/**
 * @brief JSON序列化中的字段名称键
 */
const static Json::StaticString FIELD_NAME("name");

/**
 * @brief JSON序列化中的字段类型键
 */
const static Json::StaticString FIELD_TYPE("type");

/**
 * @brief JSON序列化中的字段偏移量键
 */
const static Json::StaticString FIELD_OFFSET("offset");

/**
 * @brief JSON序列化中的字段长度键
 */
const static Json::StaticString FIELD_LEN("len");

/**
 * @brief JSON序列化中的字段可见性键
 */
const static Json::StaticString FIELD_VISIBLE("visible");

/**
 * @brief JSON序列化中的字段ID键
 */
const static Json::StaticString FIELD_FIELD_ID("FIELD_id");

/**
 * @brief FieldMeta类的默认构造函数实现
 * @details 初始化字段元数据的所有成员变量为默认值：
 * - 数据类型为UNDEFINED
 * - 偏移量为-1
 * - 长度为0
 * - 可见性为false
 * - 字段ID为0
 */
FieldMeta::FieldMeta() : attr_type_(AttrType::UNDEFINED), attr_offset_(-1), attr_len_(0), visible_(false), field_id_(0) {}

/**
 * @brief FieldMeta类的带参数构造函数实现
 * @details 调用init方法初始化字段元数据，并断言初始化成功
 * @param[in] name 字段名称
 * @param[in] attr_type 字段数据类型
 * @param[in] attr_offset 字段在记录中的偏移量
 * @param[in] attr_len 字段长度
 * @param[in] visible 字段是否可见
 * @param[in] field_id 字段ID
 */
FieldMeta::FieldMeta(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id)
{
  [[maybe_unused]] RC rc = this->init(name, attr_type, attr_offset, attr_len, visible, field_id);
  ASSERT(rc == RC::SUCCESS, "failed to init field meta. rc=%s", strrc(rc));
}

/**
 * @brief FieldMeta类的初始化方法实现
 * @details 检查参数有效性并设置字段元数据的所有成员变量
 * @param[in] name 字段名称
 * @param[in] attr_type 字段数据类型
 * @param[in] attr_offset 字段在记录中的偏移量
 * @param[in] attr_len 字段长度
 * @param[in] visible 字段是否可见
 * @param[in] field_id 字段ID
 * @return 初始化成功返回RC::SUCCESS，否则返回错误码
 */
RC FieldMeta::init(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id)
{
  if (common::is_blank(name)) {
    LOG_WARN("Name cannot be empty");
    return RC::INVALID_ARGUMENT;
  }

  if (AttrType::UNDEFINED == attr_type || attr_offset < 0 || attr_len <= 0) {
    LOG_WARN("Invalid argument. name=%s, attr_type=%d, attr_offset=%d, attr_len=%d",
              name, attr_type, attr_offset, attr_len);
    return RC::INVALID_ARGUMENT;
  }

  name_        = name;
  attr_type_   = attr_type;
  attr_len_    = attr_len;
  attr_offset_ = attr_offset;
  visible_     = visible;
  field_id_ = field_id;

  LOG_INFO("Init a field with name=%s", name);
  return RC::SUCCESS;
}

/**
 * @brief 获取字段名称的实现
 * @return 字段名称的常量指针
 */
const char *FieldMeta::name() const { return name_.c_str(); }

/**
 * @brief 获取字段数据类型的实现
 * @return 字段数据类型枚举值
 */
AttrType FieldMeta::type() const { return attr_type_; }

/**
 * @brief 获取字段在记录中偏移量的实现
 * @return 字段偏移量
 */
int FieldMeta::offset() const { return attr_offset_; }

/**
 * @brief 获取字段长度的实现
 * @return 字段长度
 */
int FieldMeta::len() const { return attr_len_; }

/**
 * @brief 获取字段是否可见的实现
 * @return 字段可见返回true，否则返回false
 */
bool FieldMeta::visible() const { return visible_; }

/**
 * @brief 获取字段ID的实现
 * @return 字段ID
 */
int FieldMeta::field_id() const { return field_id_; }

/**
 * @brief 输出字段元数据信息的实现
 * @details 将字段的名称、类型、长度和可见性等信息输出到指定的输出流
 * @param[in] os 输出流对象
 */
void FieldMeta::desc(ostream &os) const
{
  os << "field name=" << name_ << ", type=" << attr_type_to_string(attr_type_) << ", len=" << attr_len_
     << ", visible=" << (visible_ ? "yes" : "no");
}

/**
 * @brief 将字段元数据转换为JSON格式的实现
 * @details 将字段的名称、类型、偏移量、长度、可见性和ID等信息存储到JSON值对象中
 * @param[inout] json_value 用于存储转换结果的JSON值对象
 */
void FieldMeta::to_json(Json::Value &json_value) const
{
  json_value[FIELD_NAME]    = name_;
  json_value[FIELD_TYPE]    = attr_type_to_string(attr_type_);
  json_value[FIELD_OFFSET]  = attr_offset_;
  json_value[FIELD_LEN]     = attr_len_;
  json_value[FIELD_VISIBLE] = visible_;
  json_value[FIELD_FIELD_ID] = field_id_;
}

/**
 * @brief 从JSON格式解析字段元数据的实现
 * @details 从JSON值对象中解析字段的名称、类型、偏移量、长度、可见性和ID等信息，并初始化FieldMeta对象
 * @param[in] json_value 包含字段元数据的JSON值对象
 * @param[out] field 用于存储解析结果的FieldMeta对象
 * @return 解析成功返回RC::SUCCESS，否则返回错误码
 */
RC FieldMeta::from_json(const Json::Value &json_value, FieldMeta &field)
{
  if (!json_value.isObject()) {
    LOG_ERROR("Failed to deserialize field. json is not an object. json value=%s", json_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  const Json::Value &name_value    = json_value[FIELD_NAME];
  const Json::Value &type_value    = json_value[FIELD_TYPE];
  const Json::Value &offset_value  = json_value[FIELD_OFFSET];
  const Json::Value &len_value     = json_value[FIELD_LEN];
  const Json::Value &visible_value = json_value[FIELD_VISIBLE];
  const Json::Value &field_id_value = json_value[FIELD_FIELD_ID];

  if (!name_value.isString()) {
    LOG_ERROR("Field name is not a string. json value=%s", name_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if (!type_value.isString()) {
    LOG_ERROR("Field type is not a string. json value=%s", type_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  if (!offset_value.isInt()) {
    LOG_ERROR("Offset is not an integer. json value=%s", offset_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if (!len_value.isInt()) {
    LOG_ERROR("Len is not an integer. json value=%s", len_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if (!visible_value.isBool()) {
    LOG_ERROR("Visible field is not a bool value. json value=%s", visible_value.toStyledString().c_str());
    return RC::INTERNAL;
  }
  if (!field_id_value.isInt()) {
    LOG_ERROR("Field id is not an integer. json value=%s", field_id_value.toStyledString().c_str());
    return RC::INTERNAL;
  }

  AttrType type = attr_type_from_string(type_value.asCString());
  if (AttrType::UNDEFINED == type) {
    LOG_ERROR("Got invalid field type. type=%d", type);
    return RC::INTERNAL;
  }

  const char *name    = name_value.asCString();
  int         offset  = offset_value.asInt();
  int         len     = len_value.asInt();
  bool        visible = visible_value.asBool();
  int         field_id  = field_id_value.asInt();
  return field.init(name, type, offset, len, visible, field_id);
}
