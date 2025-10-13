/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

namespace common {

template <typename T, typename Compare>
int PriorityQueue<T, Compare>::push(T &&value)
{
  lock_guard<mutex> lock(mutex_);
  priority_queue_.push(std::move(value));
  return 0;
}

template <typename T, typename Compare>
int PriorityQueue<T, Compare>::pop(T &value)
{
  lock_guard<mutex> lock(mutex_);
  if (priority_queue_.empty()) {
    return -1;
  }

  value = std::move(const_cast<T&>(priority_queue_.top()));
  priority_queue_.pop();
  return 0;
}

template <typename T, typename Compare>
int PriorityQueue<T, Compare>::size() const
{
  lock_guard<mutex> lock(mutex_);
  return priority_queue_.size();
}

template <typename T, typename Compare>
int PriorityQueue<T, Compare>::clear()
{
  lock_guard<mutex> lock(mutex_);
  // 创建一个新的空优先队列来替换当前的队列
  priority_queue_ = std::priority_queue<value_type, vector<value_type>, comparator_type>(compare_);
  return 0;
}

} // namespace common