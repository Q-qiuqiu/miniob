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
// Created by Meiyi
//

#include "sql/parser/parse.h"
#include "common/log/log.h"
#include "sql/expr/expression.h"

/**
 * @brief 解析单个SQL节点的内部函数声明
 * 
 * 这是一个内部函数，用于将SQL语句解析为单个SQL节点
 * 
 * @param[in] st SQL语句字符串
 * @param[out] sqln 解析得到的SQL节点
 * @return RC 解析结果状态码
 */
RC parse(char *st, ParsedSqlNode *sqln);

/**
 * @brief ParsedSqlNode类的默认构造函数
 * 
 * 初始化SQL节点的标志为错误状态
 */
ParsedSqlNode::ParsedSqlNode() : flag(SCF_ERROR) {}

/**
 * @brief ParsedSqlNode类的带参构造函数
 * 
 * 使用指定的SQL命令标志初始化SQL节点
 * 
 * @param[in] _flag SQL命令类型标志
 */
ParsedSqlNode::ParsedSqlNode(SqlCommandFlag _flag) : flag(_flag) {}

/**
 * @brief 向解析结果中添加SQL节点
 * 
 * 将解析得到的SQL节点添加到结果集合中
 * 
 * @param[in] sql_node 待添加的SQL节点（使用unique_ptr管理）
 */
void ParsedSqlResult::add_sql_node(unique_ptr<ParsedSqlNode> sql_node)
{
  sql_nodes_.emplace_back(std::move(sql_node));
}

////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 内部SQL解析函数声明
 * 
 * 这是由yacc/bison生成的实际解析函数，由parse函数调用
 * 
 * @param[in] st SQL语句字符串
 * @param[out] sql_result 解析结果对象
 * @return int 解析结果状态码
 */
int sql_parse(const char *st, ParsedSqlResult *sql_result);

/**
 * @brief SQL解析器的主要入口函数实现
 * 
 * 该函数是SQL解析器的公共接口，负责调用内部的sql_parse函数执行实际的解析工作
 * 
 * @param[in] st SQL语句字符串
 * @param[out] sql_result 解析结果对象，用于存储解析得到的SQL节点
 * @return RC 解析结果状态码
 * @retval RC::SUCCESS 解析成功
 */
RC parse(const char *st, ParsedSqlResult *sql_result)
{
  sql_parse(st, sql_result);
  return RC::SUCCESS;
}
