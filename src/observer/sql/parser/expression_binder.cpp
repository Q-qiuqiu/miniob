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

#include "common/log/log.h"
#include "common/lang/string.h"
#include "common/lang/ranges.h"
#include "sql/parser/expression_binder.h"
#include "sql/expr/expression_iterator.h"

using namespace common;

/**
 * @brief 在查询上下文中查找指定名称的表
 * @param table_name 要查找的表名
 * @return 找到的表指针，如果未找到则返回nullptr
 * @details 使用ranges::find_if和lambda表达式进行大小写不敏感的表名匹配
 */
Table *BinderContext::find_table(const char *table_name) const
{
  // 定义一个谓词函数，用于比较表名（大小写不敏感）
  auto pred = [table_name](Table *table) { return 0 == strcasecmp(table_name, table->name()); };
  // 使用ranges::find_if查找匹配的表
  auto iter = ranges::find_if(query_tables_, pred);
  if (iter == query_tables_.end()) {
    return nullptr;
  }
  return *iter;
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 处理星号表达式中的通配符字段，将表中的所有非系统字段添加到表达式列表
 * @param table 表对象
 * @param expressions 表达式列表，用于存储生成的字段表达式
 */
static void wildcard_fields(Table *table, vector<unique_ptr<Expression>> &expressions)
{
  // 获取表的元数据
  const TableMeta &table_meta = table->table_meta();
  const int        field_num  = table_meta.field_num();
  
  // 从系统字段数量开始遍历，只处理用户定义的字段
  for (int i = table_meta.sys_field_num(); i < field_num; i++) {
    // 创建字段对象和对应的字段表达式
    Field      field(table, table_meta.field(i));
    FieldExpr *field_expr = new FieldExpr(field);
    field_expr->set_name(field.field_name());
    expressions.emplace_back(field_expr);
  }
}

/**
 * @brief 绑定表达式的主入口方法
 * @param expr 待绑定的表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 根据表达式类型调用不同的绑定方法，实现表达式的绑定和转换
 */
RC ExpressionBinder::bind_expression(unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  // 处理空表达式情况
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 根据表达式类型调用对应的绑定方法
  switch (expr->type()) {
    case ExprType::STAR: {
      return bind_star_expression(expr, bound_expressions);
    } break;

    case ExprType::UNBOUND_FIELD: {
      return bind_unbound_field_expression(expr, bound_expressions);
    } break;

    case ExprType::UNBOUND_AGGREGATION: {
      return bind_aggregate_expression(expr, bound_expressions);
    } break;

    case ExprType::FIELD: {
      return bind_field_expression(expr, bound_expressions);
    } break;

    case ExprType::VALUE: {
      return bind_value_expression(expr, bound_expressions);
    } break;

    case ExprType::CAST: {
      return bind_cast_expression(expr, bound_expressions);
    } break;

    case ExprType::COMPARISON: {
      return bind_comparison_expression(expr, bound_expressions);
    } break;

    case ExprType::CONJUNCTION: {
      return bind_conjunction_expression(expr, bound_expressions);
    } break;

    case ExprType::ARITHMETIC: {
      return bind_arithmetic_expression(expr, bound_expressions);
    } break;

    case ExprType::AGGREGATION: {
      ASSERT(false, "shouldn't be here");
    } break;

    default: {
      LOG_WARN("unknown expression type: %d", static_cast<int>(expr->type()));
      return RC::INTERNAL;
    }
  }
  return RC::INTERNAL;
}

/**
 * @brief 绑定星号表达式（如SELECT * FROM table）
 * @param expr 星号表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 根据星号表达式是否指定表名，决定将哪些表的字段添加到表达式列表
 */
RC ExpressionBinder::bind_star_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为星号表达式
  auto star_expr = static_cast<StarExpr *>(expr.get());

  vector<Table *> tables_to_wildcard;

  // 获取星号表达式指定的表名
  const char *table_name = star_expr->table_name();
  if (!is_blank(table_name) && 0 != strcmp(table_name, "*")) {
    // 如果指定了具体表名，查找该表
    Table *table = context_.find_table(table_name);
    if (nullptr == table) {
      LOG_INFO("no such table in from list: %s", table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    tables_to_wildcard.push_back(table);
  } else {
    // 如果没有指定表名或表名为*，则使用查询中的所有表
    const vector<Table *> &all_tables = context_.query_tables();
    tables_to_wildcard.insert(tables_to_wildcard.end(), all_tables.begin(), all_tables.end());
  }

  // 为每个表生成字段表达式
  for (Table *table : tables_to_wildcard) {
    wildcard_fields(table, bound_expressions);
  }

  return RC::SUCCESS;
}

/**
 * @brief 绑定未绑定字段表达式（如SELECT column_name FROM table）
 * @param expr 未绑定字段表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 根据字段名和可选的表名，将字段表达式绑定到具体的表和字段
 */
RC ExpressionBinder::bind_unbound_field_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为未绑定字段表达式
  auto unbound_field_expr = static_cast<UnboundFieldExpr *>(expr.get());

  const char *table_name = unbound_field_expr->table_name();
  const char *field_name = unbound_field_expr->field_name();

  Table *table = nullptr;
  // 处理未指定表名的情况
  if (is_blank(table_name)) {
    // 如果查询中只有一个表，可以推断字段所属的表
    if (context_.query_tables().size() != 1) {
      LOG_INFO("cannot determine table for field: %s", field_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }

    table = context_.query_tables()[0];
  } else {
    // 根据指定的表名查找表
    table = context_.find_table(table_name);
    if (nullptr == table) {
      LOG_INFO("no such table in from list: %s", table_name);
      return RC::SCHEMA_TABLE_NOT_EXIST;
    }
  }

  // 处理字段名为*的情况
  if (0 == strcmp(field_name, "*")) {
    wildcard_fields(table, bound_expressions);
  } else {
    // 查找表中的字段元数据
    const FieldMeta *field_meta = table->table_meta().field(field_name);
    if (nullptr == field_meta) {
      LOG_INFO("no such field in table: %s.%s", table_name, field_name);
      return RC::SCHEMA_FIELD_MISSING;
    }

    // 创建字段表达式并添加到结果列表
    Field      field(table, field_meta);
    FieldExpr *field_expr = new FieldExpr(field);
    field_expr->set_name(field_name);
    bound_expressions.emplace_back(field_expr);
  }

  return RC::SUCCESS;
}

/**
 * @brief 绑定已绑定字段表达式
 * @param field_expr 已绑定字段表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS
 * @details 已绑定字段表达式直接添加到结果列表即可
 */
RC ExpressionBinder::bind_field_expression(
    unique_ptr<Expression> &field_expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  bound_expressions.emplace_back(std::move(field_expr));
  return RC::SUCCESS;
}

/**
 * @brief 绑定值表达式
 * @param value_expr 值表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS
 * @details 值表达式直接添加到结果列表即可
 */
RC ExpressionBinder::bind_value_expression(
    unique_ptr<Expression> &value_expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  bound_expressions.emplace_back(std::move(value_expr));
  return RC::SUCCESS;
}

/**
 * @brief 绑定类型转换表达式
 * @param expr 类型转换表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 递归绑定类型转换表达式的子表达式
 */
RC ExpressionBinder::bind_cast_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为类型转换表达式
  auto cast_expr = static_cast<CastExpr *>(expr.get());

  vector<unique_ptr<Expression>> child_bound_expressions;
  unique_ptr<Expression>        &child_expr = cast_expr->child();

  // 递归绑定子表达式
  RC rc = bind_expression(child_expr, child_bound_expressions);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  // 类型转换表达式只能有一个子表达式
  if (child_bound_expressions.size() != 1) {
    LOG_WARN("invalid children number of cast expression: %d", child_bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  // 更新子表达式指针
  unique_ptr<Expression> &child = child_bound_expressions[0];
  if (child.get() == child_expr.get()) {
    return RC::SUCCESS;
  }

  child_expr.reset(child.release());
  bound_expressions.emplace_back(std::move(expr));
  return RC::SUCCESS;
}

/**
 * @brief 绑定比较表达式（如=, >, <等）
 * @param expr 比较表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 递归绑定比较表达式的左右子表达式
 */
RC ExpressionBinder::bind_comparison_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为比较表达式
  auto comparison_expr = static_cast<ComparisonExpr *>(expr.get());

  vector<unique_ptr<Expression>> child_bound_expressions;
  unique_ptr<Expression>        &left_expr  = comparison_expr->left();
  unique_ptr<Expression>        &right_expr = comparison_expr->right();

  // 递归绑定左子表达式
  RC rc = bind_expression(left_expr, child_bound_expressions);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  // 比较表达式的左子表达式必须是单个表达式
  if (child_bound_expressions.size() != 1) {
    LOG_WARN("invalid left children number of comparison expression: %d", child_bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  // 更新左子表达式指针
  unique_ptr<Expression> &left = child_bound_expressions[0];
  if (left.get() != left_expr.get()) {
    left_expr.reset(left.release());
  }

  // 递归绑定右子表达式
  child_bound_expressions.clear();
  rc = bind_expression(right_expr, child_bound_expressions);
  if (rc != RC::SUCCESS) {
    return rc;
  }

  // 比较表达式的右子表达式必须是单个表达式
  if (child_bound_expressions.size() != 1) {
    LOG_WARN("invalid right children number of comparison expression: %d", child_bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  // 更新右子表达式指针
  unique_ptr<Expression> &right = child_bound_expressions[0];
  if (right.get() != right_expr.get()) {
    right_expr.reset(right.release());
  }

  bound_expressions.emplace_back(std::move(expr));
  return RC::SUCCESS;
}

/**
 * @brief 绑定逻辑连接表达式（如AND, OR）
 * @param expr 逻辑连接表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 递归绑定逻辑连接表达式的所有子表达式
 */
RC ExpressionBinder::bind_conjunction_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为逻辑连接表达式
  auto conjunction_expr = static_cast<ConjunctionExpr *>(expr.get());

  vector<unique_ptr<Expression>>  child_bound_expressions;
  vector<unique_ptr<Expression>> &children = conjunction_expr->children();

  // 递归绑定每个子表达式
  for (unique_ptr<Expression> &child_expr : children) {
    child_bound_expressions.clear();

    RC rc = bind_expression(child_expr, child_bound_expressions);
    if (rc != RC::SUCCESS) {
      return rc;
    }

    // 每个子表达式必须是单个表达式
    if (child_bound_expressions.size() != 1) {
      LOG_WARN("invalid children number of conjunction expression: %d", child_bound_expressions.size());
      return RC::INVALID_ARGUMENT;
    }

    // 更新子表达式指针
    unique_ptr<Expression> &child = child_bound_expressions[0];
    if (child.get() != child_expr.get()) {
      child_expr.reset(child.release());
    }
  }

  bound_expressions.emplace_back(std::move(expr));

  return RC::SUCCESS;
}

/**
 * @brief 绑定算术表达式（如+, -, *, /）
 * @param expr 算术表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 递归绑定算术表达式的左右子表达式
 */
RC ExpressionBinder::bind_arithmetic_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为算术表达式
  auto arithmetic_expr = static_cast<ArithmeticExpr *>(expr.get());

  vector<unique_ptr<Expression>> child_bound_expressions;
  unique_ptr<Expression>        &left_expr  = arithmetic_expr->left();
  unique_ptr<Expression>        &right_expr = arithmetic_expr->right();

  // 递归绑定左子表达式
  RC rc = bind_expression(left_expr, child_bound_expressions);
  if (OB_FAIL(rc)) {
    return rc;
  }

  // 算术表达式的左子表达式必须是单个表达式
  if (child_bound_expressions.size() != 1) {
    LOG_WARN("invalid left children number of comparison expression: %d", child_bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  // 更新左子表达式指针
  unique_ptr<Expression> &left = child_bound_expressions[0];
  if (left.get() != left_expr.get()) {
    left_expr.reset(left.release());
  }

  // 递归绑定右子表达式
  child_bound_expressions.clear();
  rc = bind_expression(right_expr, child_bound_expressions);
  if (OB_FAIL(rc)) {
    return rc;
  }

  // 算术表达式的右子表达式必须是单个表达式
  if (child_bound_expressions.size() != 1) {
    LOG_WARN("invalid right children number of comparison expression: %d", child_bound_expressions.size());
    return RC::INVALID_ARGUMENT;
  }

  // 更新右子表达式指针
  unique_ptr<Expression> &right = child_bound_expressions[0];
  if (right.get() != right_expr.get()) {
    right_expr.reset(right.release());
  }

  bound_expressions.emplace_back(std::move(expr));
  return RC::SUCCESS;
}

/**
 * @brief 检查聚合表达式的有效性
 * @param expression 聚合表达式
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 验证聚合表达式的子表达式存在性、数据类型兼容性，以及禁止聚合表达式嵌套
 */
RC check_aggregate_expression(AggregateExpr &expression)
{
  // 必须有一个子表达式
  Expression *child_expression = expression.child().get();
  if (nullptr == child_expression) {
    LOG_WARN("child expression of aggregate expression is null");
    return RC::INVALID_ARGUMENT;
  }

  // 校验数据类型与聚合类型是否匹配
  AggregateExpr::Type aggregate_type   = expression.aggregate_type();
  AttrType            child_value_type = child_expression->value_type();
  switch (aggregate_type) {
    case AggregateExpr::Type::SUM:
    case AggregateExpr::Type::AVG: {
      // SUM和AVG聚合函数仅支持数值类型
      if (child_value_type != AttrType::INTS && child_value_type != AttrType::FLOATS) {
        LOG_WARN("invalid child value type for aggregate expression: %d", static_cast<int>(child_value_type));
        return RC::INVALID_ARGUMENT;
      }
    } break;

    case AggregateExpr::Type::COUNT:
    case AggregateExpr::Type::MAX:
    case AggregateExpr::Type::MIN: {
      // COUNT、MAX、MIN聚合函数支持任何数据类型
    } break;
  }

  // 子表达式中不能再包含聚合表达式（禁止聚合函数嵌套）
  function<RC(unique_ptr<Expression>&)> check_aggregate_expr = [&](unique_ptr<Expression> &expr) -> RC {
    RC rc = RC::SUCCESS;
    if (expr->type() == ExprType::AGGREGATION) {
      LOG_WARN("aggregate expression cannot be nested");
      return RC::INVALID_ARGUMENT;
    }
    rc = ExpressionIterator::iterate_child_expr(*expr, check_aggregate_expr);
    return rc;
  };

  // 遍历并检查所有子表达式
  RC rc = ExpressionIterator::iterate_child_expr(expression, check_aggregate_expr);

  return rc;
}

/**
 * @brief 绑定聚合表达式（如SUM, AVG, COUNT等）
 * @param expr 未绑定聚合表达式
 * @param bound_expressions 绑定后的表达式列表
 * @return 成功返回RC::SUCCESS，失败返回对应的错误码
 * @details 将未绑定聚合表达式转换为已绑定的聚合表达式，并验证其有效性
 */
RC ExpressionBinder::bind_aggregate_expression(
    unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions)
{
  if (nullptr == expr) {
    return RC::SUCCESS;
  }

  // 转换为未绑定聚合表达式
  auto unbound_aggregate_expr = static_cast<UnboundAggregateExpr *>(expr.get());
  const char *aggregate_name = unbound_aggregate_expr->aggregate_name();
  AggregateExpr::Type aggregate_type;
  
  // 根据聚合函数名确定聚合类型
  RC rc = AggregateExpr::type_from_string(aggregate_name, aggregate_type);
  if (OB_FAIL(rc)) {
    LOG_WARN("invalid aggregate name: %s", aggregate_name);
    return rc;
  }

  unique_ptr<Expression>        &child_expr = unbound_aggregate_expr->child();
  vector<unique_ptr<Expression>> child_bound_expressions;

  // 特殊处理COUNT(*)
  if (child_expr->type() == ExprType::STAR && aggregate_type == AggregateExpr::Type::COUNT) {
    // COUNT(*) 优化为 COUNT(1)
    ValueExpr *value_expr = new ValueExpr(Value(1));
    child_expr.reset(value_expr);
  } else {
    // 递归绑定子表达式
    rc = bind_expression(child_expr, child_bound_expressions);
    if (OB_FAIL(rc)) {
      return rc;
    }

    // 聚合表达式的子表达式必须是单个表达式
    if (child_bound_expressions.size() != 1) {
      LOG_WARN("invalid children number of aggregate expression: %d", child_bound_expressions.size());
      return RC::INVALID_ARGUMENT;
    }

    // 更新子表达式指针
    if (child_bound_expressions[0].get() != child_expr.get()) {
      child_expr.reset(child_bound_expressions[0].release());
    }
  }

  // 创建已绑定的聚合表达式
  auto aggregate_expr = make_unique<AggregateExpr>(aggregate_type, std::move(child_expr));
  aggregate_expr->set_name(unbound_aggregate_expr->name());
  
  // 检查聚合表达式的有效性
  rc = check_aggregate_expression(*aggregate_expr);
  if (OB_FAIL(rc)) {
    return rc;
  }

  bound_expressions.emplace_back(std::move(aggregate_expr));
  return RC::SUCCESS;
}
