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

#include <sstream>

#include "common/lang/mutex.h"
#include "common/lang/string.h"
#include "common/lang/set.h"
#include "common/lang/list.h"
#include "common/lang/memory.h"
#include "common/lang/sstream.h"
#include "common/log/log.h"
#include "common/os/os.h"

#include <sanitizer/asan_interface.h>

namespace common {

/**
 * 默认每个内存池中的项目数量
 */
#define DEFAULT_ITEM_NUM_PER_POOL 128
/**
 * 默认的内存池数量
 */
#define DEFAULT_POOL_NUM 1

/**
 * 用于内存池中的匹配函数指针类型
 * @param item 要匹配的项目
 * @param input_arg 输入参数
 * @return 是否匹配成功
 */
typedef bool (*match)(void *item, void *input_arg);

/**
 * 内存池抽象基类模板
 * 定义了内存池的基本接口，提供了内存分配和释放的统一方法
 * @tparam T 内存池中存储的对象类型
 */
template <class T>
class MemPool
{
public:
  /**
   * 构造函数
   * @param tag 内存池的名称标签，用于日志和调试
   */
  MemPool(const char *tag) : name(tag)
  {
    this->size = 0;

    pthread_mutexattr_t mutexatr;
    pthread_mutexattr_init(&mutexatr);
    pthread_mutexattr_settype(&mutexatr, PTHREAD_MUTEX_RECURSIVE);

    MUTEX_INIT(&mutex, &mutexatr);
  }

  /**
   * 虚析构函数
   * 确保派生类的析构函数能够被正确调用
   */
  virtual ~MemPool() { MUTEX_DESTROY(&mutex); }

  /**
   * 初始化内存池，主要工作是为内存池分配内存
   * @param pool_num 内存池的数量
   * @param item_num_per_pool 每个内存池中的项目数量
   * @return 0表示成功，其他值表示失败
   */
  virtual int init(
      bool dynamic = true, int pool_num = DEFAULT_POOL_NUM, int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL) = 0;

  /**
   * 清理内存池资源
   */
  virtual void cleanup() = 0;

  /**
   * 如果设置为动态扩展，则扩展当前内存池
   */
  virtual int extend() = 0;

  /**
   * 从内存池中分配一个对象
   * @return 分配的对象指针，如果分配失败则返回nullptr
   */
  virtual T *alloc() = 0;

  /**
   * 释放一个对象，将资源返回给内存池
   * @param item 要释放的对象指针
   */
  virtual void free(T *item) = 0;

  /**
   * 打印内存池状态信息
   * @return 包含内存池状态的字符串
   */
  virtual string to_string() = 0;

  /**
   * 获取内存池名称
   * @return 内存池名称
   */
  const string get_name() const { return name; }
  /**
   * 检查内存池是否支持动态扩展
   * @return 是否支持动态扩展
   */
  bool         is_dynamic() const { return dynamic; }
  /**
   * 获取内存池大小
   * @return 内存池中的项目总数
   */
  int          get_size() const { return size; }

protected:
  pthread_mutex_t mutex; ///< 互斥锁，用于线程安全
  int             size; ///< 内存池中的项目总数
  bool            dynamic; ///< 是否支持动态扩展
  string          name; ///< 内存池名称，用于日志和调试
};

/**
 * 简单的内存池实现类模板
 * 对象在创建内存池时构造，在清理内存池时析构
 * `alloc`调用T的`reinit`方法，`free`调用T的`reset`方法
 * @tparam T 内存池中存储的对象类型
 */
template <class T>
class MemPoolSimple : public MemPool<T>
{
public:
  /**
   * 构造函数
   * @param tag 内存池的名称标签
   */
  MemPoolSimple(const char *tag) : MemPool<T>(tag) {}

  /**
   * 析构函数
   * 自动清理内存池资源
   */
  virtual ~MemPoolSimple() { cleanup(); }

  /**
   * 初始化内存池，主要工作是为内存池分配内存
   * @param dynamic 是否支持动态扩展
   * @param pool_num 内存池的数量
   * @param item_num_per_pool 每个内存池中的项目数量
   * @return 0表示成功，其他值表示失败
   */
  int init(bool dynamic = true, int pool_num = DEFAULT_POOL_NUM, int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL);

  /**
   * 清理内存池资源
   */
  void cleanup();

  /**
   * 如果设置为动态扩展，则扩展当前内存池
   * @return 0表示成功，其他值表示失败
   */
  int extend();

  /**
   * 从内存池中分配一个对象
   * @return 分配的对象指针，如果分配失败则返回nullptr
   */
  T *alloc();

  /**
   * 释放一个对象，将资源返回给内存池
   * @param item 要释放的对象指针
   */
  void free(T *item);

  /**
   * 打印内存池状态信息
   * @return 包含内存池状态的字符串
   */
  string to_string();

  /**
   * 获取每个内存池中的项目数量
   * @return 每个内存池中的项目数量
   */
  int get_item_num_per_pool() const { return item_num_per_pool; }

  /**
   * 获取当前正在使用的项目数量
   * @return 当前正在使用的项目数量
   */
  int get_used_num()
  {
    MUTEX_LOCK(&this->mutex);
    auto num = used.size();
    MUTEX_UNLOCK(&this->mutex);
    return num;
  }

protected:
  list<T *> pools; ///< 内存池列表，每个元素是一个对象数组
  set<T *>  used; ///< 正在使用的对象集合
  list<T *> frees; ///< 空闲的对象列表
  int       item_num_per_pool; ///< 每个内存池中的项目数量

private:
  /**
   * 使用ASAN对内存进行标记（标记为不可访问）
   * @param addr 内存地址
   * @param size 内存大小
   */
  inline void asan_poison(void *addr, size_t size) { ASAN_POISON_MEMORY_REGION(addr, size); }
  /**
   * 取消ASAN对内存的标记（标记为可访问）
   * @param addr 内存地址
   * @param size 内存大小
   */
  inline void asan_unpoison(void *addr, size_t size) { ASAN_UNPOISON_MEMORY_REGION(addr, size); }
};

/**
 * 初始化内存池，主要工作是为内存池分配内存
 * @tparam T 内存池中存储的对象类型
 * @param dynamic 是否支持动态扩展
 * @param pool_num 内存池的数量
 * @param item_num_per_pool 每个内存池中的项目数量
 * @return 0表示成功，其他值表示失败
 */
template <class T>
int MemPoolSimple<T>::init(bool dynamic, int pool_num, int item_num_per_pool)
{
  if (pools.empty() == false) {
    LOG_WARN("Memory pool has been initialized, but still begin to be initialized, this->name:%s.", this->name.c_str());
    return 0;
  }

  if (pool_num <= 0 || item_num_per_pool <= 0) {
    LOG_ERROR("Invalid arguments,  pool_num:%d, item_num_per_pool:%d, this->name:%s.",
              pool_num, item_num_per_pool, this->name.c_str());
    return -1;
  }

  this->item_num_per_pool = item_num_per_pool;
  // 为了初始化内存池，这里临时启用动态扩展
  this->dynamic = true;
  for (int i = 0; i < pool_num; i++) {
    if (extend() < 0) {
      cleanup();
      return -1;
    }
  }
  this->dynamic = dynamic;

  LOG_INFO("Extend one pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
           this->size, item_num_per_pool, this->name.c_str());
  return 0;
}

/**
 * 清理内存池资源
 * @tparam T 内存池中存储的对象类型
 */
template <class T>
void MemPoolSimple<T>::cleanup()
{
  if (pools.empty() == true) {
    LOG_WARN("Begin to do cleanup, but there is no memory pool, this->name:%s!", this->name.c_str());
    return;
  }
  MUTEX_LOCK(&this->mutex);
  for (auto &&i : frees) {
    asan_unpoison(i, sizeof(T));
  }
  used.clear();
  frees.clear();
  this->size = 0;

  for (typename list<T *>::iterator iter = pools.begin(); iter != pools.end(); iter++) {
    T *pool = *iter;
    delete[] pool;
  }
  pools.clear();
  MUTEX_UNLOCK(&this->mutex);
  LOG_INFO("Successfully do cleanup, this->name:%s.", this->name.c_str());
}

/**
 * 扩展当前内存池
 * @tparam T 内存池中存储的对象类型
 * @return 0表示成功，其他值表示失败
 */
template <class T>
int MemPoolSimple<T>::extend()
{
  if (this->dynamic == false) {
    LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, this->name:%s", this->name.c_str());
    return -1;
  }

  MUTEX_LOCK(&this->mutex);
  T *pool = new T[item_num_per_pool];
  if (pool == nullptr) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_ERROR("Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
              this->size, item_num_per_pool, this->name.c_str());
    return -1;
  }

  pools.push_back(pool);
  this->size += item_num_per_pool;
  for (int i = 0; i < item_num_per_pool; i++) {
    frees.push_back(pool + i);
    asan_poison(pool + i, sizeof(T));
  }
  MUTEX_UNLOCK(&this->mutex);

  LOG_INFO("Extend one pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
           this->size, item_num_per_pool, this->name.c_str());
  return 0;
}

/**
 * 从内存池中分配一个对象
 * @tparam T 内存池中存储的对象类型
 * @return 分配的对象指针，如果分配失败则返回nullptr
 */
template <class T>
T *MemPoolSimple<T>::alloc()
{
  MUTEX_LOCK(&this->mutex);
  if (frees.empty() == true) {
    if (this->dynamic == false) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }

    if (extend() < 0) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }
  }
  T *buffer = frees.front();
  asan_unpoison(buffer, sizeof(T));
  frees.pop_front();

  used.insert(buffer);

  MUTEX_UNLOCK(&this->mutex);
  buffer->reinit(); // 调用对象的reinit方法重新初始化对象
  return buffer;
}

/**
 * 释放一个对象，将资源返回给内存池
 * @tparam T 内存池中存储的对象类型
 * @param buf 要释放的对象指针
 */
template <class T>
void MemPoolSimple<T>::free(T *buf)
{
  buf->reset(); // 调用对象的reset方法重置对象状态

  MUTEX_LOCK(&this->mutex);

  size_t num = used.erase(buf);
  if (num == 0) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_WARN("No entry of %p in %s.", buf, this->name.c_str());
    print_stacktrace();
    return;
  }
  asan_poison(buf, sizeof(T));
  frees.push_back(buf);

  MUTEX_UNLOCK(&this->mutex);
  return;  // TODO for test
}

/**
 * 打印内存池状态信息
 * @tparam T 内存池中存储的对象类型
 * @return 包含内存池状态的字符串
 */
template <class T>
string MemPoolSimple<T>::to_string()
{
  stringstream ss;

  ss << "name:" << this->name << ","
     << "dyanmic:" << this->dynamic << ","
     << "size:" << this->size << ","
     << "pool_size:" << this->pools.size() << ","
     << "used_size:" << this->used.size() << ","
     << "free_size:" << this->frees.size();
  return ss.str();
}

/**
 * 内存池项目类
 * 用于管理任意大小的内存块的内存池实现
 */
class MemPoolItem
{
public:
  using item_unique_ptr = unique_ptr<void, function<void(void *const)>>; ///< 内存池项目的智能指针类型

public:
  /**
   * 构造函数
   * @param tag 内存池的名称标签
   */
  MemPoolItem(const char *tag) : name(tag)
  {
    this->size = 0;

    pthread_mutexattr_t mutexatr;
    pthread_mutexattr_init(&mutexatr);
    pthread_mutexattr_settype(&mutexatr, PTHREAD_MUTEX_RECURSIVE);

    MUTEX_INIT(&mutex, &mutexatr);
  }

  /**
   * 析构函数
   * 自动清理内存池资源
   */
  virtual ~MemPoolItem()
  {
    cleanup();
    MUTEX_DESTROY(&mutex);
  }

  /**
   * 初始化内存池
   * @param item_size 每个项目的大小（字节）
   * @param dynamic 是否支持动态扩展
   * @param pool_num 内存池的数量
   * @param item_num_per_pool 每个内存池中的项目数量
   * @return 0表示成功，其他值表示失败
   */
  int init(int item_size, bool dynamic = true, int pool_num = DEFAULT_POOL_NUM,
      int item_num_per_pool = DEFAULT_ITEM_NUM_PER_POOL);

  /**
   * 清理内存池资源
   */
  void cleanup();

  /**
   * 如果设置为动态扩展，则扩展当前内存池
   * @return 0表示成功，其他值表示失败
   */
  int extend();

  /**
   * 从内存池中分配一个内存块
   * @return 分配的内存块指针，如果分配失败则返回nullptr
   */
  void           *alloc();
  /**
   * 从内存池中分配一个内存块，并返回智能指针
   * @return 指向分配的内存块的智能指针
   */
  item_unique_ptr alloc_unique_ptr();

  /**
   * 释放一个内存块，将资源返回给内存池
   * @param item 要释放的内存块指针
   */
  void free(void *item);

  /**
   * 检查指定的内存块是否正在被使用
   * @param item 要检查的内存块指针
   * @return 是否正在被使用
   */
  bool is_used(void *item)
  {
    MUTEX_LOCK(&mutex);
    auto it = used.find(item);
    MUTEX_UNLOCK(&mutex);
    return it != used.end();
  }

  /**
   * 打印内存池状态信息
   * @return 包含内存池状态的字符串
   */
  string to_string()
  {

    stringstream ss;

    ss << "name:" << this->name << ","
       << "dyanmic:" << this->dynamic << ","
       << "size:" << this->size << ","
       << "pool_size:" << this->pools.size() << ","
       << "used_size:" << this->used.size() << ","
       << "free_size:" << this->frees.size();
    return ss.str();
  }

  /**
   * 获取内存池名称
   * @return 内存池名称
   */
  const string get_name() const { return name; }
  /**
   * 检查内存池是否支持动态扩展
   * @return 是否支持动态扩展
   */
  bool         is_dynamic() const { return dynamic; }
  /**
   * 获取内存池大小
   * @return 内存池中的项目总数
   */
  int          get_size() const { return size; }
  /**
   * 获取每个项目的大小
   * @return 每个项目的大小（字节）
   */
  int          get_item_size() const { return item_size; }
  /**
   * 获取每个内存池中的项目数量
   * @return 每个内存池中的项目数量
   */
  int          get_item_num_per_pool() const { return item_num_per_pool; }

  /**
   * 获取当前正在使用的项目数量
   * @return 当前正在使用的项目数量
   */
  int get_used_num()
  {
    MUTEX_LOCK(&mutex);
    auto num = used.size();
    MUTEX_UNLOCK(&mutex);
    return num;
  }

protected:
  pthread_mutex_t mutex; ///< 互斥锁，用于线程安全
  string          name; ///< 内存池名称，用于日志和调试
  bool            dynamic; ///< 是否支持动态扩展
  int             size; ///< 内存池中的项目总数
  int             item_size; ///< 每个项目的大小（字节）
  int             item_num_per_pool; ///< 每个内存池中的项目数量

  list<void *> pools; ///< 内存池列表，每个元素是一个内存块数组
  set<void *>  used; ///< 正在使用的内存块集合
  list<void *> frees; ///< 空闲的内存块列表
};

}  // namespace common
