/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/type/data_type.h"

/**
 * @brief 整型数据类型
 * @ingroup DataType
 * @details 负责处理整数类型的所有操作，包括比较、算术运算、类型转换等
 */
class IntegerType : public DataType
{
public:
  /**
   * @brief 构造函数
   * @details 初始化数据类型为 INTS
   */
  IntegerType() : DataType(AttrType::INTS) {}
  
  /**
   * @brief 析构函数
   */
  virtual ~IntegerType() {}

  /**
   * @brief 比较两个整数的大小
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @return 
   *  -1 表示 left < right
   *  0 表示 left = right
   *  1 表示 left > right
   *  INT32_MAX 表示未实现的比较
   */
  int compare(const Value &left, const Value &right) const override;

  /**
   * @brief 计算 left + right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC add(const Value &left, const Value &right, Value &result) const override;
  
  /**
   * @brief 计算 left - right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC subtract(const Value &left, const Value &right, Value &result) const override;
  
  /**
   * @brief 计算 left * right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC multiply(const Value &left, const Value &right, Value &result) const override;
  
  /**
   * @brief 计算 -val，并将结果保存到 result 中
   * @param val 要取负值的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC negative(const Value &val, Value &result) const override;

  /**
   * @brief 将整数值转换为指定类型
   * @param val 要转换的值
   * @param type 目标类型
   * @param result 用于存储转换结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  /**
   * @brief 从字符串设置整数值
   * @param val 要设置值的对象
   * @param data 字符串数据
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC set_value_from_str(Value &val, const string &data) const override;

  /**
   * @brief 将整数值转换为字符串
   * @param val 要转换的值
   * @param result 用于存储转换结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  RC to_string(const Value &val, string &result) const override;
};