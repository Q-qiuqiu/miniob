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

#pragma once

#include "sql/expr/expression.h"

/**
 * @brief 表达式绑定上下文
 * @details 维护查询中涉及的表信息，为表达式绑定提供必要的上下文环境
 */
class BinderContext
{
public:
  /**
   * @brief 默认构造函数
   */
  BinderContext() = default;
  
  /**
   * @brief 默认析构函数
   */
  virtual ~BinderContext() = default;

  /**
   * @brief 添加表到查询上下文中
   * @param table 要添加的表指针
   */
  void add_table(Table *table) { query_tables_.push_back(table); }

  /**
   * @brief 根据表名查找表对象
   * @param table_name 表名（大小写不敏感）
   * @return 找到的表指针，如果未找到则返回nullptr
   */
  Table *find_table(const char *table_name) const;

  /**
   * @brief 获取查询中涉及的所有表
   * @return 表指针向量的常量引用
   */
  const vector<Table *> &query_tables() const { return query_tables_; }

private:
  /**
   * @brief 查询中涉及的表列表
   */
  vector<Table *> query_tables_;
};

/**
 * @brief 表达式绑定器
 * @details 负责将SQL解析后的表达式文本绑定到具体的数据库对象（如表、字段等）
 *          在SQL执行前，需要将解析阶段产生的表达式转换为可执行的表达式对象
 */
class ExpressionBinder
{
public:
  /**
   * @brief 构造函数
   * @param context 表达式绑定上下文
   */
  ExpressionBinder(BinderContext &context) : context_(context) {}
  
  /**
   * @brief 析构函数
   */
  virtual ~ExpressionBinder() = default;

  /**
   * @brief 绑定表达式的入口方法
   * @param expr 待绑定的表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_expression(unique_ptr<Expression> &expr, vector<unique_ptr<Expression>> &bound_expressions);

private:
  /**
   * @brief 绑定星号表达式（如*或table.*）
   * @param star_expr 星号表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_star_expression(unique_ptr<Expression> &star_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定未绑定字段表达式
   * @param unbound_field_expr 未绑定字段表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_unbound_field_expression(
      unique_ptr<Expression> &unbound_field_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定已绑定字段表达式
   * @param field_expr 已绑定字段表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_field_expression(unique_ptr<Expression> &field_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定值表达式
   * @param value_expr 值表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_value_expression(unique_ptr<Expression> &value_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定类型转换表达式
   * @param cast_expr 类型转换表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_cast_expression(unique_ptr<Expression> &cast_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定比较表达式（如=, >, <等）
   * @param comparison_expr 比较表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_comparison_expression(
      unique_ptr<Expression> &comparison_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定逻辑连接表达式（如AND, OR）
   * @param conjunction_expr 逻辑连接表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_conjunction_expression(
      unique_ptr<Expression> &conjunction_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定算术表达式（如+, -, *, /）
   * @param arithmetic_expr 算术表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_arithmetic_expression(
      unique_ptr<Expression> &arithmetic_expr, vector<unique_ptr<Expression>> &bound_expressions);
  
  /**
   * @brief 绑定聚合表达式（如SUM, AVG, COUNT等）
   * @param aggregate_expr 聚合表达式
   * @param bound_expressions 绑定后的表达式列表
   * @return 成功返回RC::SUCCESS，失败返回对应的错误码
   */
  RC bind_aggregate_expression(
      unique_ptr<Expression> &aggregate_expr, vector<unique_ptr<Expression>> &bound_expressions);

private:
  /**
   * @brief 表达式绑定上下文
   */
  BinderContext &context_;
};
