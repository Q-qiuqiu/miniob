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

/**
 * @file aggregator.h
 * @brief 聚合函数计算模块
 * @details 该文件定义了聚合函数的基类和具体实现类，用于处理SQL中的聚合操作（如SUM、AVG、MIN、MAX等）。
 * 聚合器负责累计计算聚合结果，并在需要时返回最终的聚合值。
 */

#pragma once

#include "common/value.h"
#include "common/sys/rc.h"

/**
 * @brief 聚合器基类
 * @details 定义了所有聚合函数实现类需要遵循的接口规范。聚合器的主要功能是累计处理输入值，并在需要时输出聚合结果。
 */
class Aggregator
{
public:
  /**
   * @brief 析构函数
   * @details 虚析构函数，确保派生类对象能够正确析构
   */
  virtual ~Aggregator() = default;

  /**
   * @brief 累计处理一个输入值
   * @param[in] value 要累计的输入值
   * @return 操作结果状态码，成功返回RC::SUCCESS
   */
  virtual RC accumulate(const Value &value) = 0;
  virtual RC evaluate(Value &result)        = 0;

protected:
  /**
   * @brief 存储聚合中间结果的值
   * @details 不同类型的聚合器会以不同方式使用该值来存储中间计算结果
   */
  Value value_;
};

/**
 * @brief 求和聚合器
 * @details 实现SUM聚合函数，用于计算一组数值的总和
 */
class SumAggregator : public Aggregator
{
public:
  /**
   * @brief 累计处理一个输入值到总和中
   * @param[in] value 要累加到总和中的输入值
   * @return 操作结果状态码，成功返回RC::SUCCESS
   */
  RC accumulate(const Value &value) override;
  
  /**
   * @brief 获取计算得到的总和结果
   * @param[out] result 用于存储总和结果的值
   * @return 操作结果状态码，成功返回RC::SUCCESS
   */
  RC evaluate(Value &result) override;
};
