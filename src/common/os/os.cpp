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
// Created by Longda on 2010.
//

#include <execinfo.h>

#include "common/defs.h"
#include "common/log/log.h"
#include "common/os/os.h"
#include "common/lang/thread.h"

/**
 * @file os.cpp
 * @brief 操作系统相关功能的实现
 * 
 * 该文件实现了os.h中定义的操作系统相关功能接口，包括获取CPU核心数、
 * 打印堆栈跟踪信息等功能的具体实现。
 */
namespace common {
/**
 * @brief 获取当前系统的CPU核心数量实现
 * 
 * 该函数通过调用thread::hardware_concurrency()获取当前系统的CPU核心数量。
 * 注意：当前实现不考虑Windows平台。
 * 
 * @return uint32_t CPU核心数量
 */
uint32_t getCpuNum() { 
  // 不考虑Windows平台
  return thread::hardware_concurrency(); 
}

/**
 * @brief 最大堆栈帧数宏定义
 * 
 * 定义了堆栈跟踪时最多获取的堆栈帧数，用于限制堆栈跟踪的深度。
 */
#define MAX_STACK_SIZE 32

/**
 * @brief 打印当前线程的堆栈跟踪信息实现
 * 
 * 该函数使用系统的backtrace和backtrace_symbols函数获取当前线程的调用堆栈信息，
 * 并将其打印到日志系统中。堆栈信息包括函数调用链，最多打印MAX_STACK_SIZE个堆栈帧。
 * 
 * 实现步骤：
 * 1. 分配一个足够大的数组来存储堆栈帧地址
 * 2. 调用backtrace函数获取当前线程的堆栈帧地址
 * 3. 调用backtrace_symbols函数将地址转换为可读的符号信息
 * 4. 将符号信息打印到日志系统中
 * 5. 释放backtrace_symbols函数分配的内存
 */
void print_stacktrace()
{
  int    size = MAX_STACK_SIZE;               // 堆栈跟踪的最大深度
  void  *array[MAX_STACK_SIZE];               // 用于存储堆栈帧地址的数组
  int    stack_num = backtrace(array, size);  // 获取堆栈帧数量
  
  // 将堆栈帧地址转换为可读的符号信息
  char **stacktrace = backtrace_symbols(array, stack_num);
  
  // 打印每个堆栈帧的信息到日志系统
  for (int i = 0; i < stack_num; ++i) {
    LOG_INFO("%d ----- %s\n", i, stacktrace[i]);
  }
  
  // 释放backtrace_symbols分配的内存
  free(stacktrace);
}

}  // namespace common