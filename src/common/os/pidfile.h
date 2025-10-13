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
// Created by Longda on 2010
//

#pragma once

#include "common/lang/string.h"

/**
 * @file pidfile.h
 * @brief PID文件管理接口定义
 * 
 * 该文件定义了进程ID文件(PID file)的管理接口，包括创建、删除和获取PID文件路径等功能。
 * PID文件通常用于记录进程的ID号，便于系统管理和监控进程状态。
 */
namespace common {

//! Generates a PID file for the current component
/**
 * Gets the process ID (PID) of the calling process and writes a file
 * dervied from the input argument containing that value in a system
 * standard directory, e.g. /var/run/progName.pid
 *
 * @param[in] programName as basis for file to write
 * @return    0 for success, error otherwise
 */
int writePidFile(const char *progName);

//! Cleanup PID file for the current component
/**
 * @brief 删除当前进程的PID文件
 * 
 * 移除之前由writePidFile函数创建的PID文件，通常在程序正常退出时调用。
 */
void removePidFile(void);

/**
 * @brief 获取当前PID文件的路径
 * 
 * 返回当前进程PID文件的完整路径。该函数返回对内部静态字符串的引用，
 * 该字符串在setPidPath或writePidFile函数调用时被设置。
 * 
 * @return string& PID文件路径的字符串引用
 */
string &getPidPath();

}  // namespace common
