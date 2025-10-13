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

#include <errno.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * @file defs.h
 * @brief 定义系统级的通用常量、错误码和宏
 * 
 * 该文件包含了miniob项目中广泛使用的基础定义，包括状态码、
 * 跨平台线程ID获取、文件路径常量和浮点数比较精度等。
 * 这些定义被项目中多个模块共享，为系统提供统一的基础组件。
 */
namespace common {

/**
 * @brief 跨平台获取线程ID的宏定义
 * 
 * 该宏在不同操作系统平台上提供一致的线程ID获取方法。
 * 在MacOS（__MACH__）和Linux平台上，通过pthread_self()获取线程ID并转换为long long类型。
 */
#ifndef gettid
#if defined(__MACH__)
#define gettid() ((long long)pthread_self())
#elif defined(LINUX)
#define gettid() ((long long)pthread_self())
#endif

#endif

/**
 * @brief 通用状态码枚举
 * 
 * 定义了系统中使用的基本状态码，用于表示函数执行结果。
 * 所有状态码从STATUS_SUCCESS（0）开始，按照功能分类递增。
 */
enum
{
  // 通用错误码
  STATUS_SUCCESS = 0,     //!< 操作成功，状态码应为零
  STATUS_INVALID_PARAM,   //!< 无效的参数
  STATUS_FAILED_INIT,     //!< 程序初始化失败
  STATUS_PROPERTY_ERR,    //!< 配置属性错误
  STATUS_INIT_LOG,        //!< 日志初始化错误
  STATUS_INIT_THREAD,     //!< 线程初始化失败
  STATUS_FAILED_JOB,      //!< 任务执行失败
  STATUS_FAILED_NETWORK,  //!< 网络操作失败

  STATUS_UNKNOW_ERROR,    //!< 未知错误
  STATUS_LAST_ERR         //!< 最后一个错误码（用于扩展）
};

/**
 * @brief 文件路径分隔符字符常量
 * 
 * 定义了用于构建文件路径的分隔符字符，使用斜杠'/'。
 */
static const char FILE_PATH_SPLIT       = '/';

/**
 * @brief 文件路径分隔符字符串常量
 * 
 * 定义了用于构建文件路径的分隔符字符串，使用斜杠"/"。
 */
static const char FILE_PATH_SPLIT_STR[] = "/";

/**
 * @brief 浮点数比较的精度常量
 * 
 * 定义了用于浮点数比较的极小值，当两个浮点数的差值小于EPSILON时，认为它们相等。
 * 该值通常用于避免浮点数计算中的精度误差。
 */
#define EPSILON (1E-6)

}  // namespace common
