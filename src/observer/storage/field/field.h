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
// Created by Wangyunlai on 2022/07/05.
//

#pragma once

#include "storage/field/field_meta.h"
#include "storage/table/table.h"

/**
 * @brief 字段类，用于表示表中的一个字段
 * @details 该类封装了字段的元数据信息，并提供了访问和操作字段数据的方法
 */
class Field
{
public:
  /**
   * @brief 默认构造函数
   */
  Field() = default;
  
  /**
   * @brief 构造函数
   * @param table 表对象指针
   * @param field 字段元数据指针
   */
  Field(const Table *table, const FieldMeta *field) : table_(table), field_(field) {}
  
  /**
   * @brief 拷贝构造函数
   */
  Field(const Field &) = default;

  /**
   * @brief 获取表对象指针
   * @return 表对象的常量指针
   */
  const Table     *table() const { return table_; }
  
  /**
   * @brief 获取字段元数据指针
   * @return 字段元数据的常量指针
   */
  const FieldMeta *meta() const { return field_; }

  /**
   * @brief 获取字段的数据类型
   * @return 字段的数据类型
   */
  AttrType attr_type() const { return field_->type(); }

  /**
   * @brief 获取表名
   * @return 表名的字符串指针
   */
  const char *table_name() const { return table_->name(); }
  
  /**
   * @brief 获取字段名
   * @return 字段名的字符串指针
   */
  const char *field_name() const { return field_->name(); }

  /**
   * @brief 设置表对象
   * @param table 表对象指针
   */
  void set_table(const Table *table) { this->table_ = table; }
  
  /**
   * @brief 设置字段元数据
   * @param field 字段元数据指针
   */
  void set_field(const FieldMeta *field) { this->field_ = field; }

  /**
   * @brief 设置整型字段值
   * @param record 记录对象
   * @param value 要设置的整数值
   */
  void set_int(Record &record, int value);
  
  /**
   * @brief 获取整型字段值
   * @param record 记录对象
   * @return 字段的整数值
   */
  int  get_int(const Record &record);

  /**
   * @brief 获取字段的原始数据
   * @param record 记录对象
   * @return 字段数据的字符串指针
   */
  const char *get_data(const Record &record);

private:
  const Table     *table_ = nullptr;  ///< 表对象指针
  const FieldMeta *field_ = nullptr;  ///< 字段元数据指针
};
