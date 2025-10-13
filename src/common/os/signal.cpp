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

#include "common/os/signal.h"
#include "common/log/log.h"
#include "pthread.h"
#include "common/lang/iostream.h"

namespace common {

/**
 * 设置特定信号的处理函数
 * @param sig 信号编号
 * @param func 信号处理函数指针
 * @note 如果设置信号处理函数失败，会输出错误信息到标准错误流
 */
void set_signal_handler(int sig, sighandler_t func)
{
  struct sigaction newsa, oldsa;
  sigemptyset(&newsa.sa_mask);  // 初始化信号掩码集为空
  newsa.sa_flags   = 0;         // 不设置特殊标志
  newsa.sa_handler = func;      // 设置信号处理函数

  int rc = sigaction(sig, &newsa, &oldsa);  // 应用新的信号处理配置
  if (rc) {
    // 输出错误信息，包括信号编号和系统调用错误位置
    cerr << "Failed to set signal " << sig << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
  }
}

/**
 * 设置多个常用信号的处理函数
 * @param func 信号处理函数指针
 * @note 会设置 SIGQUIT、SIGINT、SIGHUP、SIGTERM 信号的处理函数，并忽略 SIGPIPE 信号
 */
void set_signal_handler(sighandler_t func)
{
  set_signal_handler(SIGQUIT, func);  // 设置退出信号处理
  set_signal_handler(SIGINT, func);   // 设置中断信号处理
  set_signal_handler(SIGHUP, func);   // 设置挂起信号处理
  set_signal_handler(SIGTERM, func);  // 设置终止信号处理
  signal(SIGPIPE, SIG_IGN);           // 忽略管道破裂信号
}

/**
 * 阻塞默认信号
 * @param signal_set 输出参数，用于存储被阻塞的信号集
 * @param old_set 输出参数，用于存储之前的信号集状态
 * @note 在 DEBUG 模式下不会阻塞 SIGINT 信号，以方便调试
 */
void block_default_signals(sigset_t *signal_set, sigset_t *old_set)
{
  sigemptyset(signal_set);  // 初始化信号集为空
#ifndef DEBUG
  // 在非调试模式下阻塞 SIGINT 信号
  sigaddset(signal_set, SIGINT);
#endif
  sigaddset(signal_set, SIGTERM);  // 阻塞终止信号
  sigaddset(signal_set, SIGUSR1);  // 阻塞用户自定义信号
  pthread_sigmask(SIG_BLOCK, signal_set, old_set);  // 应用信号阻塞设置
}

/**
 * 解除默认信号的阻塞
 * @param signal_set 输出参数，用于存储要解除阻塞的信号集
 * @param old_set 输出参数，用于存储之前的信号集状态
 * @note 在 DEBUG 模式下不会解除 SIGINT 信号的阻塞
 */
void unblock_default_signals(sigset_t *signal_set, sigset_t *old_set)
{
  sigemptyset(signal_set);  // 初始化信号集为空
#ifndef DEBUG
  // 在非调试模式下解除 SIGINT 信号的阻塞
  sigaddset(signal_set, SIGINT);
#endif
  sigaddset(signal_set, SIGTERM);  // 解除终止信号阻塞
  sigaddset(signal_set, SIGUSR1);  // 解除用户自定义信号阻塞
  pthread_sigmask(SIG_UNBLOCK, signal_set, old_set);  // 应用信号解除阻塞设置
}

/**
 * 等待信号的函数
 * @param args 传递给线程的参数，实际上是信号集指针
 * @return 函数不会返回（无限循环等待信号）
 * @note 此函数通常作为线程函数运行，用于等待并处理指定的信号
 */
void *wait_for_signals(void *args)
{
  LOG_INFO("Start thread to wait signals.");
  sigset_t *signal_set = (sigset_t *)args;
  int       sig_number = -1;
  while (true) {
    errno   = 0;
    int ret = sigwait(signal_set, &sig_number);  // 等待信号集里的信号
    LOG_INFO("sigwait return value: %d, %d \n", ret, sig_number);
    if (ret != 0) {
      LOG_ERROR("error (%d) %s\n", errno, strerror(errno));
    }
  }
  return NULL;
}

/**
 * 启动一个线程来等待信号
 * @param signal_set 要等待的信号集
 * @note 创建一个分离的线程来运行 wait_for_signals 函数
 */
void start_wait_for_signals(sigset_t *signal_set)
{
  pthread_t      pThread;
  pthread_attr_t pThreadAttrs;

  // 初始化线程属性，设置为分离状态
  pthread_attr_init(&pThreadAttrs);
  pthread_attr_setdetachstate(&pThreadAttrs, PTHREAD_CREATE_DETACHED);

  // 创建线程，执行 wait_for_signals 函数
  pthread_create(&pThread, &pThreadAttrs, wait_for_signals, (void *)signal_set);
}
}  // namespace common
