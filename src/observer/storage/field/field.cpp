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
// Created by Wangyunlai on 2023/04/24.
//

#include "storage/field/field.h"
#include "common/log/log.h"
#include "common/value.h"
#include "storage/record/record.h"

/**
 * @brief 设置整型字段值
 * @details 将指定的整数值设置到记录中对应字段的位置
 * @param record 要修改的记录对象
 * @param value 要设置的整数值
 * @warning 函数会进行断言检查，确保字段类型为整数且字段长度与整数值大小匹配
 */
void Field::set_int(Record &record, int value)
{
  ASSERT(field_->type() == AttrType::INTS, "could not set int value to a non-int field");
  ASSERT(field_->len() == sizeof(value), "invalid field len");

  char *field_data = record.data() + field_->offset();
  memcpy(field_data, &value, sizeof(value));
}

/**
 * @brief 获取整型字段值
 * @details 从记录中对应字段的位置读取整数值
 * @param record 要读取的记录对象
 * @return 字段的整数值
 * @note 内部使用Value类来转换和获取整数值
 */
int Field::get_int(const Record &record)
{
  Value value(field_->type(), const_cast<char *>(record.data() + field_->offset()), field_->len());
  return value.get_int();
}

/**
 * @brief 获取字段的原始数据
 * @details 返回记录中对应字段的原始数据指针
 * @param record 要读取的记录对象
 * @return 字段数据的指针
 */
const char *Field::get_data(const Record &record) { return record.data() + field_->offset(); }