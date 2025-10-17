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

/**
 * @file parse_defs.h
 * @brief SQL解析器核心数据结构定义
 * @details 该文件定义了SQL解析过程中使用的所有核心数据结构，包括各种SQL语句的解析结果表示、
 *          表达式、条件、属性等。这些结构构成了SQL解析器与执行引擎之间的数据交换接口。
 */

#pragma once

#include "common/lang/string.h"
#include "common/lang/vector.h"
#include "common/lang/memory.h"
#include "common/value.h"
#include "common/lang/utility.h"

/**
 * @brief Expression类前置声明
 * @details 表达式基类，用于表示SQL中的各种表达式（算术表达式、属性引用、常量等）
 */
class Expression;

/**
 * @defgroup SQLParser SQL解析器模块
 * @brief SQL解析器相关的数据结构、函数和接口定义
 */

/**
 * @brief 关系属性SQL节点
 * @ingroup SQLParser
 * @details 描述一个关系属性(表字段)，可以包含表名和属性名
 * @note "Rel" 是 "Relation"(关系/表)的缩写，"Attr" 是 "Attribute"(属性/字段)的缩写
 */
struct RelAttrSqlNode
{
  string relation_name;   ///< 关系名(表名)，可能为空，表示不指定表名
  string attribute_name;  ///< 属性名(字段名)
};

/**
 * @brief 比较运算符枚举
 * @ingroup SQLParser
 * @details 定义SQL中支持的所有比较操作符
 */
enum CompOp
{
  EQUAL_TO,     ///< 等于运算符 "="
  LESS_EQUAL,   ///< 小于等于运算符 "<="
  NOT_EQUAL,    ///< 不等于运算符 "<>"
  LESS_THAN,    ///< 小于运算符 "<"
  GREAT_EQUAL,  ///< 大于等于运算符 ">="
  GREAT_THAN,   ///< 大于运算符 ">"
  NO_OP         ///< 无效操作符，表示不需要比较
};

/**
 * @brief 条件比较SQL节点
 * @ingroup SQLParser
 * @details 表示SQL查询中WHERE子句中的一个条件表达式（如 "a > b"）
 * @note 条件比较由左右两边的操作数和一个比较运算符组成。每个操作数可以是字段引用或常量值。
 */
struct ConditionSqlNode
{
  int left_is_attr;              ///< 左操作数是否为属性(字段)：1表示是属性，0表示是常量值
  Value          left_value;     ///< 当left_is_attr=0时，左操作数的常量值
  RelAttrSqlNode left_attr;      ///< 当left_is_attr=1时，左操作数的属性信息
  CompOp         comp;           ///< 比较运算符
  int            right_is_attr;  ///< 右操作数是否为属性(字段)：1表示是属性，0表示是常量值
  RelAttrSqlNode right_attr;     ///< 当right_is_attr=1时，右操作数的属性信息
  Value          right_value;    ///< 当right_is_attr=0时，右操作数的常量值
};

/**
 * @brief SELECT语句SQL节点
 * @ingroup SQLParser
 * @details 表示一个SELECT查询语句的解析结果。
 * @details 一个正常的select语句描述起来比这个要复杂很多，这里做了简化。
 * 一个select语句由三部分组成，分别是select, from, where。
 * select部分表示要查询的字段，from部分表示要查询的表，where部分表示查询的条件。
 * 比如 from 中可以是多个表，也可以是另一个查询语句，这里仅仅支持表，也就是 relations。
 * where 条件 conditions，这里表示使用AND串联起来多个条件。正常的SQL语句会有OR，NOT等，
 * 甚至可以包含复杂的表达式。
 * @note 该结构是标准SQL SELECT语句的简化版本，主要包含四个部分：
 *       1. expressions: 要查询的表达式列表（字段、计算表达式等）
 *       2. relations: 要查询的表名列表
 *       3. conditions: 查询条件列表，目前仅支持使用AND连接的条件
 *       4. group_by: GROUP BY子句中的分组表达式列表
 */
struct SelectSqlNode
{
  vector<unique_ptr<Expression>> expressions;  ///< 查询表达式列表，可以是字段引用、函数调用等
  vector<string>                 relations;    ///< 查询涉及的表名列表
  vector<ConditionSqlNode>       conditions;   ///< 查询条件列表，多个条件使用AND连接
  vector<unique_ptr<Expression>> group_by;     ///< GROUP BY子句中的分组表达式列表
};

/**
 * @brief 计算表达式SQL节点
 * @ingroup SQLParser
 * @details 表示CALC语句的解析结果，用于计算算术表达式的值
 */
struct CalcSqlNode
{
  vector<unique_ptr<Expression>> expressions;  ///< 待计算的表达式列表
};

/**
 * @brief INSERT语句SQL节点
 * @ingroup SQLParser
 * @details 表示INSERT INTO语句的解析结果
 * @note 这是INSERT语句的简化版本，只支持全字段插入（不支持指定部分字段）
 */
struct InsertSqlNode
{
  string        relation_name;  ///< 要插入数据的表名
  vector<Value> values;         ///< 要插入的字段值列表，顺序必须与表定义一致
};

/**
 * @brief DELETE语句SQL节点
 * @ingroup SQLParser
 * @details 表示DELETE FROM语句的解析结果
 */
struct DeleteSqlNode
{
  string                   relation_name;  ///< 要删除数据的表名
  vector<ConditionSqlNode> conditions;     ///< 删除条件列表，满足条件的记录将被删除
};

/**
 * @brief UPDATE语句SQL节点
 * @ingroup SQLParser
 * @details 表示UPDATE语句的解析结果
 * @note 当前仅支持单字段更新
 */
struct UpdateSqlNode
{
  string                   relation_name;   ///< 要更新数据的表名
  string                   attribute_name;  ///< 要更新的字段名
  Value                    value;           ///< 更新后的新值
  vector<ConditionSqlNode> conditions;      ///< 更新条件列表，满足条件的记录将被更新
};

/**
 * @brief 属性信息SQL节点
 * @ingroup SQLParser
 * @details 描述表中的一个属性（字段）的定义信息
 */
struct AttrInfoSqlNode
{
  AttrType type;    ///< 属性数据类型（如INT、FLOAT、STRING等）
  string   name;    ///< 属性名称（字段名）
  size_t   length;  ///< 属性长度，主要用于字符串类型
};

/**
 * @brief CREATE TABLE语句SQL节点
 * @ingroup SQLParser
 * @details 表示CREATE TABLE语句的解析结果
 * @note 这是CREATE TABLE语句的简化版本，支持指定表名、属性列表、主键和存储选项
 */
struct CreateTableSqlNode
{
  string                  relation_name;  ///< 要创建的表名
  vector<AttrInfoSqlNode> attr_infos;     ///< 表的属性定义列表
  vector<string>          primary_keys;   ///< 主键字段名列表
  // TODO: integrate to CreateTableOptions
  string storage_format;  ///< 存储格式
  string storage_engine;  ///< 存储引擎类型
};

/**
 * @brief DROP TABLE语句SQL节点
 * @ingroup SQLParser
 * @details 表示DROP TABLE语句的解析结果
 */
struct DropTableSqlNode
{
  string relation_name;  ///< 要删除的表名
};

/**
 * @brief ANALYZE TABLE语句SQL节点
 * @ingroup SQLParser
 * @details 表示ANALYZE TABLE语句的解析结果，用于收集表的统计信息
 */
struct AnalyzeTableSqlNode
{
  string relation_name;  ///< 要分析的表名
};

/**
 * @brief CREATE INDEX语句SQL节点
 * @ingroup SQLParser
 * @details 表示CREATE INDEX语句的解析结果
 * @note 当前仅支持为单个字段创建索引
 */
struct CreateIndexSqlNode
{
  string index_name;      ///< 索引名称
  string relation_name;   ///< 要创建索引的表名
  string attribute_name;  ///< 要建立索引的字段名
};

/**
 * @brief DROP INDEX语句SQL节点
 * @ingroup SQLParser
 * @details 表示DROP INDEX语句的解析结果
 */
struct DropIndexSqlNode
{
  string index_name;     ///< 要删除的索引名称
  string relation_name;  ///< 索引所属的表名
};

/**
 * @brief DESC TABLE语句SQL节点
 * @ingroup SQLParser
 * @details 表示DESC/DESCRIBE TABLE语句的解析结果，用于查询表的结构信息
 */
struct DescTableSqlNode
{
  string relation_name;  ///< 要查询结构的表名
};

/**
 * @brief LOAD DATA语句SQL节点
 * @ingroup SQLParser
 * @details 表示LOAD DATA语句的解析结果，用于从文件导入数据到表中
 * @note 文件格式要求：每行一条记录，字段数和数据类型必须与目标表定义一致
 */
struct LoadDataSqlNode
{
  string relation_name;  ///< 目标表名
  string file_name;      ///< 数据文件名
};

/**
 * @brief SET VARIABLE语句SQL节点
 * @ingroup SQLParser
 * @details 表示SET语句的解析结果，用于设置系统或会话变量的值
 * @note 目前仅支持设置变量，不支持查询变量
 */
struct SetVariableSqlNode
{
  string name;  ///< 变量名
  Value  value; ///< 变量的新值
};

/**
 * @brief 解析SQL节点类前置声明
 */
class ParsedSqlNode;

/**
 * @brief EXPLAIN语句SQL节点
 * @ingroup SQLParser
 * @details 会创建operator的语句，才能用explain输出执行计划。
 * 一个command就是一个语句，比如select语句，insert语句等。
 * 可能改成SqlCommand更合适。
 * @details 表示EXPLAIN语句的解析结果，用于获取SQL语句的执行计划
 * @note 只有会生成执行计划的语句（如SELECT、INSERT等）才能使用EXPLAIN
 */
struct ExplainSqlNode
{
  unique_ptr<ParsedSqlNode> sql_node;  ///< 要解释的SQL语句节点
};

/**
 * @brief 错误SQL节点
 * @ingroup SQLParser
 * @details 表示SQL解析过程中出现的错误信息
 * @note 目前行号和列号信息可能不准确或未设置
 */
struct ErrorSqlNode
{
  string error_msg;  ///< 错误描述信息
  int    line;       ///< 错误发生的行号
  int    column;     ///< 错误发生的列号
};

/**
 * @brief SQL命令类型枚举
 * @ingroup SQLParser
 * @details 定义了系统支持的所有SQL语句类型
 */
enum SqlCommandFlag
{
  SCF_ERROR = 0,              ///< 错误类型
  SCF_CALC,                   ///< 计算表达式语句
  SCF_SELECT,                 ///< 查询语句
  SCF_INSERT,                 ///< 插入语句
  SCF_UPDATE,                 ///< 更新语句
  SCF_DELETE,                 ///< 删除语句
  SCF_CREATE_TABLE,           ///< 创建表语句
  SCF_DROP_TABLE,             ///< 删除表语句
  SCF_ANALYZE_TABLE,          ///< 分析表语句
  SCF_CREATE_INDEX,           ///< 创建索引语句
  SCF_DROP_INDEX,             ///< 删除索引语句
  SCF_SYNC,                   ///< 同步数据语句
  SCF_SHOW_TABLES,            ///< 显示表列表语句
  SCF_DESC_TABLE,             ///< 描述表结构语句
  SCF_BEGIN,                  ///< 事务开始语句，可以在这里扩展只读事务
  SCF_COMMIT,                 ///< 事务提交语句
  SCF_CLOG_SYNC,              ///< 日志同步语句
  SCF_ROLLBACK,               ///< 事务回滚语句
  SCF_LOAD_DATA,              ///< 加载数据语句
  SCF_HELP,                   ///< 帮助语句
  SCF_EXIT,                   ///< 退出语句
  SCF_EXPLAIN,                ///< 解释执行计划语句
  SCF_SET_VARIABLE,           ///< 设置变量语句
};

/**
 * @brief 解析SQL节点类
 * @ingroup SQLParser
 * @details 表示一个完整SQL语句的解析结果，包含了各种类型SQL语句的具体信息
 * @note 根据flag字段的值，可以访问对应的特定SQL语句结构体
 */
class ParsedSqlNode
{
public:
  enum SqlCommandFlag flag;           ///< SQL语句类型标志
  ErrorSqlNode        error;          ///< 错误信息（当flag为SCF_ERROR时有效）
  CalcSqlNode         calc;           ///< 计算表达式语句信息
  SelectSqlNode       selection;      ///< SELECT语句信息
  InsertSqlNode       insertion;      ///< INSERT语句信息
  DeleteSqlNode       deletion;       ///< DELETE语句信息
  UpdateSqlNode       update;         ///< UPDATE语句信息
  CreateTableSqlNode  create_table;   ///< CREATE TABLE语句信息
  DropTableSqlNode    drop_table;     ///< DROP TABLE语句信息
  AnalyzeTableSqlNode analyze_table;  ///< ANALYZE TABLE语句信息
  CreateIndexSqlNode  create_index;   ///< CREATE INDEX语句信息
  DropIndexSqlNode    drop_index;     ///< DROP INDEX语句信息
  DescTableSqlNode    desc_table;     ///< DESC TABLE语句信息
  LoadDataSqlNode     load_data;      ///< LOAD DATA语句信息
  ExplainSqlNode      explain;        ///< EXPLAIN语句信息
  SetVariableSqlNode  set_variable;   ///< SET VARIABLE语句信息

public:
  /**
   * @brief 默认构造函数
   */
  ParsedSqlNode();
  
  /**
   * @brief 带参数构造函数
   * @param flag SQL语句类型
   */
  explicit ParsedSqlNode(SqlCommandFlag flag);
};

/**
 * @brief 解析SQL结果类
 * @ingroup SQLParser
 * @details 表示SQL解析后的整体结果，包含一个或多个SQL语句的解析节点
 * @note 虽然数据结构设计上支持多个SQL语句，但目前系统只处理第一个语句
 */
class ParsedSqlResult
{
public:
  /**
   * @brief 添加一个SQL节点到结果中
   * @param sql_node 要添加的SQL节点（使用智能指针管理内存）
   */
  void add_sql_node(unique_ptr<ParsedSqlNode> sql_node);

  /**
   * @brief 获取SQL节点列表的引用
   * @return SQL节点列表的引用
   */
  vector<unique_ptr<ParsedSqlNode>> &sql_nodes() { return sql_nodes_; }

private:
  vector<unique_ptr<ParsedSqlNode>> sql_nodes_;  ///< SQL命令列表，目前只处理第一个
};

/** @} */ // SQLParser模块结束
