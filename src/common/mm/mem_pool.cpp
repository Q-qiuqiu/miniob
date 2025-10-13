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
// Created by Longda on 2022/1/28.
//

#include "common/mm/mem_pool.h"
namespace common {

/**
 * 初始化内存池
 * @param item_size 每个项目的大小（字节）
 * @param dynamic 是否支持动态扩展
 * @param pool_num 内存池的数量
 * @param item_num_per_pool 每个内存池中的项目数量
 * @return 0表示成功，其他值表示失败
 */
int MemPoolItem::init(int item_size, bool dynamic, int pool_num, int item_num_per_pool)
{
  // 检查内存池是否已经初始化
  if (pools.empty() == false) {
    LOG_WARN("Memory pool has been initialized, but still begin to be initialized, this->name:%s.", this->name.c_str());
    return 0;
  }

  // 检查参数是否有效
  if (item_size <= 0 || pool_num <= 0 || item_num_per_pool <= 0) {
    LOG_ERROR("Invalid arguments, item_size:%d, pool_num:%d, item_num_per_pool:%d, this->name:%s.",
        item_size, pool_num, item_num_per_pool, this->name.c_str());
    return -1;
  }

  this->item_size         = item_size;
  this->item_num_per_pool = item_num_per_pool;
  // 为了初始化内存池，这里临时启用动态扩展
  this->dynamic = true;
  for (int i = 0; i < pool_num; i++) {
    if (extend() < 0) {
      cleanup();
      return -1;
    }
  }
  this->dynamic = dynamic; // 恢复用户指定的动态扩展设置

  LOG_INFO("Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      this->size, item_size, item_num_per_pool, this->name.c_str());
  return 0;
}

/**
 * 清理内存池资源
 */
void MemPoolItem::cleanup()
{
  // 检查内存池是否为空
  if (pools.empty() == true) {
    LOG_WARN("Begin to do cleanup, but there is no memory pool, this->name:%s!", this->name.c_str());
    return;
  }

  MUTEX_LOCK(&this->mutex); // 加锁保证线程安全

  // 清空使用中和空闲的项目集合
  used.clear();
  frees.clear();
  this->size = 0;

  // 释放所有内存池
  for (list<void *>::iterator iter = pools.begin(); iter != pools.end(); iter++) {
    void *pool = *iter;

    ::free(pool); // 使用系统free函数释放内存
  }
  pools.clear();
  MUTEX_UNLOCK(&this->mutex); // 解锁
  LOG_INFO("Successfully do cleanup, this->name:%s.", this->name.c_str());
}

/**
 * 扩展当前内存池，分配新的内存块
 * @return 0表示成功，其他值表示失败
 */
int MemPoolItem::extend()
{
  // 检查是否允许动态扩展
  if (this->dynamic == false) {
    LOG_ERROR("Disable dynamic extend memory pool, but begin to extend, this->name:%s", this->name.c_str());
    return -1;
  }

  MUTEX_LOCK(&this->mutex); // 加锁保证线程安全
  // 分配新的内存块
  void *pool = malloc(static_cast<size_t>(item_num_per_pool) * item_size);
  if (pool == nullptr) {
    MUTEX_UNLOCK(&this->mutex);
    LOG_ERROR("Failed to extend memory pool, this->size:%d, item_num_per_pool:%d, this->name:%s.",
        this->size,
        item_num_per_pool,
        this->name.c_str());
    return -1;
  }

  // 将新内存块添加到内存池列表
  pools.push_back(pool);
  this->size += item_num_per_pool;
  // 将新内存块中的每个项目添加到空闲列表
  for (int i = 0; i < item_num_per_pool; i++) {
    char *item = (char *)pool + i * item_size;
    frees.push_back((void *)item);
  }
  MUTEX_UNLOCK(&this->mutex); // 解锁

  LOG_INFO("Extend one pool, this->size:%d, item_size:%d, item_num_per_pool:%d, this->name:%s.",
      this->size,
      item_size,
      item_num_per_pool,
      this->name.c_str());
  return 0;
}

/**
 * 从内存池中分配一个内存块
 * @return 分配的内存块指针，如果分配失败则返回nullptr
 */
void *MemPoolItem::alloc()
{
  MUTEX_LOCK(&this->mutex); // 加锁保证线程安全
  // 检查是否有空闲项目
  if (frees.empty() == true) {
    if (this->dynamic == false) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }

    // 动态扩展内存池
    if (extend() < 0) {
      MUTEX_UNLOCK(&this->mutex);
      return nullptr;
    }
  }
  // 从空闲列表获取第一个项目
  void *buffer = frees.front();
  frees.pop_front();

  // 将项目添加到使用中集合
  used.insert(buffer);

  MUTEX_UNLOCK(&this->mutex); // 解锁

  // 初始化分配的内存块为0
  memset(buffer, 0, sizeof(item_size));
  return buffer;
}

/**
 * 从内存池中分配一个内存块，并返回智能指针
 * @return 指向分配的内存块的智能指针
 */
MemPoolItem::item_unique_ptr MemPoolItem::alloc_unique_ptr()
{
  void *item = this->alloc(); // 分配内存块
  // 创建自定义删除器，使用内存池的free方法释放资源
  auto deleter = [this](void *p) { this->free(p); };
  return MemPoolItem::item_unique_ptr(item, deleter); // 返回带有自定义删除器的智能指针
}

/**
 * 释放一个内存块，将资源返回给内存池
 * @param buf 要释放的内存块指针
 */
void MemPoolItem::free(void *buf)
{
  MUTEX_LOCK(&this->mutex); // 加锁保证线程安全

  // 从使用中集合中删除内存块
  size_t num = used.erase(buf);
  if (num == 0) {
    MUTEX_UNLOCK(&this->mutex); // 解锁
    LOG_WARN("No entry of %p in %s.", buf, this->name.c_str());
    return;
  }

  // 将内存块添加到空闲列表
  frees.push_back(buf);

  MUTEX_UNLOCK(&this->mutex); // 解锁
  return;  // TODO for test
}
}  // namespace common