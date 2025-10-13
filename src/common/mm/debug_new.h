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

#include <new>
#include <stdlib.h>

namespace common {

/**
 * @file debug_new.h
 * @brief 内存调试工具，用于跟踪内存分配和检测内存泄漏
 * 
 * 本文件提供了一个内存调试工具，通过重定义new操作符来记录内存分配的文件名和行号，
 * 从而在程序结束时能够检测并报告内存泄漏情况。
 */

/* 函数原型 */

/**
 * @brief 检查内存泄漏
 * @return 如果没有内存泄漏返回true，否则返回false
 * @details 在程序结束时调用，会报告所有未释放的内存块
 */
bool  check_leaks();

/**
 * @brief 带调试信息的new操作符重载
 * @param[in] size 要分配的内存大小
 * @param[in] file 调用new的文件名
 * @param[in] line 调用new的行号
 * @return 分配的内存指针
 * @details 记录内存分配的位置信息，用于后续的内存泄漏检测
 */
void *operator new(size_t size, const char *file, int line);

/**
 * @brief 带调试信息的数组new操作符重载
 * @param[in] size 要分配的内存大小
 * @param[in] file 调用new的文件名
 * @param[in] line 调用new的行号
 * @return 分配的内存指针
 * @details 记录数组内存分配的位置信息
 */
void *operator new[](size_t size, const char *file, int line);

#ifndef NO_PLACEMENT_DELETE
/**
 * @brief 带调试信息的delete操作符重载
 * @param[in] pointer 要释放的内存指针
 * @param[in] file 调用delete的文件名
 * @param[in] line 调用delete的行号
 * @details 与带调试信息的new配对使用的delete操作符
 */
void operator delete(void *pointer, const char *file, int line);

/**
 * @brief 带调试信息的数组delete操作符重载
 * @param[in] pointer 要释放的内存指针
 * @param[in] file 调用delete的文件名
 * @param[in] line 调用delete的行号
 * @details 与带调试信息的数组new配对使用的delete操作符
 */
void operator delete[](void *pointer, const char *file, int line);
#endif                           // NO_PLACEMENT_DELETE

/**
 * @brief MSVC 6需要的数组delete操作符声明
 * @param[in] pointer 要释放的内存指针
 */
void operator delete[](void *);

/* 宏定义 */

#ifndef DEBUG_NEW_NO_NEW_REDEFINITION
/**
 * @brief 重定义new操作符，使其自动记录文件名和行号
 * @details 当未定义DEBUG_NEW_NO_NEW_REDEFINITION时，所有的new操作都会被替换为带调试信息的版本
 */
#define new DEBUG_NEW
#define DEBUG_NEW new (__FILE__, __LINE__)
#define debug_new new
#else
/**
 * @brief 仅定义debug_new宏，不重定义默认的new操作符
 * @details 当定义了DEBUG_NEW_NO_NEW_REDEFINITION时，只有显式使用debug_new的地方才会记录调试信息
 */
#define debug_new new (__FILE__, __LINE__)
#endif  // DEBUG_NEW_NO_NEW_REDEFINITION

#ifdef DEBUG_NEW_EMULATE_MALLOC
/**
 * @brief 模拟malloc函数，使用debug_new实现
 * @param[in] s 要分配的内存大小
 * @return 分配的内存指针
 * @details 当定义了DEBUG_NEW_EMULATE_MALLOC时，将malloc重定向到debug_new
 */
#define malloc(s) ((void *)(debug_new char[s]))

/**
 * @brief 模拟free函数，使用delete[]实现
 * @param[in] p 要释放的内存指针
 * @details 当定义了DEBUG_NEW_EMULATE_MALLOC时，将free重定向到delete[]
 */
#define free(p) delete[](char *)(p)

#endif  // DEBUG_NEW_EMULATE_MALLOC

/* 控制标志 */

/**
 * @brief 是否输出详细信息的标志
 * @details 默认为false，表示不输出详细信息
 */
extern bool new_verbose_flag;

/**
 * @brief 程序退出时是否自动检查内存泄漏的标志
 * @details 默认为true，表示程序退出时会自动调用check_leaks()
 */
extern bool new_autocheck_flag;

}  // namespace common
