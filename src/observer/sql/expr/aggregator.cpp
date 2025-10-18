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
// Created by Wangyunlai on 2024/05/29.
//

#include "sql/expr/aggregator.h"
#include "common/log/log.h"

/**
 * @brief 累计处理一个输入值到总和中
 * @details 将输入值累加到聚合器内部存储的中间结果中。如果是第一个输入值，则直接赋值；
 * 否则，执行加法运算，将当前值与中间结果相加。
 * @param[in] value 要累加到总和中的输入值
 * @return 操作结果状态码，成功返回RC::SUCCESS
 * @retval RC::SUCCESS 累加成功
 */
RC SumAggregator::accumulate(const Value &value)
{
  // 检查是否为第一次累加操作
  if (value_.attr_type() == AttrType::UNDEFINED) {
    // 第一次累加，直接将输入值赋给中间结果
    value_ = value;
    return RC::SUCCESS;
  }
  
  // 确保输入值类型与中间结果类型一致，否则会导致计算错误
  ASSERT(value.attr_type() == value_.attr_type(), "type mismatch. value type: %s, value_.type: %s", 
        attr_type_to_string(value.attr_type()), attr_type_to_string(value_.attr_type()));
  
  // 执行加法操作，将输入值加到中间结果中
  Value::add(value, value_, value_);
  return RC::SUCCESS;
}

/**
 * @brief 获取计算得到的总和结果
 * @details 将聚合器内部存储的累加结果复制到输出参数中。对于求和操作，直接返回累计的中间结果即可。
 * @param[out] result 用于存储总和结果的值
 * @return 操作结果状态码，成功返回RC::SUCCESS
 * @retval RC::SUCCESS 获取结果成功
 */
RC SumAggregator::evaluate(Value& result)
{
  // 将内部存储的累加结果赋值给输出参数
  result = value_;
  return RC::SUCCESS;
}
