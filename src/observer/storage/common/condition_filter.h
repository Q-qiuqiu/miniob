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
// Created by Wangyunlai on 2021/5/7.
//

#pragma once

#include "sql/parser/parse.h"

class Record;
class Table;

/**
 * @brief 条件描述结构体，用于表示条件过滤中的操作数
 * @details 这个结构体可以表示表中的属性或者具体的值
 */
struct ConDesc
{
  bool  is_attr;      ///< 是否为属性，false 表示是具体值
  int   attr_length;  ///< 如果是属性，表示属性值的长度
  int   attr_offset;  ///< 如果是属性，表示在记录中的偏移量
  Value value;        ///< 如果是值类型，这里记录值的数据
};

/**
 * @brief 条件过滤器的抽象基类
 * @details 定义了条件过滤的接口，所有具体的条件过滤器都需要实现这个接口
 */
class ConditionFilter
{
public:
  /**
   * @brief 虚析构函数，确保派生类能够正确析构
   */
  virtual ~ConditionFilter();

  /**
   * @brief 过滤一条记录
   * @param rec 要过滤的记录
   * @return true 表示满足条件，false 表示不满足条件
   */
  virtual bool filter(const Record &rec) const = 0;
};

/**
 * @brief 默认条件过滤器，实现单个条件的过滤功能
 * @details 支持属性与属性、属性与值、值与值之间的比较操作
 */
class DefaultConditionFilter : public ConditionFilter
{
public:
  /**
   * @brief 默认构造函数
   */
  DefaultConditionFilter();
  
  /**
   * @brief 析构函数
   */
  virtual ~DefaultConditionFilter();

  /**
   * @brief 初始化条件过滤器
   * @param left 左操作数描述
   * @param right 右操作数描述
   * @param attr_type 属性类型
   * @param comp_op 比较操作符
   * @return 初始化成功返回 RC::SUCCESS，否则返回错误码
   */
  RC init(const ConDesc &left, const ConDesc &right, AttrType attr_type, CompOp comp_op);
  
  /**
   * @brief 从SQL条件节点初始化条件过滤器
   * @param table 表对象
   * @param condition SQL条件节点
   * @return 初始化成功返回 RC::SUCCESS，否则返回错误码
   */
  RC init(Table &table, const ConditionSqlNode &condition);

  /**
   * @brief 过滤一条记录
   * @param rec 要过滤的记录
   * @return true 表示满足条件，false 表示不满足条件
   */
  virtual bool filter(const Record &rec) const;

public:
  /**
   * @brief 获取左操作数描述
   * @return 左操作数描述的常量引用
   */
  const ConDesc &left() const { return left_; }
  
  /**
   * @brief 获取右操作数描述
   * @return 右操作数描述的常量引用
   */
  const ConDesc &right() const { return right_; }

  /**
   * @brief 获取比较操作符
   * @return 比较操作符
   */
  CompOp   comp_op() const { return comp_op_; }
  
  /**
   * @brief 获取属性类型
   * @return 属性类型
   */
  AttrType attr_type() const { return attr_type_; }

private:
  ConDesc  left_;         ///< 左操作数描述
  ConDesc  right_;        ///< 右操作数描述
  AttrType attr_type_ = AttrType::UNDEFINED;  ///< 属性类型
  CompOp   comp_op_   = NO_OP;  ///< 比较操作符
};

/**
 * @brief 复合条件过滤器，组合多个条件过滤器
 * @details 实现多个条件的组合过滤，采用逻辑与的方式连接多个条件
 */
class CompositeConditionFilter : public ConditionFilter
{
public:
  /**
   * @brief 默认构造函数
   */
  CompositeConditionFilter() = default;
  
  /**
   * @brief 析构函数
   */
  virtual ~CompositeConditionFilter();

  /**
   * @brief 初始化复合条件过滤器
   * @param filters 条件过滤器数组
   * @param filter_num 条件过滤器数量
   * @return 初始化成功返回 RC::SUCCESS，否则返回错误码
   */
  RC init(const ConditionFilter *filters[], int filter_num);
  
  /**
   * @brief 从SQL条件节点数组初始化复合条件过滤器
   * @param table 表对象
   * @param conditions SQL条件节点数组
   * @param condition_num 条件节点数量
   * @return 初始化成功返回 RC::SUCCESS，否则返回错误码
   */
  RC init(Table &table, const ConditionSqlNode *conditions, int condition_num);

  /**
   * @brief 过滤一条记录
   * @param rec 要过滤的记录
   * @return true 表示满足所有条件，false 表示不满足至少一个条件
   */
  virtual bool filter(const Record &rec) const;

public:
  /**
   * @brief 获取条件过滤器数量
   * @return 条件过滤器数量
   */
  int                    filter_num() const { return filter_num_; }
  
  /**
   * @brief 获取指定索引的条件过滤器
   * @param index 索引
   * @return 条件过滤器的常量引用
   */
  const ConditionFilter &filter(int index) const { return *filters_[index]; }

private:
  /**
   * @brief 初始化复合条件过滤器的内部实现
   * @param filters 条件过滤器数组
   * @param filter_num 条件过滤器数量
   * @param own_memory 是否拥有内存的所有权
   * @return 初始化成功返回 RC::SUCCESS，否则返回错误码
   */
  RC init(const ConditionFilter *filters[], int filter_num, bool own_memory);

private:
  const ConditionFilter **filters_      = nullptr;  ///< 条件过滤器数组
  int                     filter_num_   = 0;  ///< 条件过滤器数量
  bool                    memory_owner_ = false;  ///< filters_的内存是否由自己来控制
};
