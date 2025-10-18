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

#pragma once

#include "common/sys/rc.h"

class SQLStageEvent;

/**
 * @brief SQL解析器的Resolve阶段类
 * 
 * ResolveStage负责将解析后的SQL语句（ParsedSqlNode）转换为可执行的语句对象（Stmt），
 * 同时进行语义检查、表和字段的绑定、权限验证等工作。这是SQL执行流程中连接解析和优化执行的关键阶段。
 * 
 * @ingroup SQLStage
 */
class ResolveStage
{
public:
  /**
   * @brief 处理SQL解析阶段后的请求
   * 
   * 该方法是ResolveStage的核心入口，负责将解析后的SQL节点转换为可执行的语句对象
   * 
   * @param[in,out] sql_event SQL事件对象，包含解析后的SQL节点和会话信息
   * @return RC 处理结果状态码
   * @retval RC::SUCCESS 转换成功
   * @retval RC::SCHEMA_DB_NOT_EXIST 当前数据库不存在
   * @retval 其他错误码 转换失败
   */
  RC handle_request(SQLStageEvent *sql_event);
};
