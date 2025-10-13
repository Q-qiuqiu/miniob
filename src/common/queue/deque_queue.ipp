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

template <typename T>
int DequeQueue<T>::push(T &&value)
{
  lock_guard<mutex> lock(mutex_);
  deque_.push_back(std::move(value));
  return 0;
}

template <typename T>
int DequeQueue<T>::pop(T &value)
{
  lock_guard<mutex> lock(mutex_);
  if (deque_.empty()) {
    return -1;
  }

  value = std::move(deque_.front());
  deque_.pop_front();
  return 0;
}

template <typename T>
int DequeQueue<T>::size() const
{
  lock_guard<mutex> lock(mutex_);
  return deque_.size();
}

template <typename T>
int DequeQueue<T>::push_back(T &&value)
{
  lock_guard<mutex> lock(mutex_);
  deque_.push_back(std::move(value));
  return 0;
}

template <typename T>
int DequeQueue<T>::push_front(T &&value)
{
  lock_guard<mutex> lock(mutex_);
  deque_.push_front(std::move(value));
  return 0;
}

template <typename T>
int DequeQueue<T>::pop_back(T &value)
{
  lock_guard<mutex> lock(mutex_);
  if (deque_.empty()) {
    return -1;
  }

  value = std::move(deque_.back());
  deque_.pop_back();
  return 0;
}

} // namespace common