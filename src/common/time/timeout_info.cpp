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

#include "common/time/timeout_info.h"

#include <sys/time.h>
namespace common {

/**
 * @brief TimeoutInfo构造函数实现
 * @param[in] deadLine 超时截止时间
 * @details 初始化超时信息对象，设置截止时间、超时标志和引用计数，并初始化互斥锁
 */
TimeoutInfo::TimeoutInfo(time_t deadLine) : deadline_(deadLine), is_timed_out_(false), ref_cnt_(0)
{
  MUTEX_INIT(&mutex_, NULL);
}

/**
 * @brief TimeoutInfo析构函数实现
 * @details 析构前先解锁互斥锁，然后销毁互斥锁资源
 */
TimeoutInfo::~TimeoutInfo()
{
  // unlock mutex_ as we locked it before 'delete this'
  MUTEX_UNLOCK(&mutex_);

  MUTEX_DESTROY(&mutex_);
}

/**
 * @brief 增加引用计数的实现
 * @details 加锁保护引用计数的增加操作，确保线程安全
 */
void TimeoutInfo::attach()
{
  MUTEX_LOCK(&mutex_);
  ref_cnt_++;
  MUTEX_UNLOCK(&mutex_);
}

/**
 * @brief 减少引用计数的实现
 * @details 加锁保护引用计数的减少操作，当引用计数为0时，自动删除对象
 */
void TimeoutInfo::detach()
{
  MUTEX_LOCK(&mutex_);
  if (0 == --ref_cnt_) {
    // 当引用计数为0时，删除对象自身
    // 注意：在删除前不会解锁，析构函数中会处理解锁
    delete this;
    return;
  }
  MUTEX_UNLOCK(&mutex_);
}

/**
 * @brief 检查是否已超时的实现
 * @return 如果当前时间超过截止时间则返回true，否则返回false
 * @details 加锁保护超时状态的检查，一旦检测到超时，会设置超时标志并在后续调用中直接返回结果
 */
bool TimeoutInfo::has_timed_out()
{
  MUTEX_LOCK(&mutex_);
  bool ret = is_timed_out_;
  if (!is_timed_out_) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // 比较当前时间和截止时间，设置超时标志
    ret = is_timed_out_ = (tv.tv_sec >= deadline_);
  }
  MUTEX_UNLOCK(&mutex_);

  return ret;
}

}  // namespace common