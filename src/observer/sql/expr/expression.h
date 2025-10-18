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

/**
 * @file expression.h
 * @brief 表达式计算模块
 * @details 该文件定义了SQL查询中各种表达式的抽象基类和具体实现类，包括字段表达式、常量表达式、
 * 比较表达式、算术表达式、聚合表达式等。表达式是SQL查询计算的基本单元，用于处理数据值的获取、转换和计算。
 */

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "common/lang/unordered_set.h"
#include "common/value.h"
#include "storage/field/field.h"
#include "sql/expr/aggregator.h"
#include "storage/common/chunk.h"

class Tuple;

/**
 * @defgroup Expression
 * @brief 表达式
 */

/**
 * @brief 表达式类型
 * @ingroup Expression
 */
enum class ExprType
{
  NONE,
  STAR,                 ///< 星号，表示所有字段
  UNBOUND_FIELD,        ///< 未绑定的字段，需要在resolver阶段解析为FieldExpr
  UNBOUND_AGGREGATION,  ///< 未绑定的聚合函数，需要在resolver阶段解析为AggregateExpr

  FIELD,        ///< 字段。在实际执行时，根据行数据内容提取对应字段的值
  VALUE,        ///< 常量值
  CAST,         ///< 需要做类型转换的表达式
  COMPARISON,   ///< 需要做比较的表达式
  CONJUNCTION,  ///< 多个表达式使用同一种关系(AND或OR)来联结
  ARITHMETIC,   ///< 算术运算
  AGGREGATION,  ///< 聚合运算
};

/**
 * @brief 表达式的抽象基类
 * @ingroup Expression
 * @details 在SQL的元素中，任何需要得出值的元素都可以使用表达式来描述
 * 比如获取某个字段的值、比较运算、类型转换
 * 当然还有一些当前没有实现的表达式，比如算术运算。
 *
 * 通常表达式的值，是在真实的算子运算过程中，拿到具体的tuple后
 * 才能计算出来真实的值。但是有些表达式可能就表示某一个固定的
 * 值，比如ValueExpr。
 *
 * TODO 区分unbound和bound的表达式
 * @details 所有类型的SQL表达式的基类，定义了表达式系统的核心接口。在SQL查询中，任何需要计算数据值的元素
 * 都可以用表达式来表示，包括字段引用、常量值、比较运算、算术运算、聚合运算等。
 * 
 * 表达式系统采用了面向对象的设计模式，通过不同的派生类实现各种具体的表达式类型。每个表达式都可以:
 * 1. 根据输入的元组数据计算出值
 * 2. 尝试在优化阶段计算出常量值
 * 3. 从批量数据(Chunk)中获取表达式的计算结果列
 * 4. 提供表达式的元数据信息(类型、长度、名称等)
 */
class Expression
{
public:
  /**
   * @brief 默认构造函数
   */
  Expression() = default;

  /**
   * @brief 虚析构函数
   * @details 确保派生类对象能够被正确析构
   */
  virtual ~Expression() = default;

  /**
   * @brief 创建当前表达式的深拷贝
   * @return 指向新表达式对象的智能指针
   */
  virtual unique_ptr<Expression> copy() const = 0;

  /**
   * @brief 判断两个表达式是否相等
   * @param[in] other 要比较的另一个表达式
   * @return 如果两个表达式在语义上相等则返回true，否则返回false
   */
  virtual bool equal(const Expression &other) const { return false; }
  
  /**
   * @brief 根据输入的元组计算表达式的值
   * @param[in] tuple 包含数据的元组，可能是表中的一行数据或中间结果
   * @param[out] value 用于存储计算结果的值对象
   * @return 操作结果状态码，成功返回RC::SUCCESS
   */
  virtual RC get_value(const Tuple &tuple, Value &value) const = 0;

  /**
   * @brief 在没有元组的情况下尝试获取表达式的值
   * @details 主要用于优化阶段，对于可以在编译时计算出结果的表达式（如常量表达式），
   * 可以提前计算以提高执行效率
   * @param[out] value 用于存储计算结果的值对象
   * @return 如果能计算出值返回RC::SUCCESS，否则返回相应的错误码
   */
  virtual RC try_get_value(Value &value) const { return RC::UNIMPLEMENTED; }

  /**
   * @brief 从数据块中获取表达式对应的列
   * @details 用于批量计算，从包含多行数据的Chunk中获取表达式的计算结果列
   * @param[in] chunk 包含多行数据的数据块
   * @param[out] column 用于存储表达式计算结果的列
   * @return 操作结果状态码，成功返回RC::SUCCESS
   */
  virtual RC get_column(Chunk &chunk, Column &column) { return RC::UNIMPLEMENTED; }

  /**
   * @brief 获取表达式的类型
   * @return 表达式类型枚举值
   */
  virtual ExprType type() const = 0;

  /**
   * @brief 获取表达式计算结果的数据类型
   * @return 属性类型枚举值
   */
  virtual AttrType value_type() const = 0;

  /**
   * @brief 获取表达式计算结果的长度
   * @return 结果值的长度，对于可变长度类型很重要
   */
  virtual int value_length() const { return -1; }

  /**
   * @brief 获取表达式的名称
   * @return 表达式的名称，如字段名或用户定义的别名
   */
  virtual const char *name() const { return name_.c_str(); }
  
  /**
   * @brief 设置表达式的名称
   * @param[in] name 要设置的表达式名称
   */
  virtual void set_name(string name) { name_ = name; }

  /**
   * @brief 获取表达式在数据块中的位置
   * @return 表达式结果在Chunk中的列索引位置
   */
  virtual int pos() const { return pos_; }
  
  /**
   * @brief 设置表达式在数据块中的位置
   * @param[in] pos 表达式结果在Chunk中的列索引位置
   */
  virtual void set_pos(int pos) { pos_ = pos; }

  /**
   * @brief 计算表达式在数据块上的选择结果
   * @details 主要用于比较表达式，计算数据块中每行是否满足条件
   * @param[in] chunk 包含多行数据的数据块
   * @param[out] select 存储每行是否满足条件的布尔向量
   * @return 操作结果状态码
   */
  virtual RC eval(Chunk &chunk, vector<uint8_t> &select) { return RC::UNIMPLEMENTED; }

protected:
  /**
   * @brief 表达式在下层算子返回的chunk中的位置
   * @details 当pos_ = -1时，表示下层算子没有预先计算该表达式；当pos_ >= 0时，
   * 表示该表达式的结果已在下层算子的chunk中的指定位置可用
   */
  int pos_ = -1;

private:
  /**
   * @brief 表达式的名称
   */
  string name_;
};

class StarExpr : public Expression
{
public:
  StarExpr() : table_name_() {}
  StarExpr(const char *table_name) : table_name_(table_name) {}
  virtual ~StarExpr() = default;

  unique_ptr<Expression> copy() const override { return make_unique<StarExpr>(table_name_.c_str()); }

  ExprType type() const override { return ExprType::STAR; }
  AttrType value_type() const override { return AttrType::UNDEFINED; }

  RC get_value(const Tuple &tuple, Value &value) const override { return RC::UNIMPLEMENTED; }  // 不需要实现

  const char *table_name() const { return table_name_.c_str(); }

private:
  string table_name_;
};

class UnboundFieldExpr : public Expression
{
public:
  UnboundFieldExpr(const string &table_name, const string &field_name)
      : table_name_(table_name), field_name_(field_name)
  {}

  virtual ~UnboundFieldExpr() = default;

  unique_ptr<Expression> copy() const override { return make_unique<UnboundFieldExpr>(table_name_, field_name_); }

  ExprType type() const override { return ExprType::UNBOUND_FIELD; }
  AttrType value_type() const override { return AttrType::UNDEFINED; }

  RC get_value(const Tuple &tuple, Value &value) const override { return RC::INTERNAL; }

  const char *table_name() const { return table_name_.c_str(); }
  const char *field_name() const { return field_name_.c_str(); }

private:
  string table_name_;
  string field_name_;
};

/**
 * @brief 字段表达式
 * @ingroup Expression
 * @details 表示对表中某个字段的引用，用于在SQL查询中获取特定字段的值。
 * 字段表达式包含表名和字段名信息，能够从输入的元组中提取指定字段的值。
 */
class FieldExpr : public Expression
{
public:
  /**
   * @brief 默认构造函数
   */
  FieldExpr() = default;
  
  /**
   * @brief 构造函数，通过表指针和字段元数据创建字段表达式
   * @param[in] table 表指针
   * @param[in] field 字段元数据
   */
  FieldExpr(const Table *table, const FieldMeta *field) : field_(table, field) {}
  
  /**
   * @brief 构造函数，通过Field对象创建字段表达式
   * @param[in] field 字段对象
   */
  FieldExpr(const Field &field) : field_(field) {}

  /**
   * @brief 析构函数
   */
  virtual ~FieldExpr() = default;

  /**
   * @brief 判断两个字段表达式是否相等
   * @details 比较两个字段表达式是否引用同一个表中的同一个字段
   * @param[in] other 要比较的另一个表达式
   * @return 如果两个表达式引用同一个字段则返回true，否则返回false
   */
  bool equal(const Expression &other) const override;

  /**
   * @brief 创建当前字段表达式的深拷贝
   * @return 指向新字段表达式对象的智能指针
   */
  unique_ptr<Expression> copy() const override { return make_unique<FieldExpr>(field_); }

  /**
   * @brief 获取表达式类型
   * @return 表达式类型：ExprType::FIELD
   */
  ExprType type() const override { return ExprType::FIELD; }
  
  /**
   * @brief 获取字段的数据类型
   * @return 字段的属性类型
   */
  AttrType value_type() const override { return field_.attr_type(); }
  
  /**
   * @brief 获取字段的长度
   * @return 字段的长度
   */
  int value_length() const override { return field_.meta()->len(); }

  /**
   * @brief 获取字段对象的引用（非const版本）
   * @return 字段对象的引用
   */
  Field &field() { return field_; }

  /**
   * @brief 获取字段对象的const引用
   * @return 字段对象的const引用
   */
  const Field &field() const { return field_; }

  /**
   * @brief 获取字段所属的表名
   * @return 表名
   */
  const char *table_name() const { return field_.table_name(); }
  
  /**
   * @brief 获取字段名
   * @return 字段名
   */
  const char *field_name() const { return field_.field_name(); }

  /**
   * @brief 从数据块中获取字段对应的列
   * @param[in] chunk 包含多行数据的数据块
   * @param[out] column 用于存储字段值的列
   * @return 操作结果状态码
   */
  RC get_column(Chunk &chunk, Column &column) override;

  /**
   * @brief 从元组中获取字段的值
   * @param[in] tuple 包含数据的元组
   * @param[out] value 用于存储字段值的对象
   * @return 操作结果状态码
   */
  RC get_value(const Tuple &tuple, Value &value) const override;

private:
  /**
   * @brief 字段对象，包含表和字段的元数据信息
   */
  Field field_;
};

/**
 * @brief 常量值表达式
 * @ingroup Expression
 * @details 表示SQL查询中的常量值，如数字、字符串、NULL等。
 * 常量表达式在查询执行过程中其值不会改变，可以直接从内部存储的值对象中获取。
 */
class ValueExpr : public Expression
{
public:
  /**
   * @brief 默认构造函数
   */
  ValueExpr() = default;
  
  /**
   * @brief 构造函数，通过Value对象创建常量表达式
   * @param[in] value 常量值对象
   */
  explicit ValueExpr(const Value &value) : value_(value) {}

  /**
   * @brief 析构函数
   */
  virtual ~ValueExpr() = default;

  /**
   * @brief 判断两个常量表达式是否相等
   * @details 比较两个常量表达式的值是否相等
   * @param[in] other 要比较的另一个表达式
   * @return 如果两个表达式的值相等则返回true，否则返回false
   */
  bool equal(const Expression &other) const override;

  /**
   * @brief 创建当前常量表达式的深拷贝
   * @return 指向新常量表达式对象的智能指针
   */
  unique_ptr<Expression> copy() const override { return make_unique<ValueExpr>(value_); }

  /**
   * @brief 获取常量表达式的值
   * @details 对于常量表达式，直接返回存储的值，忽略输入的元组
   * @param[in] tuple 包含数据的元组（常量表达式不需要此参数，但为了符合接口要求而保留）
   * @param[out] value 用于存储常量值的对象
   * @return 操作结果状态码，总是成功
   */
  RC get_value(const Tuple &tuple, Value &value) const override;
  
  /**
   * @brief 从数据块中获取常量表达式对应的列
   * @details 为数据块中的每一行创建一个包含常量值的列
   * @param[in] chunk 包含多行数据的数据块
   * @param[out] column 用于存储常量值的列
   * @return 操作结果状态码
   */
  RC get_column(Chunk &chunk, Column &column) override;
  
  /**
   * @brief 在没有元组的情况下尝试获取表达式的值
   * @details 对于常量表达式，可以直接返回存储的值
   * @param[out] value 用于存储计算结果的值对象
   * @return 操作结果状态码，总是成功
   */
  RC try_get_value(Value &value) const override
  {
    value = value_;
    return RC::SUCCESS;
  }

  /**
   * @brief 获取表达式类型
   * @return 表达式类型：ExprType::VALUE
   */
  ExprType type() const override { return ExprType::VALUE; }
  
  /**
   * @brief 获取常量的数据类型
   * @return 常量的属性类型
   */
  AttrType value_type() const override { return value_.attr_type(); }
  
  /**
   * @brief 获取常量的长度
   * @return 常量的长度
   */
  int      value_length() const override { return value_.length(); }

  /**
   * @brief 获取常量的值（非const版本）
   * @param[out] value 用于存储常量值的对象
   */
  void         get_value(Value &value) const { value = value_; }
  
  /**
   * @brief 获取常量的值（const版本）
   * @return 常量值对象的const引用
   */
  const Value &get_value() const { return value_; }

private:
  /**
   * @brief 常量值对象，存储实际的常量数据
   */
  Value value_;
};

/**
 * @brief 类型转换表达式
 * @ingroup Expression
 */
class CastExpr : public Expression
{
public:
  CastExpr(unique_ptr<Expression> child, AttrType cast_type);
  virtual ~CastExpr();

  unique_ptr<Expression> copy() const override { return make_unique<CastExpr>(child_->copy(), cast_type_); }

  ExprType type() const override { return ExprType::CAST; }

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC try_get_value(Value &value) const override;

  AttrType value_type() const override { return cast_type_; }

  unique_ptr<Expression> &child() { return child_; }

private:
  RC cast(const Value &value, Value &cast_value) const;

private:
  unique_ptr<Expression> child_;      ///< 从这个表达式转换
  AttrType               cast_type_;  ///< 想要转换成这个类型
};

/**
 * @brief 比较表达式
 * @ingroup Expression
 */
class ComparisonExpr : public Expression
{
public:
  ComparisonExpr(CompOp comp, unique_ptr<Expression> left, unique_ptr<Expression> right);
  virtual ~ComparisonExpr();

  ExprType type() const override { return ExprType::COMPARISON; }
  RC       get_value(const Tuple &tuple, Value &value) const override;
  AttrType value_type() const override { return AttrType::BOOLEANS; }
  CompOp   comp() const { return comp_; }

  unique_ptr<Expression> copy() const override
  {
    return make_unique<ComparisonExpr>(comp_, left_->copy(), right_->copy());
  }

  /**
   * @brief 根据 ComparisonExpr 获得 `select` 结果。
   * select 的长度与chunk 的行数相同，表示每一行在ComparisonExpr 计算后是否会被输出。
   */
  RC eval(Chunk &chunk, vector<uint8_t> &select) override;

  unique_ptr<Expression> &left() { return left_; }
  unique_ptr<Expression> &right() { return right_; }

  /**
   * 尝试在没有tuple的情况下获取当前表达式的值
   * 在优化的时候，可能会使用到
   */
  RC try_get_value(Value &value) const override;

  /**
   * compare the two tuple cells
   * @param value the result of comparison
   */
  RC compare_value(const Value &left, const Value &right, bool &value) const;

  template <typename T>
  RC compare_column(const Column &left, const Column &right, vector<uint8_t> &result) const;

private:
  CompOp                 comp_;
  unique_ptr<Expression> left_;
  unique_ptr<Expression> right_;
};

/**
 * @brief 联结表达式
 * @ingroup Expression
 * 多个表达式使用同一种关系(AND或OR)来联结
 * 当前miniob仅有AND操作
 */
class ConjunctionExpr : public Expression
{
public:
  enum class Type
  {
    AND,
    OR
  };

public:
  ConjunctionExpr(Type type, vector<unique_ptr<Expression>> &children);
  virtual ~ConjunctionExpr() = default;

  unique_ptr<Expression> copy() const override
  {
    vector<unique_ptr<Expression>> children;
    for (auto &child : children_) {
      children.emplace_back(child->copy());
    }
    return make_unique<ConjunctionExpr>(conjunction_type_, children);
  }

  ExprType type() const override { return ExprType::CONJUNCTION; }
  AttrType value_type() const override { return AttrType::BOOLEANS; }
  RC       get_value(const Tuple &tuple, Value &value) const override;

  Type conjunction_type() const { return conjunction_type_; }

  vector<unique_ptr<Expression>> &children() { return children_; }

private:
  Type                           conjunction_type_;
  vector<unique_ptr<Expression>> children_;
};

/**
 * @brief 算术表达式
 * @ingroup Expression
 */
class ArithmeticExpr : public Expression
{
public:
  enum class Type
  {
    ADD,
    SUB,
    MUL,
    DIV,
    NEGATIVE,
  };

public:
  ArithmeticExpr(Type type, Expression *left, Expression *right);
  ArithmeticExpr(Type type, unique_ptr<Expression> left, unique_ptr<Expression> right);
  virtual ~ArithmeticExpr() = default;

  unique_ptr<Expression> copy() const override
  {
    if (right_) {
      return make_unique<ArithmeticExpr>(arithmetic_type_, left_->copy(), right_->copy());
    } else {
      return make_unique<ArithmeticExpr>(arithmetic_type_, left_->copy(), nullptr);
    }
  }

  bool     equal(const Expression &other) const override;
  ExprType type() const override { return ExprType::ARITHMETIC; }

  AttrType value_type() const override;
  int      value_length() const override
  {
    if (!right_) {
      return left_->value_length();
    }
    return 4;  // sizeof(float) or sizeof(int)
  };

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC get_column(Chunk &chunk, Column &column) override;

  RC try_get_value(Value &value) const override;

  Type arithmetic_type() const { return arithmetic_type_; }

  unique_ptr<Expression> &left() { return left_; }
  unique_ptr<Expression> &right() { return right_; }

private:
  RC calc_value(const Value &left_value, const Value &right_value, Value &value) const;

  RC calc_column(const Column &left_column, const Column &right_column, Column &column) const;

  template <bool LEFT_CONSTANT, bool RIGHT_CONSTANT>
  RC execute_calc(const Column &left, const Column &right, Column &result, Type type, AttrType attr_type) const;

private:
  Type                   arithmetic_type_;
  unique_ptr<Expression> left_;
  unique_ptr<Expression> right_;
};

class UnboundAggregateExpr : public Expression
{
public:
  UnboundAggregateExpr(const char *aggregate_name, Expression *child);
  UnboundAggregateExpr(const char *aggregate_name, unique_ptr<Expression> child);
  virtual ~UnboundAggregateExpr() = default;

  ExprType type() const override { return ExprType::UNBOUND_AGGREGATION; }

  unique_ptr<Expression> copy() const override
  {
    return make_unique<UnboundAggregateExpr>(aggregate_name_.c_str(), child_->copy());
  }

  const char *aggregate_name() const { return aggregate_name_.c_str(); }

  unique_ptr<Expression> &child() { return child_; }

  RC       get_value(const Tuple &tuple, Value &value) const override { return RC::INTERNAL; }
  AttrType value_type() const override { return child_->value_type(); }

private:
  string                 aggregate_name_;
  unique_ptr<Expression> child_;
};

class AggregateExpr : public Expression
{
public:
  enum class Type
  {
    COUNT,
    SUM,
    AVG,
    MAX,
    MIN,
  };

public:
  AggregateExpr(Type type, Expression *child);
  AggregateExpr(Type type, unique_ptr<Expression> child);
  virtual ~AggregateExpr() = default;

  bool equal(const Expression &other) const override;

  unique_ptr<Expression> copy() const override { return make_unique<AggregateExpr>(aggregate_type_, child_->copy()); }

  ExprType type() const override { return ExprType::AGGREGATION; }

  AttrType value_type() const override { return child_->value_type(); }
  int      value_length() const override { return child_->value_length(); }

  RC get_value(const Tuple &tuple, Value &value) const override;

  RC get_column(Chunk &chunk, Column &column) override;

  Type aggregate_type() const { return aggregate_type_; }

  unique_ptr<Expression> &child() { return child_; }

  const unique_ptr<Expression> &child() const { return child_; }

  unique_ptr<Aggregator> create_aggregator() const;

public:
  static RC type_from_string(const char *type_str, Type &type);

private:
  Type                   aggregate_type_;
  unique_ptr<Expression> child_;
};
