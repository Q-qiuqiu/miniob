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
 * @file expression.cpp
 * @brief SQL表达式计算模块实现文件
 * @details 该文件实现了SQL表达式计算系统中的各种表达式类型，包括字段表达式、值表达式、
 * 转换表达式、比较表达式、连接表达式和算术表达式等。表达式系统是SQL查询执行的核心组件，
 * 负责在查询过程中计算各种表达式的值，如条件判断、算术运算等。
 */

#include "sql/expr/expression.h"
#include "sql/expr/tuple.h"
#include "sql/expr/arithmetic_operator.hpp"

using namespace std;

/**
 * @brief 从元组中获取字段表达式的值
 * @details 根据字段的表名和字段名在元组中查找对应的值
 * @param[in] tuple 包含数据的元组
 * @param[out] value 用于存储获取到的字段值
 * @return 操作结果状态码
 */
RC FieldExpr::get_value(const Tuple &tuple, Value &value) const
{
  // 构建字段规范，在元组中查找对应的值
  return tuple.find_cell(TupleCellSpec(table_name(), field_name()), value);
}

/**
 * @brief 判断两个字段表达式是否相等
 * @details 比较两个字段表达式是否引用同一个表中的同一个字段
 * @param[in] other 要比较的另一个表达式
 * @return 如果两个表达式引用同一个字段则返回true，否则返回false
 */
bool FieldExpr::equal(const Expression &other) const
{
  // 自反性检查
  if (this == &other) {
    return true;
  }
  // 类型检查
  if (other.type() != ExprType::FIELD) {
    return false;
  }
  // 类型转换后比较表名和字段名
  const auto &other_field_expr = static_cast<const FieldExpr &>(other);
  return table_name() == other_field_expr.table_name() && field_name() == other_field_expr.field_name();
}

// TODO: 在进行表达式计算时，`chunk` 包含了所有列，因此可以通过 `field_id` 获取到对应列。
// 后续可以优化成在 `FieldExpr` 中存储 `chunk` 中某列的位置信息。
/**
 * @brief 从数据块中获取字段对应的列
 * @details 根据存储的位置信息或字段ID获取对应的列数据
 * @param[in] chunk 包含多行数据的数据块
 * @param[out] column 用于存储获取到的列数据的引用
 * @return 操作结果状态码
 */
RC FieldExpr::get_column(Chunk &chunk, Column &column)
{
  // 如果已经缓存了位置信息，直接使用
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
  } else {
    // 否则通过字段ID获取列
    column.reference(chunk.column(field().meta()->field_id()));
  }
  return RC::SUCCESS;
}

/**
 * @brief 判断两个值表达式是否相等
 * @details 比较两个值表达式的值是否相等
 * @param[in] other 要比较的另一个表达式
 * @return 如果两个表达式的值相等则返回true，否则返回false
 */
bool ValueExpr::equal(const Expression &other) const
{
  // 自反性检查
  if (this == &other) {
    return true;
  }
  // 类型检查
  if (other.type() != ExprType::VALUE) {
    return false;
  }
  // 类型转换后比较值
  const auto &other_value_expr = static_cast<const ValueExpr &>(other);
  return value_.compare(other_value_expr.get_value()) == 0;
}

/**
 * @brief 获取值表达式的值
 * @details 对于值表达式，直接返回存储的值，忽略输入的元组
 * @param[in] tuple 包含数据的元组（值表达式不需要此参数，但为了符合接口要求而保留）
 * @param[out] value 用于存储表达式值的对象
 * @return 操作结果状态码，总是成功
 */
RC ValueExpr::get_value(const Tuple &tuple, Value &value) const
{
  // 直接将内部存储的值复制给输出参数
  value = value_;
  return RC::SUCCESS;
}

/**
 * @brief 从数据块中获取值表达式对应的列
 * @details 为值表达式创建一个常量列
 * @param[in] chunk 包含多行数据的数据块（值表达式不需要此参数，但为了符合接口要求而保留）
 * @param[out] column 用于存储创建的常量列
 * @return 操作结果状态码
 */
RC ValueExpr::get_column(Chunk &chunk, Column &column)
{
  // 用值表达式的值初始化列
  column.init(value_);
  return RC::SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////////
CastExpr::CastExpr(unique_ptr<Expression> child, AttrType cast_type) : child_(std::move(child)), cast_type_(cast_type)
{}

CastExpr::~CastExpr() {}

RC CastExpr::cast(const Value &value, Value &cast_value) const
{
  RC rc = RC::SUCCESS;
  if (this->value_type() == value.attr_type()) {
    cast_value = value;
    return rc;
  }
  rc = Value::cast_to(value, cast_type_, cast_value);
  return rc;
}

RC CastExpr::get_value(const Tuple &tuple, Value &result) const
{
  Value value;
  RC rc = child_->get_value(tuple, value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(value, result);
}

RC CastExpr::try_get_value(Value &result) const
{
  Value value;
  RC rc = child_->try_get_value(value);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  return cast(value, result);
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 比较表达式的构造函数
 * @details 创建一个比较表达式，包含比较操作符和两个子表达式
 * @param[in] comp 比较操作符
 * @param[in] left 左子表达式，智能指针形式
 * @param[in] right 右子表达式，智能指针形式
 */
ComparisonExpr::ComparisonExpr(CompOp comp, unique_ptr<Expression> left, unique_ptr<Expression> right)
    : comp_(comp), left_(std::move(left)), right_(std::move(right))
{
}

/**
 * @brief 比较表达式的析构函数
 */
ComparisonExpr::~ComparisonExpr() {}

/**
 * @brief 比较两个值
 * @details 根据比较操作符对两个值进行比较，返回布尔结果
 * @param[in] left 左操作数
 * @param[in] right 右操作数
 * @param[out] result 比较结果，根据比较操作符确定的布尔值
 * @return 操作结果状态码，成功返回RC::SUCCESS，不支持的比较操作符返回RC::INTERNAL
 */
RC ComparisonExpr::compare_value(const Value &left, const Value &right, bool &result) const
{
  RC  rc         = RC::SUCCESS;
  // 先获取两个值的比较结果，返回-1、0或1
  int cmp_result = left.compare(right);
  result         = false;
  
  // 根据比较操作符确定最终的布尔结果
  switch (comp_) {
    case EQUAL_TO: {
      // 相等：比较结果等于0
      result = (0 == cmp_result);
    } break;
    case LESS_EQUAL: {
      // 小于等于：比较结果小于等于0
      result = (cmp_result <= 0);
    } break;
    case NOT_EQUAL: {
      // 不等于：比较结果不等于0
      result = (cmp_result != 0);
    } break;
    case LESS_THAN: {
      // 小于：比较结果小于0
      result = (cmp_result < 0);
    } break;
    case GREAT_EQUAL: {
      // 大于等于：比较结果大于等于0
      result = (cmp_result >= 0);
    } break;
    case GREAT_THAN: {
      // 大于：比较结果大于0
      result = (cmp_result > 0);
    } break;
    default: {
      LOG_WARN("unsupported comparison. %d", comp_);
      rc = RC::INTERNAL;
    } break;
  }

  return rc;
}

RC ComparisonExpr::try_get_value(Value &cell) const
{
  if (left_->type() == ExprType::VALUE && right_->type() == ExprType::VALUE) {
    ValueExpr *  left_value_expr  = static_cast<ValueExpr *>(left_.get());
    ValueExpr *  right_value_expr = static_cast<ValueExpr *>(right_.get());
    const Value &left_cell        = left_value_expr->get_value();
    const Value &right_cell       = right_value_expr->get_value();

    bool value = false;
    RC   rc    = compare_value(left_cell, right_cell, value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to compare tuple cells. rc=%s", strrc(rc));
    } else {
      cell.set_boolean(value);
    }
    return rc;
  }

  return RC::INVALID_ARGUMENT;
}

/**
 * @brief 获取比较表达式的值
 * @details 计算左右子表达式的值，然后对这两个值进行比较，返回布尔结果
 * @param[in] tuple 包含数据的元组，用于计算子表达式的值
 * @param[out] value 用于存储比较结果的布尔值
 * @return 操作结果状态码，成功返回RC::SUCCESS，计算子表达式失败返回相应错误码
 */
RC ComparisonExpr::get_value(const Tuple &tuple, Value &value) const
{
  Value left_value;
  Value right_value;

  // 计算左子表达式的值
  RC rc = left_->get_value(tuple, left_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }
  
  // 计算右子表达式的值
  rc = right_->get_value(tuple, right_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }

  // 比较两个值并设置结果
  bool bool_value = false;
  rc = compare_value(left_value, right_value, bool_value);
  if (rc == RC::SUCCESS) {
    value.set_boolean(bool_value);
  }
  return rc;
}

RC ComparisonExpr::eval(Chunk &chunk, vector<uint8_t> &select)
{
  RC     rc = RC::SUCCESS;
  Column left_column;
  Column right_column;

  rc = left_->get_column(chunk, left_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }
  rc = right_->get_column(chunk, right_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }
  if (left_column.attr_type() != right_column.attr_type()) {
    LOG_WARN("cannot compare columns with different types");
    return RC::INTERNAL;
  }
  if (left_column.attr_type() == AttrType::INTS) {
    rc = compare_column<int>(left_column, right_column, select);
  } else if (left_column.attr_type() == AttrType::FLOATS) {
    rc = compare_column<float>(left_column, right_column, select);
  } else {
    // TODO: support string compare
    LOG_WARN("unsupported data type %d", left_column.attr_type());
    return RC::INTERNAL;
  }
  return rc;
}

template <typename T>
RC ComparisonExpr::compare_column(const Column &left, const Column &right, vector<uint8_t> &result) const
{
  RC rc = RC::SUCCESS;

  bool left_const  = left.column_type() == Column::Type::CONSTANT_COLUMN;
  bool right_const = right.column_type() == Column::Type::CONSTANT_COLUMN;
  if (left_const && right_const) {
    compare_result<T, true, true>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  } else if (left_const && !right_const) {
    compare_result<T, true, false>((T *)left.data(), (T *)right.data(), right.count(), result, comp_);
  } else if (!left_const && right_const) {
    compare_result<T, false, true>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  } else {
    compare_result<T, false, false>((T *)left.data(), (T *)right.data(), left.count(), result, comp_);
  }
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 逻辑连接表达式的构造函数
 * @details 创建一个逻辑连接表达式（AND或OR），包含多个子表达式
 * @param[in] type 连接类型，可以是AND或OR
 * @param[in] children 子表达式列表，智能指针形式
 */
ConjunctionExpr::ConjunctionExpr(Type type, vector<unique_ptr<Expression>> &children)
    : conjunction_type_(type), children_(std::move(children))
{}

/**
 * @brief 获取逻辑连接表达式的值
 * @details 根据连接类型（AND/OR）计算所有子表达式的逻辑组合结果
 * @param[in] tuple 包含数据的元组，用于计算子表达式的值
 * @param[out] value 用于存储逻辑连接结果的布尔值
 * @return 操作结果状态码，成功返回RC::SUCCESS，计算子表达式失败返回相应错误码
 */
RC ConjunctionExpr::get_value(const Tuple &tuple, Value &value) const
{
  RC rc = RC::SUCCESS;
  
  // 空子表达式列表的默认值为true
  if (children_.empty()) {
    value.set_boolean(true);
    return rc;
  }

  Value tmp_value;
  for (const unique_ptr<Expression> &expr : children_) {
    // 计算当前子表达式的值
    rc = expr->get_value(tuple, tmp_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value by child expression. rc=%s", strrc(rc));
      return rc;
    }
    
    bool bool_value = tmp_value.get_boolean();
    
    // 短路优化：AND连接时遇到false直接返回false；OR连接时遇到true直接返回true
    if ((conjunction_type_ == Type::AND && !bool_value) || (conjunction_type_ == Type::OR && bool_value)) {
      value.set_boolean(bool_value);
      return rc;
    }
  }

  // 所有子表达式都已计算完成，根据连接类型设置默认结果
  // AND连接：所有子表达式都为true时返回true
  // OR连接：所有子表达式都为false时返回false
  bool default_value = (conjunction_type_ == Type::AND);
  value.set_boolean(default_value);
  return rc;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 算术表达式的构造函数（使用原始指针）
 * @details 创建一个算术表达式，包含算术操作符和左右子表达式
 * @param[in] type 算术操作符类型（加、减、乘、除、取负等）
 * @param[in] left 左子表达式指针
 * @param[in] right 右子表达式指针
 */
ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, Expression *left, Expression *right)
    : arithmetic_type_(type), left_(left), right_(right)
{}

/**
 * @brief 算术表达式的构造函数（使用智能指针）
 * @details 创建一个算术表达式，包含算术操作符和左右子表达式
 * @param[in] type 算术操作符类型（加、减、乘、除、取负等）
 * @param[in] left 左子表达式智能指针
 * @param[in] right 右子表达式智能指针
 */
ArithmeticExpr::ArithmeticExpr(ArithmeticExpr::Type type, unique_ptr<Expression> left, unique_ptr<Expression> right)
    : arithmetic_type_(type), left_(std::move(left)), right_(std::move(right))
{}

/**
 * @brief 比较两个算术表达式是否相等
 * @details 判断当前算术表达式是否与另一个表达式相等
 * @param[in] other 要比较的另一个表达式
 * @return 如果两个表达式类型相同、操作符相同且左右子表达式都相等，则返回true，否则返回false
 */
bool ArithmeticExpr::equal(const Expression &other) const
{
  // 自反性检查：同一个对象肯定相等
  if (this == &other) {
    return true;
  }
  
  // 类型检查：表达式类型不同则肯定不相等
  if (type() != other.type()) {
    return false;
  }
  
  // 比较操作符类型和子表达式
  auto &other_arith_expr = static_cast<const ArithmeticExpr &>(other);
  return arithmetic_type_ == other_arith_expr.arithmetic_type() && 
         left_->equal(*other_arith_expr.left_) &&
         right_->equal(*other_arith_expr.right_);
}

/**
 * @brief 获取算术表达式的返回值类型
 * @details 根据子表达式的类型和算术操作符确定返回值类型
 * @return 表达式的返回值类型（INTS或FLOATS）
 */
AttrType ArithmeticExpr::value_type() const
{
  // 对于一元操作（如取负），返回值类型与左操作数相同
  if (!right_) {
    return left_->value_type();
  }

  // 两个整数类型且不是除法操作，结果仍为整数
  if (left_->value_type() == AttrType::INTS && right_->value_type() == AttrType::INTS &&
      arithmetic_type_ != Type::DIV) {
    return AttrType::INTS;
  }

  // 其他情况返回浮点数类型
  return AttrType::FLOATS;
}

/**
 * @brief 计算算术表达式的值
 * @details 根据算术操作符对左右操作数执行相应的算术运算
 * @param[in] left_value 左操作数
 * @param[in] right_value 右操作数
 * @param[out] value 用于存储运算结果的值
 * @return 操作结果状态码，成功返回RC::SUCCESS，不支持的操作符返回RC::INTERNAL
 */
RC ArithmeticExpr::calc_value(const Value &left_value, const Value &right_value, Value &value) const
{
  RC rc = RC::SUCCESS;

  // 确定目标值类型
  const AttrType target_type = value_type();
  value.set_type(target_type);

  // 根据操作符类型执行相应的算术运算
  switch (arithmetic_type_) {
    case Type::ADD: {
      // 加法运算
      Value::add(left_value, right_value, value);
    } break;

    case Type::SUB: {
      // 减法运算
      Value::subtract(left_value, right_value, value);
    } break;

    case Type::MUL: {
      // 乘法运算
      Value::multiply(left_value, right_value, value);
    } break;

    case Type::DIV: {
      // 除法运算
      Value::divide(left_value, right_value, value);
    } break;

    case Type::NEGATIVE: {
      // 取负运算（一元操作，只使用左操作数）
      Value::negative(left_value, value);
    } break;

    default: {
      rc = RC::INTERNAL;
      LOG_WARN("unsupported arithmetic type. %d", arithmetic_type_);
    } break;
  }

  return rc;
}

/**
 * @brief 执行批量算术计算
 * @details 对列数据执行批量算术运算，支持常量列和非常量列的组合
 * @tparam LEFT_CONSTANT 左操作数是否为常量列
 * @tparam RIGHT_CONSTANT 右操作数是否为常量列
 * @param[in] left 左操作数列
 * @param[in] right 右操作数列
 * @param[out] result 存储计算结果的列
 * @param[in] type 算术操作类型
 * @param[in] attr_type 数据类型
 * @return 操作结果状态码，成功返回RC::SUCCESS，不支持的数据类型返回RC::UNIMPLEMENTED
 */
template <bool LEFT_CONSTANT, bool RIGHT_CONSTANT>
RC ArithmeticExpr::execute_calc(
    const Column &left, const Column &right, Column &result, Type type, AttrType attr_type) const
{
  RC rc = RC::SUCCESS;
  
  // 根据算术操作类型执行相应的批量计算
  switch (type) {
    case Type::ADD: {
      // 加法运算
      if (attr_type == AttrType::INTS) {
        // 整数类型的加法
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, AddOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        // 浮点数类型的加法
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, AddOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
    } break;
    case Type::SUB: {
      // 减法运算
      if (attr_type == AttrType::INTS) {
        // 整数类型的减法
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, SubtractOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        // 浮点数类型的减法
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, SubtractOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
    } break;
      break;
    case Type::MUL:
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, MultiplyOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, MultiplyOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    case Type::DIV:
      if (attr_type == AttrType::INTS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, int, DivideOperator>(
            (int *)left.data(), (int *)right.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        binary_operator<LEFT_CONSTANT, RIGHT_CONSTANT, float, DivideOperator>(
            (float *)left.data(), (float *)right.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    case Type::NEGATIVE:
      if (attr_type == AttrType::INTS) {
        unary_operator<LEFT_CONSTANT, int, NegateOperator>((int *)left.data(), (int *)result.data(), result.capacity());
      } else if (attr_type == AttrType::FLOATS) {
        unary_operator<LEFT_CONSTANT, float, NegateOperator>(
            (float *)left.data(), (float *)result.data(), result.capacity());
      } else {
        rc = RC::UNIMPLEMENTED;
      }
      break;
    default: rc = RC::UNIMPLEMENTED; break;
  }
  if (rc == RC::SUCCESS) {
    result.set_count(result.capacity());
  }
  return rc;
}

/**
 * @brief 获取算术表达式的值
 * @details 计算左右子表达式的值，然后根据算术操作符执行相应的计算
 * @param[in] tuple 包含数据的元组，用于计算子表达式的值
 * @param[out] value 用于存储算术计算结果的值
 * @return 操作结果状态码，成功返回RC::SUCCESS，计算子表达式失败返回相应错误码
 */
RC ArithmeticExpr::get_value(const Tuple &tuple, Value &value) const
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  // 计算左子表达式的值
  rc = left_->get_value(tuple, left_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }
  
  // 计算右子表达式的值
  rc = right_->get_value(tuple, right_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
    return rc;
  }
  
  // 执行算术运算并返回结果
  return calc_value(left_value, right_value, value);
}

RC ArithmeticExpr::get_column(Chunk &chunk, Column &column)
{
  RC rc = RC::SUCCESS;
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
    return rc;
  }
  Column left_column;
  Column right_column;

  rc = left_->get_column(chunk, left_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get column of left expression. rc=%s", strrc(rc));
    return rc;
  }
  rc = right_->get_column(chunk, right_column);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get column of right expression. rc=%s", strrc(rc));
    return rc;
  }
  return calc_column(left_column, right_column, column);
}

RC ArithmeticExpr::calc_column(const Column &left_column, const Column &right_column, Column &column) const
{
  RC rc = RC::SUCCESS;

  const AttrType target_type = value_type();
  column.init(target_type, left_column.attr_len(), max(left_column.count(), right_column.count()));
  bool left_const  = left_column.column_type() == Column::Type::CONSTANT_COLUMN;
  bool right_const = right_column.column_type() == Column::Type::CONSTANT_COLUMN;
  if (left_const && right_const) {
    column.set_column_type(Column::Type::CONSTANT_COLUMN);
    rc = execute_calc<true, true>(left_column, right_column, column, arithmetic_type_, target_type);
  } else if (left_const && !right_const) {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<true, false>(left_column, right_column, column, arithmetic_type_, target_type);
  } else if (!left_const && right_const) {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<false, true>(left_column, right_column, column, arithmetic_type_, target_type);
  } else {
    column.set_column_type(Column::Type::NORMAL_COLUMN);
    rc = execute_calc<false, false>(left_column, right_column, column, arithmetic_type_, target_type);
  }
  return rc;
}

RC ArithmeticExpr::try_get_value(Value &value) const
{
  RC rc = RC::SUCCESS;

  Value left_value;
  Value right_value;

  rc = left_->try_get_value(left_value);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to get value of left expression. rc=%s", strrc(rc));
    return rc;
  }

  if (right_) {
    rc = right_->try_get_value(right_value);
    if (rc != RC::SUCCESS) {
      LOG_WARN("failed to get value of right expression. rc=%s", strrc(rc));
      return rc;
    }
  }

  return calc_value(left_value, right_value, value);
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 未绑定聚合表达式的构造函数（使用原始指针）
 * @details 创建一个未绑定的聚合表达式，包含聚合函数名和子表达式
 * @param[in] aggregate_name 聚合函数名称字符串
 * @param[in] child 子表达式指针
 */
UnboundAggregateExpr::UnboundAggregateExpr(const char *aggregate_name, Expression *child)
    : aggregate_name_(aggregate_name), child_(child)
{}

/**
 * @brief 未绑定聚合表达式的构造函数（使用智能指针）
 * @details 创建一个未绑定的聚合表达式，包含聚合函数名和子表达式
 * @param[in] aggregate_name 聚合函数名称字符串
 * @param[in] child 子表达式智能指针
 */
UnboundAggregateExpr::UnboundAggregateExpr(const char *aggregate_name, unique_ptr<Expression> child)
    : aggregate_name_(aggregate_name), child_(std::move(child))
{}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 聚合表达式的构造函数（使用原始指针）
 * @details 创建一个聚合表达式，包含聚合类型和子表达式
 * @param[in] type 聚合类型（如SUM、COUNT等）
 * @param[in] child 子表达式指针
 */
AggregateExpr::AggregateExpr(Type type, Expression *child) : aggregate_type_(type), child_(child) {}

/**
 * @brief 聚合表达式的构造函数（使用智能指针）
 * @details 创建一个聚合表达式，包含聚合类型和子表达式
 * @param[in] type 聚合类型（如SUM、COUNT等）
 * @param[in] child 子表达式智能指针
 */
AggregateExpr::AggregateExpr(Type type, unique_ptr<Expression> child) : aggregate_type_(type), child_(std::move(child))
{}

/**
 * @brief 获取聚合表达式对应的列
 * @details 从数据块中获取聚合表达式对应的列数据
 * @param[in] chunk 数据块对象
 * @param[out] column 用于存储获取到的列的引用
 * @return 操作结果状态码，如果位置有效返回RC::SUCCESS，否则返回RC::INTERNAL
 */
RC AggregateExpr::get_column(Chunk &chunk, Column &column)
{
  RC rc = RC::SUCCESS;
  // 如果位置有效，引用对应的列
  if (pos_ != -1) {
    column.reference(chunk.column(pos_));
  } else {
    // 位置无效，返回内部错误
    rc = RC::INTERNAL;
  }
  return rc;
}

/**
 * @brief 比较两个聚合表达式是否相等
 * @details 判断当前聚合表达式是否与另一个表达式相等
 * @param[in] other 要比较的另一个表达式
 * @return 如果两个表达式类型相同、聚合类型相同且子表达式相等，则返回true，否则返回false
 */
bool AggregateExpr::equal(const Expression &other) const
{
  // 自反性检查：同一个对象肯定相等
  if (this == &other) {
    return true;
  }
  
  // 类型检查：表达式类型不同则肯定不相等
  if (other.type() != type()) {
    return false;
  }
  
  // 比较聚合类型和子表达式
  const AggregateExpr &other_aggr_expr = static_cast<const AggregateExpr &>(other);
  return aggregate_type_ == other_aggr_expr.aggregate_type() && child_->equal(*other_aggr_expr.child());
}

/**
 * @brief 创建聚合计算器
 * @details 根据聚合类型创建对应的聚合计算器实例
 * @return 聚合计算器的智能指针，根据聚合类型返回不同的实现
 */
unique_ptr<Aggregator> AggregateExpr::create_aggregator() const
{
  unique_ptr<Aggregator> aggregator;
  
  // 根据聚合类型创建对应的聚合计算器
  switch (aggregate_type_) {
    case Type::SUM: {
      // 创建求和聚合计算器
      aggregator = make_unique<SumAggregator>();
      break;
    }
    default: {
      // 不支持的聚合类型，触发断言失败
      ASSERT(false, "unsupported aggregate type");
      break;
    }
  }
  
  return aggregator;
}

/**
 * @brief 获取聚合表达式的值
 * @details 从元组中查找聚合表达式对应的值
 * @param[in] tuple 包含聚合结果的元组
 * @param[out] value 用于存储查找到的值
 * @return 操作结果状态码，成功返回RC::SUCCESS，失败返回相应错误码
 */
RC AggregateExpr::get_value(const Tuple &tuple, Value &value) const
{
  // 根据表达式名称在元组中查找对应的单元格值
  return tuple.find_cell(TupleCellSpec(name()), value);
}

RC AggregateExpr::type_from_string(const char *type_str, AggregateExpr::Type &type)
{
  RC rc = RC::SUCCESS;
  if (0 == strcasecmp(type_str, "count")) {
    type = Type::COUNT;
  } else if (0 == strcasecmp(type_str, "sum")) {
    type = Type::SUM;
  } else if (0 == strcasecmp(type_str, "avg")) {
    type = Type::AVG;
  } else if (0 == strcasecmp(type_str, "max")) {
    type = Type::MAX;
  } else if (0 == strcasecmp(type_str, "min")) {
    type = Type::MIN;
  } else {
    rc = RC::INVALID_ARGUMENT;
  }
  return rc;
}
