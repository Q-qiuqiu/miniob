/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/queue/queue.h"
#include "common/lang/mutex.h"
#include "common/lang/deque.h"

namespace common {

/**
 * @brief 基于双端队列的线程安全任务队列
 * @details 使用双端队列作为底层容器，支持从队列两端进行操作，所有接口都加锁保证线程安全。
 * @tparam T 任务数据类型
 * @ingroup Queue
 */
template <typename T>
class DequeQueue : public Queue<T>
{
public:
  using value_type = T;

public:
  DequeQueue() : Queue<T>() {}
  virtual ~DequeQueue() {}

  //! @copydoc Queue::push
  int push(value_type &&value) override;
  //! @copydoc Queue::pop
  int pop(value_type &value) override;
  //! @copydoc Queue::size
  int size() const override;

  /**
   * @brief 从队列尾部添加一个任务
   * 
   * @param value 任务数据
   * @return int 成功返回0
   */
  int push_back(value_type &&value);

  /**
   * @brief 从队列头部添加一个任务
   * 
   * @param value 任务数据
   * @return int 成功返回0
   */
  int push_front(value_type &&value);

  /**
   * @brief 从队列尾部取出一个任务
   * 
   * @param value 任务数据
   * @return int 成功返回0。如果队列为空，返回-1
   */
  int pop_back(value_type &value);

private:
  mutable mutex mutex_;  ///< 用于保证线程安全的互斥锁
  deque<value_type> deque_;  ///< 底层双端队列容器
};

}  // namespace common

#include "common/queue/deque_queue.ipp"