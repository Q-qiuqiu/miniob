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

#pragma once

#include "common/sys/rc.h"
#include "common/lang/string.h"
#include "sql/parser/parse_defs.h"

namespace Json {
class Value;
}  // namespace Json

/**
 * @brief 字段元数据类
 * @details 用于描述数据表中字段的元信息，包括字段名称、数据类型、偏移量、长度、可见性等属性
 */
class FieldMeta
{
public:
  /**
   * @brief 默认构造函数
   * @details 初始化字段元数据为默认值
   */
  FieldMeta();
  
  /**
   * @brief 带参数的构造函数
   * @param[in] name 字段名称
   * @param[in] attr_type 字段数据类型
   * @param[in] attr_offset 字段在记录中的偏移量
   * @param[in] attr_len 字段长度
   * @param[in] visible 字段是否可见
   * @param[in] field_id 字段ID
   */
  FieldMeta(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id);
  
  /**
   * @brief 析构函数
   */
  ~FieldMeta() = default;

  /**
   * @brief 初始化字段元数据
   * @param[in] name 字段名称
   * @param[in] attr_type 字段数据类型
   * @param[in] attr_offset 字段在记录中的偏移量
   * @param[in] attr_len 字段长度
   * @param[in] visible 字段是否可见
   * @param[in] field_id 字段ID
   * @return 初始化成功返回RC::SUCCESS，否则返回错误码
   */
  RC init(const char *name, AttrType attr_type, int attr_offset, int attr_len, bool visible, int field_id);

public:
  /**
   * @brief 获取字段名称
   * @return 字段名称的常量指针
   */
  const char *name() const;
  
  /**
   * @brief 获取字段数据类型
   * @return 字段数据类型枚举值
   */
  AttrType    type() const;
  
  /**
   * @brief 获取字段在记录中的偏移量
   * @return 字段偏移量
   */
  int         offset() const;
  
  /**
   * @brief 获取字段长度
   * @return 字段长度
   */
  int         len() const;
  
  /**
   * @brief 获取字段是否可见
   * @return 字段可见返回true，否则返回false
   */
  bool        visible() const;
  
  /**
   * @brief 获取字段ID
   * @return 字段ID
   */
  int         field_id() const;

public:
  /**
   * @brief 输出字段元数据信息
   * @param[in] os 输出流对象
   */
  void desc(ostream &os) const;

public:
  /**
   * @brief 将字段元数据转换为JSON格式
   * @param[inout] json_value 用于存储转换结果的JSON值对象
   */
  void      to_json(Json::Value &json_value) const;
  
  /**
   * @brief 从JSON格式解析字段元数据
   * @param[in] json_value 包含字段元数据的JSON值对象
   * @param[out] field 用于存储解析结果的FieldMeta对象
   * @return 解析成功返回RC::SUCCESS，否则返回错误码
   */
  static RC from_json(const Json::Value &json_value, FieldMeta &field);

protected:
  /**
   * @brief 字段名称
   */
  string   name_;
  
  /**
   * @brief 字段数据类型
   */
  AttrType attr_type_;
  
  /**
   * @brief 字段在记录中的偏移量
   */
  int      attr_offset_;
  
  /**
   * @brief 字段长度
   */
  int      attr_len_;
  
  /**
   * @brief 字段是否可见
   */
  bool     visible_;
  
  /**
   * @brief 字段ID
   */
  int      field_id_;
};
