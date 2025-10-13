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

#include <signal.h>

namespace common {

/**
 * 阻塞默认信号的函数
 * @param signal_set 输出参数，用于存储被阻塞的信号集
 * @param old_set 输出参数，用于存储之前的信号集状态
 * @note 当前实现会阻塞 SIGINT、SIGTERM 和 SIGUSR1 信号
 */
void block_default_signals(sigset_t *signal_set, sigset_t *old_set);

/**
 * 解除默认信号阻塞的函数
 * @param signal_set 输出参数，用于存储要解除阻塞的信号集
 * @param old_set 输出参数，用于存储之前的信号集状态
 * @note 当前实现会解除 SIGINT、SIGTERM 和 SIGUSR1 信号的阻塞
 */
void unblock_default_signals(sigset_t *signal_set, sigset_t *old_set);

/**
 * 等待信号的函数
 * @param signal_set 要等待的信号集
 * @return 函数不会返回（无限循环等待信号）
 */
void *wait_for_signals(sigset_t *signal_set);

/**
 * 启动信号等待线程的函数
 * @param signal_set 要等待的信号集
 * @note 此函数会创建一个分离的线程来等待指定的信号集
 */
void  start_wait_for_signals(sigset_t *signal_set);

/**
 * 信号处理函数类型定义
 * @param int 信号编号
 */
typedef void (*sighandler_t)(int);

/**
 * 设置多个常用信号的处理函数
 * @param func 信号处理函数指针
 * @note 会设置 SIGQUIT、SIGINT、SIGHUP、SIGTERM 信号的处理函数，并忽略 SIGPIPE 信号
 */
void set_signal_handler(sighandler_t func);

/**
 * 设置特定信号的处理函数
 * @param sig 信号编号
 * @param func 信号处理函数指针
 */
void set_signal_handler(int sig, sighandler_t func);

}  // namespace common
