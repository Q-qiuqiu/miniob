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
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/type/float_type.h"
#include "common/value.h"
#include "common/lang/limits.h"
#include "common/value.h"

/**
 * @brief 比较两个浮点数的大小
 * @param left 左边的操作数，必须是 FLOAT 类型
 * @param right 右边的操作数，必须是 INT 或 FLOAT 类型
 * @return 
 *  -1 表示 left < right
 *  0 表示 left = right
 *  1 表示 left > right
 *  INT32_MAX 表示未实现的比较
 */
int FloatType::compare(const Value &left, const Value &right) const
{
  // 断言检查左操作数是否为浮点类型
  ASSERT(left.attr_type() == AttrType::FLOATS, "left type is not integer");
  // 断言检查右操作数是否为数值类型（整数或浮点数）
  ASSERT(right.attr_type() == AttrType::INTS || right.attr_type() == AttrType::FLOATS, "right type is not numeric");
  
  float left_val  = left.get_float();  // 获取左操作数的浮点值
  float right_val = right.get_float(); // 获取右操作数的浮点值
  
  // 使用通用的浮点比较函数进行比较
  return common::compare_float((void *)&left_val, (void *)&right_val);
}

/**
 * @brief 计算 left + right，并将结果保存到 result 中
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC FloatType::add(const Value &left, const Value &right, Value &result) const
{
  // 计算两个浮点数的和并设置结果
  result.set_float(left.get_float() + right.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 计算 left - right，并将结果保存到 result 中
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC FloatType::subtract(const Value &left, const Value &right, Value &result) const
{
  // 计算两个浮点数的差并设置结果
  result.set_float(left.get_float() - right.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 计算 left * right，并将结果保存到 result 中
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC FloatType::multiply(const Value &left, const Value &right, Value &result) const
{
  // 计算两个浮点数的积并设置结果
  result.set_float(left.get_float() * right.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 计算 left / right，并将结果保存到 result 中
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 * @note 当右操作数为0时，结果设置为浮点数最大值（当前miniob没有NULL概念）
 */
RC FloatType::divide(const Value &left, const Value &right, Value &result) const
{
  // 检查右操作数是否接近于0（考虑浮点精度问题）
  if (right.get_float() > -EPSILON && right.get_float() < EPSILON) {
    // NOTE:
    // 设置为浮点数最大值是不正确的。通常的做法是设置为NULL，但是当前的miniob没有NULL概念，所以这里设置为浮点数最大值。
    result.set_float(numeric_limits<float>::max());
  } else {
    // 计算两个浮点数的商并设置结果
    result.set_float(left.get_float() / right.get_float());
  }
  return RC::SUCCESS;
}

/**
 * @brief 计算 -val，并将结果保存到 result 中
 * @param val 要取负值的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC FloatType::negative(const Value &val, Value &result) const
{
  // 计算浮点数的负值并设置结果
  result.set_float(-val.get_float());
  return RC::SUCCESS;
}

/**
 * @brief 从字符串设置浮点数值
 * @param val 要设置值的对象
 * @param data 字符串数据
 * @return 执行结果，成功返回 RC::SUCCESS，类型不匹配返回 RC::SCHEMA_FIELD_TYPE_MISMATCH
 */
RC FloatType::set_value_from_str(Value &val, const string &data) const
{
  RC                rc = RC::SUCCESS;
  stringstream deserialize_stream;
  deserialize_stream.clear();
  deserialize_stream.str(data);

  float float_value;
  // 尝试从字符串流中读取浮点数
  deserialize_stream >> float_value;
  // 检查读取是否成功，并且流是否已经到达末尾
  if (!deserialize_stream || !deserialize_stream.eof()) {
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH; // 类型不匹配
  } else {
    val.set_float(float_value); // 设置浮点数的值
  }
  return rc;
}

/**
 * @brief 将浮点数值转换为字符串
 * @param val 要转换的值
 * @param result 用于存储转换结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC FloatType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  // 使用通用的浮点转字符串函数将浮点数转换为字符串
  ss << common::double_to_str(val.value_.float_value_);
  result = ss.str();
  return RC::SUCCESS;
}
