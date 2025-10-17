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
// Created by Longda on 2021/4/13.
//

#include <string.h>

#include "parse_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "sql/parser/parse.h"

using namespace common;

/**
 * @brief SQL解析阶段的核心处理函数
 * 
 * 该函数是ParseStage类的主要方法，负责将SQL语句解析为结构化的SQL节点。
 * 它是SQL执行流程中的解析阶段的入口点，处理从SQL字符串到结构化表示的转换。
 * 
 * @param[in,out] sql_event SQL事件对象，包含待解析的SQL语句和会话信息
 * @return RC 处理结果状态码
 * @retval RC::SUCCESS 解析成功
 * @retval RC::INTERNAL 解析结果为空时的内部错误
 * @retval RC::SQL_SYNTAX SQL语法错误
 */
RC ParseStage::handle_request(SQLStageEvent *sql_event)
{
  RC rc = RC::SUCCESS;

  // 获取SQL结果对象，用于存储解析结果或错误信息
  SqlResult         *sql_result = sql_event->session_event()->sql_result();
  // 获取待解析的SQL语句字符串
  const string &sql        = sql_event->sql();

  // 创建解析结果对象，用于存储解析后的SQL节点
  ParsedSqlResult parsed_sql_result;

  // 调用SQL解析器对SQL语句进行解析
  parse(sql.c_str(), &parsed_sql_result);
  
  // 检查解析结果是否为空
  if (parsed_sql_result.sql_nodes().empty()) {
    // 解析结果为空时，设置成功状态码但返回内部错误
    sql_result->set_return_code(RC::SUCCESS);
    sql_result->set_state_string("");
    return RC::INTERNAL;
  }

  // 检查是否解析出多个SQL命令（当前只处理第一个）
  if (parsed_sql_result.sql_nodes().size() > 1) {
    LOG_WARN("got multi sql commands but only 1 will be handled");
  }

  // 提取第一个SQL节点
  unique_ptr<ParsedSqlNode> sql_node = std::move(parsed_sql_result.sql_nodes().front());
  
  // 检查SQL节点是否包含语法错误
  if (sql_node->flag == SCF_ERROR) {
    // 设置错误信息到事件中
    rc = RC::SQL_SYNTAX;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("Failed to parse sql");
    return rc;
  }

  // 将成功解析的SQL节点保存到事件对象中，供后续处理阶段使用
  sql_event->set_sql_node(std::move(sql_node));

  return RC::SUCCESS;
}
