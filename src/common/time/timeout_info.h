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

#ifndef __COMMON_TIME_TIMEOUT_INFO_H__
#define __COMMON_TIME_TIMEOUT_INFO_H__

#include <time.h>

#include "common/lang/mutex.h"
namespace common {

/**
 * Timeout info class used to judge if a certain deadline_ has reached or not.
 * It's good to use handle-body to automate the reference count
 * increase/decrease. However, explicit attach/detach interfaces
 * are used here to simplify the implementation.
 */

/**
 * @file timeout_info.h
 * @brief 超时信息管理模块
 * @details 提供了用于判断是否达到截止时间的功能类TimeoutInfo
 */

/**
 * @class TimeoutInfo
 * @brief 超时信息类，用于判断是否已达到某个截止时间
 * @details 该类使用引用计数机制管理对象生命周期，提供了判断超时状态的方法
 *          虽然可以使用handle-body模式自动管理引用计数，但为了简化实现，这里使用显式的attach/detach接口
 */
class TimeoutInfo
{
public:
  /**
   * @brief 构造函数
   * @param[in] deadline_ 超时截止时间
   */
  TimeoutInfo(time_t deadline_);

  /**
   * @brief 增加引用计数
   * @details 调用此方法表示增加一个对此对象的引用，防止对象在使用过程中被销毁
   */
  void attach();

  /**
   * @brief 减少引用计数
   * @details 调用此方法表示减少一个对此对象的引用，当引用计数为0时，对象会被自动销毁
   */
  void detach();

  /**
   * @brief 检查是否已超时
   * @return 如果已超时返回true，否则返回false
   */
  bool has_timed_out();

private:
  /**
   * @brief 禁止复制构造函数，以支持引用计数机制
   */
  TimeoutInfo(const TimeoutInfo &ti);

  /**
   * @brief 禁止赋值运算符，以支持引用计数机制
   */
  TimeoutInfo &operator=(const TimeoutInfo &ti);

protected:
  /**
   * @brief 析构函数
   * @details 防止在堆上创建TimeoutInfo对象，以便于与StageEvent关联
   */
  ~TimeoutInfo();

private:
  time_t deadline_;       ///< 超时截止时间

  // used to predict timeout if now + reservedTime > deadline_
  // time_t reservedTime;
  
  bool   is_timed_out_;   ///< 超时标志
  int    ref_cnt_;        ///< 对象的引用计数
  pthread_mutex_t mutex_; ///< 用于保护引用计数和超时标志的互斥锁
};

}  // namespace common
#endif  // __COMMON_TIME_TIMEOUT_INFO_H__
