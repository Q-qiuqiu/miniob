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
#include "common/type/integer_type.h"
#include "common/value.h"

/**
 * @brief 比较两个数值的大小
 * @param left 左边的操作数，必须是整数类型
 * @param right 右边的操作数，可以是整数或浮点数类型
 * @return 
 *  -1 表示 left < right
 *  0 表示 left = right
 *  1 表示 left > right
 *  INT32_MAX 表示未实现的比较
 */
int IntegerType::compare(const Value &left, const Value &right) const
{
  // 确保左边操作数是整数类型
  ASSERT(left.attr_type() == AttrType::INTS, "left type is integer");
  // 确保右边操作数是数值类型（整数或浮点数）
  ASSERT(right.attr_type() == AttrType::INTS || right.attr_type() == AttrType::FLOATS, "right type is not numeric");
  
  // 根据右边操作数的类型选择不同的比较方法
  if (right.attr_type() == AttrType::INTS) {
    // 两个整数比较，使用整数比较函数
    return common::compare_int((void *)&left.value_.int_value_, (void *)&right.value_.int_value_);
  } else if (right.attr_type() == AttrType::FLOATS) {
    // 整数与浮点数比较，先将整数转换为浮点数，再使用浮点数比较函数
    float left_val  = left.get_float();
    float right_val = right.get_float();
    return common::compare_float((void *)&left_val, (void *)&right_val);
  }
  
  // 其他情况返回未实现的比较结果
  return INT32_MAX;
}

/**
 * @brief 将整数值转换为指定类型
 * @param val 要转换的值，必须是整数类型
 * @param type 目标类型
 * @param result 用于存储转换结果
 * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
 */
RC IntegerType::cast_to(const Value &val, AttrType type, Value &result) const
{
  // 根据目标类型进行不同的转换
  switch (type) {
  case AttrType::FLOATS: {
    // 转换为浮点数类型
    float float_value = val.get_int();
    result.set_float(float_value);
    return RC::SUCCESS;
  }
  default:
    // 不支持的目标类型
    LOG_WARN("unsupported type %d", type);
    return RC::SCHEMA_FIELD_TYPE_MISMATCH;
  }
}

/**
 * @brief 计算两个整数的和
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC IntegerType::add(const Value &left, const Value &right, Value &result) const
{
  // 计算两个整数的和并设置结果
  result.set_int(left.get_int() + right.get_int());
  return RC::SUCCESS;
}

/**
 * @brief 计算两个整数的差
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC IntegerType::subtract(const Value &left, const Value &right, Value &result) const
{
  // 计算两个整数的差并设置结果
  result.set_int(left.get_int() - right.get_int());
  return RC::SUCCESS;
}

/**
 * @brief 计算两个整数的积
 * @param left 左边的操作数
 * @param right 右边的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC IntegerType::multiply(const Value &left, const Value &right, Value &result) const
{
  // 计算两个整数的积并设置结果
  result.set_int(left.get_int() * right.get_int());
  return RC::SUCCESS;
}

/**
 * @brief 计算整数的负值
 * @param val 要取负值的操作数
 * @param result 用于存储计算结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC IntegerType::negative(const Value &val, Value &result) const
{
  // 计算整数的负值并设置结果
  result.set_int(-val.get_int());
  return RC::SUCCESS;
}

/**
 * @brief 从字符串设置整数值
 * @param val 要设置值的对象
 * @param data 字符串数据
 * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
 */
RC IntegerType::set_value_from_str(Value &val, const string &data) const
{
  RC                rc = RC::SUCCESS;
  stringstream deserialize_stream;
  deserialize_stream.clear();  // 清理stream的状态，防止多次解析出现异常
  deserialize_stream.str(data);
  int int_value;
  deserialize_stream >> int_value;
  
  // 检查解析是否成功且整个字符串都被解析
  if (!deserialize_stream || !deserialize_stream.eof()) {
    rc = RC::SCHEMA_FIELD_TYPE_MISMATCH;
  } else {
    val.set_int(int_value);
  }
  return rc;
}

/**
 * @brief 将整数值转换为字符串
 * @param val 要转换的值
 * @param result 用于存储转换结果
 * @return 执行结果，成功返回 RC::SUCCESS
 */
RC IntegerType::to_string(const Value &val, string &result) const
{
  stringstream ss;
  ss << val.value_.int_value_;  // 将整数值写入字符串流
  result = ss.str();  // 获取字符串结果
  return RC::SUCCESS;
}