/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/lang/comparator.h"
#include "common/log/log.h"
#include "common/type/char_type.h"
#include "common/value.h"

/**
 * @brief 比较两个字符串值的大小
 * @param[in] left 左操作数
 * @param[in] right 右操作数
 * @return 比较结果，小于0表示left小于right，等于0表示相等，大于0表示left大于right
 * 
 * 首先验证左右操作数类型是否都是CHARS，然后调用common::compare_string函数进行字符串比较。
 */
int CharType::compare(const Value &left, const Value &right) const
{
  ASSERT(left.attr_type() == AttrType::CHARS && right.attr_type() == AttrType::CHARS, "invalid type");
  return common::compare_string(
      (void *)left.value_.pointer_value_, left.length_, (void *)right.value_.pointer_value_, right.length_);
}

/**
 * @brief 从字符串设置值
 * @param[in,out] val 要设置的Value对象
 * @param[in] data 字符串数据
 * @return 操作结果，成功返回RC::SUCCESS
 * 
 * 调用val的set_string方法设置字符串值。
 */
RC CharType::set_value_from_str(Value &val, const string &data) const
{
  val.set_string(data.c_str());
  return RC::SUCCESS;
}

/**
 * @brief 将字符串值转换为指定类型
 * @param[in] val 源值
 * @param[in] type 目标类型
 * @param[out] result 转换后的结果
 * @return 操作结果，当前实现中除默认情况外都返回RC::UNIMPLEMENTED
 * 
 * 注意：当前实现中未完成任何具体的类型转换功能，仅返回UNIMPLEMENTED。
 */
RC CharType::cast_to(const Value &val, AttrType type, Value &result) const
{
  switch (type) {
    default: return RC::UNIMPLEMENTED;
  }
  return RC::SUCCESS;
}

/**
 * @brief 计算将字符串类型转换为指定类型的成本
 * @param[in] type 目标类型
 * @return 转换成本，数值越小成本越低
 * 
 * 当目标类型是CHARS时，成本为0；否则成本为INT32_MAX（表示转换成本极高或不支持）。
 */
int CharType::cast_cost(AttrType type)
{
  if (type == AttrType::CHARS) {
    return 0;
  }
  return INT32_MAX;
}

/**
 * @brief 将值转换为字符串表示
 * @param[in] val 源值
 * @param[out] result 转换后的字符串
 * @return 操作结果，成功返回RC::SUCCESS
 * 
 * 使用stringstream将字符串值转换为string对象。
 */
RC CharType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << val.value_.pointer_value_;
  result = ss.str();
  return RC::SUCCESS;
}