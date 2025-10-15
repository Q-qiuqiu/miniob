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
// Created by Wangyunlai on 2023/03/07.
//

#pragma once

#include "common/types.h"
#include <stdint.h>

/**
 * @brief 事务ID的类型定义
 */
using TrxID = int32_t;

/**
 * @brief 表示无效的页面编号
 */
static constexpr PageNum BP_INVALID_PAGE_NUM = -1;

/**
 * @brief 表示文件头页面的编号，文件的第一个页面
 */
static constexpr PageNum BP_HEADER_PAGE = 0;

/**
 * @brief 页面的总大小，1 << 13 = 8KB
 */
static constexpr const int BP_PAGE_SIZE      = (1 << 13);

/**
 * @brief 页面中数据部分的大小，总大小减去LSN和校验和占用的空间
 */
static constexpr const int BP_PAGE_DATA_SIZE = (BP_PAGE_SIZE - sizeof(LSN) - sizeof(CheckSum));

/**
 * @brief 表示一个页面，可能放在内存或磁盘上
 * @ingroup BufferPool
 * 
 * 页面是存储引擎中基本的存储单位，用于存储表数据、索引数据等。
 * 每个页面包含页面头信息（LSN和校验和）和实际数据部分。
 */
struct Page
{
  /**
   * @brief 日志序列号，用于崩溃恢复时确定页面的最新版本
   */
  LSN      lsn;
  
  /**
   * @brief 页面数据的校验和，用于检测数据损坏
   */
  CheckSum check_sum;
  
  /**
   * @brief 页面的实际数据部分
   */
  char     data[BP_PAGE_DATA_SIZE];
};
