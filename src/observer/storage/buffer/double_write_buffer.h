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
// Created by Wenbin1002 on 2024/04/16
//

#pragma once

#include "common/lang/mutex.h"
#include "common/lang/unordered_map.h"
#include "common/types.h"
#include "common/sys/rc.h"
#include "storage/buffer/page.h"

class DiskBufferPool;
struct DoubleWritePage;
class BufferPoolManager;

/**
 * @brief 双写缓冲区接口类
 * @ingroup BufferPool
 * @details 定义了双写缓冲区的基本操作接口，用于解决页面原子写入的问题
 */
class DoubleWriteBuffer
{
public:
  /**
   * @brief 默认构造函数
   */
  DoubleWriteBuffer()          = default;
  /**
   * @brief 虚析构函数
   */
  virtual ~DoubleWriteBuffer() = default;

  /**
   * @brief 将页面加入buffer，并且写入磁盘中的共享表空间
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 页面数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) = 0;

  /**
   * @brief 从双写缓冲区中读取页面
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 用于存储读取的页面数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) = 0;

  /**
   * @brief 清空所有与指定buffer pool关联的页面
   * @param bp 磁盘缓冲区池指针
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC clear_pages(DiskBufferPool *bp) = 0;
};

/**
 * @brief 双写缓冲区头部结构
 * @details 用于存储双写缓冲区文件的元数据
 */
struct DoubleWriteBufferHeader
{
  /**
   * @brief 页面数量
   */
  int32_t page_cnt = 0;

  /**
   * @brief 头部结构大小
   */
  static const int32_t SIZE;
};

/**
 * @brief 双写页面键结构
 * @details 用于在哈希表中唯一标识一个双写页面
 * @note TODO change to FrameId
 */
struct DoubleWritePageKey
{
  /**
   * @brief 缓冲区池ID
   */
  int32_t buffer_pool_id;
  /**
   * @brief 页面编号
   */
  PageNum page_num;

  /**
   * @brief 相等运算符重载
   * @param other 要比较的另一个DoubleWritePageKey
   * @return 如果相等返回true，否则返回false
   */
  bool operator==(const DoubleWritePageKey &other) const
  {
    return buffer_pool_id == other.buffer_pool_id && page_num == other.page_num;
  }
};

/**
 * @brief DoubleWritePageKey的哈希函数结构
 * @details 用于计算DoubleWritePageKey的哈希值
 */
struct DoubleWritePageKeyHash
{
  /**
   * @brief 计算哈希值
   * @param key 要计算哈希的DoubleWritePageKey
   * @return 哈希值
   */
  size_t operator()(const DoubleWritePageKey &key) const
  {
    return hash<int32_t>()(key.buffer_pool_id) ^ hash<PageNum>()(key.page_num);
  }
};

/**
 * @brief 页面二次缓冲区，为了解决页面原子写入的问题
 * @ingroup BufferPool
 * @details 一个页面通常比较大，不能保证要么都写入磁盘成功，要么不写入磁盘。如果存在写入一部分的情况，
 * 我们应该有手段检测出来，否则会遇到灾难性的数据不一致问题。
 * 这里的解决方案是在我们写入真实页面数据之前，先将数据放入一个公共的缓冲区，也就是DoubleWriteBuffer，
 * DoubleWriteBuffer会先在一个共享磁盘文件中写入页面数据，在确定写入成功后，再写入真实的页面。
 * 当我们从磁盘中读取页面时，会校验页面的checksum，如果校验失败，则说明页面写入不完整，这时候可以从
 * DoubleWriteBuffer中读取数据。
 *
 * @note 每次都要保证，不管在内存中还是在文件中，这里的数据都是最新的，都比Buffer pool中的数据要新
 */
class DiskDoubleWriteBuffer : public DoubleWriteBuffer
{
public:
  /**
   * @brief 构造函数
   *
   * @param bp_manager 关联的buffer pool manager
   * @param max_pages  内存中保存的最大页面数
   */
  DiskDoubleWriteBuffer(BufferPoolManager &bp_manager, int max_pages = 16);
  /**
   * @brief 析构函数
   */
  virtual ~DiskDoubleWriteBuffer();

  /**
   * @brief 打开磁盘中的共享表空间文件
   * @param filename 文件名
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC open_file(const char *filename);

  /**
   * @brief 将buffer中的页全部写入磁盘，并且清空buffer
   * @details TODO 目前的解决方案是等buffer装满后再刷盘，可能会导致程序卡住一段时间
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC flush_page();

  /**
   * @brief 将页面加入buffer，并且写入磁盘中的共享表空间
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 页面数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  /**
   * @brief 从双写缓冲区中读取页面
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 用于存储读取的页面数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  /**
   * @brief 清空所有与指定buffer pool关联的页面
   * @param bp 磁盘缓冲区池指针
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC clear_pages(DiskBufferPool *bp) override;

  /**
   * @brief 恢复双写缓冲区中的页面
   * @details 将双写缓冲区中的页面写入对应的磁盘缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC recover();

private:
  /**
   * @brief 将buffer中的页面写入对应的磁盘
   * @param page 双写页面指针
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC write_page(DoubleWritePage *page);

  /**
   * @brief 将页面写到当前double write buffer文件中
   * @details 每次页面更新都应该写入到磁盘中。保证double write buffer
   * 内存和文件中的数据都是最新的。
   * @param page 双写页面指针
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC write_page_internal(DoubleWritePage *page);

  /**
   * @brief 将磁盘文件中的内容加载到内存中。在启动时调用
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC load_pages();

private:
  /**
   * @brief 文件描述符
   */
  int                     file_desc_ = -1;
  /**
   * @brief 内存中保存的最大页面数
   */
  int                     max_pages_ = 0;
  /**
   * @brief 互斥锁，用于保护双写缓冲区的并发访问
   */
  common::Mutex           lock_;
  /**
   * @brief 关联的缓冲区池管理器
   */
  BufferPoolManager      &bp_manager_;
  /**
   * @brief 双写缓冲区头部信息
   */
  DoubleWriteBufferHeader header_;

  /**
   * @brief 双写页面哈希表，用于快速查找页面
   */
  unordered_map<DoubleWritePageKey, DoubleWritePage *, DoubleWritePageKeyHash> dblwr_pages_;
};

/**
 * @brief 空实现的双写缓冲区
 * @details 不进行实际的双写操作，直接将页面写入磁盘
 */
class VacuousDoubleWriteBuffer : public DoubleWriteBuffer
{
public:
  /**
   * @brief 析构函数
   */
  virtual ~VacuousDoubleWriteBuffer() = default;

  /**
   * @brief 将页面直接写入磁盘，不经过双写缓冲区
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 页面数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC add_page(DiskBufferPool *bp, PageNum page_num, Page &page) override;

  /**
   * @brief 从磁盘读取页面
   * @param bp 磁盘缓冲区池指针
   * @param page_num 页面编号
   * @param page 用于存储读取的页面数据
   * @return 操作结果，总是返回RC::BUFFERPOOL_INVALID_PAGE_NUM
   */
  RC read_page(DiskBufferPool *bp, PageNum page_num, Page &page) override { return RC::BUFFERPOOL_INVALID_PAGE_NUM; }

  /**
   * @brief 清空指定buffer pool的页面（空实现）
   * @param bp 磁盘缓冲区池指针
   * @return 操作结果，总是返回RC::SUCCESS
   */
  RC clear_pages(DiskBufferPool *bp) override { return RC::SUCCESS; }
};
