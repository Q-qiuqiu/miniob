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

/**
 * @file os.h
 * @brief 操作系统相关功能的接口定义
 * 
 * 该文件定义了与操作系统交互的基础功能接口，包括获取CPU核心数、
 * 打印堆栈跟踪信息等功能，为系统提供底层操作系统相关的服务抽象。
 */
namespace common {

/**
 * @brief 获取当前系统的CPU核心数量
 * 
 * 该函数返回当前运行环境中的CPU物理核心数量，用于并行计算和线程池配置等场景。
 * 
 * @return uint32_t CPU核心数量
 */
uint32_t getCpuNum();

/**
 * @brief 打印当前线程的堆栈跟踪信息
 * 
 * 该函数用于调试目的，打印当前线程的调用堆栈信息到日志系统。
 * 堆栈信息包括函数调用链，可以帮助开发者定位程序运行过程中的问题。
 */
void print_stacktrace();

}  // namespace common
