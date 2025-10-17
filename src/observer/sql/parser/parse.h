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

#pragma once

#include "common/sys/rc.h"
#include "sql/parser/parse_defs.h"

/**
 * @brief SQL解析器的对外接口函数
 * 
 * 该函数是SQL解析器的主要入口点，用于将SQL语句文本解析为结构化的SQL节点对象
 * 
 * @param[in] st SQL语句字符串
 * @param[out] sql_result 解析结果对象，用于存储解析得到的SQL节点
 * @return RC 解析结果状态码
 * @retval RC::SUCCESS 解析成功
 * @retval 其他错误码 解析失败
 */
RC parse(const char *st, ParsedSqlResult *sql_result);
