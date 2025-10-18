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

#include "resolve_stage.h"

#include "common/conf/ini.h"
#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "event/session_event.h"
#include "event/sql_event.h"
#include "session/session.h"
#include "sql/stmt/stmt.h"

using namespace common;

/**
 * @brief ResolveStage类的核心处理方法实现
 * 
 * 该方法实现了SQL解析节点到可执行语句对象的转换过程，主要步骤包括：
 * 1. 获取会话信息和当前数据库
 * 2. 检查数据库是否已选择
 * 3. 获取解析后的SQL节点
 * 4. 创建对应的语句对象
 * 5. 将创建的语句对象保存到SQL事件中
 * 
 * @param[in,out] sql_event SQL事件对象，包含解析后的SQL节点和会话信息
 * @return RC 处理结果状态码
 * @retval RC::SUCCESS 转换成功
 * @retval RC::SCHEMA_DB_NOT_EXIST 当前数据库不存在
 * @retval 其他错误码 转换失败
 */
RC ResolveStage::handle_request(SQLStageEvent *sql_event)
{
  // 初始化返回状态码为成功
  RC            rc            = RC::SUCCESS;
  // 获取会话事件对象
  SessionEvent *session_event = sql_event->session_event();
  // 获取SQL结果对象，用于存储错误信息
  SqlResult    *sql_result    = session_event->sql_result();

  // 获取当前会话的数据库对象
  Db *db = session_event->session()->get_current_db();
  // 检查数据库是否已选择
  if (nullptr == db) {
    LOG_ERROR("cannot find current db");
    rc = RC::SCHEMA_DB_NOT_EXIST;
    sql_result->set_return_code(rc);
    sql_result->set_state_string("no db selected");
    return rc;
  }

  // 获取解析后的SQL节点
  ParsedSqlNode *sql_node = sql_event->sql_node().get();
  // 定义语句对象指针，后续将创建具体的语句实现
  Stmt          *stmt     = nullptr;

  // 调用Stmt工厂方法创建具体的语句对象
  rc = Stmt::create_stmt(db, *sql_node, stmt);
  // 检查创建结果，如果失败且不是未实现的功能，则设置错误信息
  if (rc != RC::SUCCESS && rc != RC::UNIMPLEMENTED) {
    LOG_WARN("failed to create stmt. rc=%d:%s", rc, strrc(rc));
    sql_result->set_return_code(rc);
    return rc;
  }

  // 将创建的语句对象保存到SQL事件中，供后续执行阶段使用
  sql_event->set_stmt(stmt);

  return rc;
}
