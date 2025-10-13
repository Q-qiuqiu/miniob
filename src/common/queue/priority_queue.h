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
#include "common/lang/functional.h"
#include "common/lang/vector.h"

namespace common {

/**
 * @brief 基于优先级的线程安全任务队列
 * @details 使用vector作为底层容器，支持自定义比较器，所有接口都加锁保证线程安全。
 * 优先级队列会自动根据比较器将元素按优先级排序，默认情况下，优先级高的元素会先出队。
 * @tparam T 任务数据类型
 * @tparam Compare 比较器类型，默认为std::less<T>
 * @ingroup Queue
 */
template <typename T, typename Compare = std::less<T>>
class PriorityQueue : public Queue<T>
{
public:
  using value_type = T;
  using comparator_type = Compare;

public:
  PriorityQueue() : Queue<T>(), compare_(), priority_queue_(compare_) {}
  
  /**
   * @brief 使用自定义比较器构造优先队列
   * 
   * @param compare 比较器对象
   */
  explicit PriorityQueue(const comparator_type &compare) 
    : Queue<T>(), compare_(compare), priority_queue_(compare_) {}
  
  virtual ~PriorityQueue() {}

  //! @copydoc Queue::push
  int push(value_type &&value) override;
  //! @copydoc Queue::pop
  int pop(value_type &value) override;
  //! @copydoc Queue::size
  int size() const override;

  /**
   * @brief 清空队列
   * 
   * @return int 成功返回0
   */
  int clear();

private:
  mutable mutex mutex_;  ///< 用于保证线程安全的互斥锁
  comparator_type compare_;  ///< 比较器对象
  std::priority_queue<value_type, vector<value_type>, comparator_type> priority_queue_;  ///< 底层优先队列容器
};

}  // namespace common

#include "common/queue/priority_queue.ipp"