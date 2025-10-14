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

#include "common/sys/rc.h"
#include "common/type/data_type.h"

/**
 * @brief 固定长度的字符串类型
 * @ingroup DataType
 * 
 * CharType类是DataType的子类，专门用于处理字符串类型的数据。
 * 实现了字符串的比较、转换、设置值等操作。
 */
class CharType : public DataType
{
public:
  /**
   * @brief 构造函数，初始化属性类型为CHARS
   */
  CharType() : DataType(AttrType::CHARS) {}

  /**
   * @brief 析构函数
   */
  virtual ~CharType() = default;

  /**
   * @brief 比较两个字符串值的大小
   * @param[in] left 左操作数
   * @param[in] right 右操作数
   * @return 比较结果，小于0表示left小于right，等于0表示相等，大于0表示left大于right
   */
  int compare(const Value &left, const Value &right) const override;

  /**
   * @brief 将字符串值转换为指定类型
   * @param[in] val 源值
   * @param[in] type 目标类型
   * @param[out] result 转换后的结果
   * @return 操作结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC cast_to(const Value &val, AttrType type, Value &result) const override;

  /**
   * @brief 从字符串设置值
   * @param[in,out] val 要设置的Value对象
   * @param[in] data 字符串数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC set_value_from_str(Value &val, const string &data) const override;

  /**
   * @brief 计算将字符串类型转换为指定类型的成本
   * @param[in] type 目标类型
   * @return 转换成本，数值越小成本越低
   */
  int cast_cost(AttrType type) override;

  /**
   * @brief 将值转换为字符串表示
   * @param[in] val 源值
   * @param[out] result 转换后的字符串
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC to_string(const Value &val, string &result) const override;
};
