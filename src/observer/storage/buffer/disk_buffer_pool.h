/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/4/13.
//
#pragma once

#include <fcntl.h>
#include <functional>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <optional>

#include "common/lang/bitmap.h"
#include "common/lang/lru_cache.h"
#include "common/lang/mutex.h"
#include "common/lang/memory.h"
#include "common/lang/unordered_map.h"
#include "common/mm/mem_pool.h"
#include "common/sys/rc.h"
#include "common/types.h"
#include "storage/buffer/frame.h"
#include "storage/buffer/page.h"
#include "storage/buffer/buffer_pool_log.h"

class BufferPoolManager;
class DiskBufferPool;
class DoubleWriteBuffer;
class LogHandler;
class BufferPoolLogHandler;

/**
 * @brief BufferPool 的实现
 * @defgroup BufferPool
 */

#define BP_FILE_SUB_HDR_SIZE (sizeof(BPFileSubHeader))

/**
 * @brief BufferPool的文件第一个页面，存放一些元数据信息，包括了后面每页的分配信息。
 * @ingroup BufferPool
 * @details
 * @code
 * TODO 1. 当前的做法，只能分配比较少的页面，你可以扩展一下，支持更多的页面或无限多的页面吗？
 *         可以参考Linux ext(n)和Windows NTFS等文件系统
 *      2. 当前使用bitmap存放页面分配情况，但是这种方法在页面非常多的时候，查找空闲页面的
 *         效率非常低，你有办法优化吗？
 * @endcode
 */
struct BPFileHeader
{
  int32_t buffer_pool_id;   //! buffer pool id
  int32_t page_count;       //! 当前文件一共有多少个页面
  int32_t allocated_pages;  //! 已经分配了多少个页面
  char    bitmap[0];        //! 页面分配位图, 第0个页面(就是当前页面)，总是1

  /**
   * 能够分配的最大的页面个数，即bitmap的字节数 乘以8
   */
  static const int MAX_PAGE_NUM = 
      (BP_PAGE_DATA_SIZE - sizeof(buffer_pool_id) - sizeof(page_count) - sizeof(allocated_pages)) * 8;

  /**
   * @brief 将BPFileHeader转换为字符串表示
   * @return 包含页面数量和已分配页面数量的字符串
   */
  string to_string() const;
};

/**
 * @brief 管理页面Frame
 * @ingroup BufferPool
 * @details 管理内存中的页帧。内存是有限的，内存中能够存放的页帧个数也是有限的。
 * 当内存中的页帧不够用时，需要从内存中淘汰一些页帧，以便为新的页帧腾出空间。
 * 这个管理器负责为所有的BufferPool提供页帧管理服务，也就是所有的BufferPool磁盘文件
 * 在访问时都使用这个管理器映射到内存。
 */
class BPFrameManager
{
public:
  /**
   * @brief 构造函数
   * @param tag 管理器的标签名
   */
  BPFrameManager(const char *tag);

  /**
   * @brief 初始化帧管理器
   * @param pool_num 帧池的大小
   * @return 初始化结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC init(int pool_num);
  
  /**
   * @brief 清理帧管理器资源
   * @return 清理结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC cleanup();

  /**
   * @brief 获取指定的页面
   * 
   * @param buffer_pool_id buffer Pool标识
   * @param page_num  页面号
   * @return Frame* 页帧指针，如果不存在则返回nullptr
   */
  Frame *get(int buffer_pool_id, PageNum page_num);

  /**
   * @brief 列出所有指定文件的页面
   * 
   * @param buffer_pool_id buffer Pool标识
   * @return list<Frame *> 页帧列表
   */
  list<Frame *> find_list(int buffer_pool_id);

  /**
   * @brief 分配一个新的页面
   * 
   * @param buffer_pool_id buffer Pool标识
   * @param page_num 页面编号
   * @return Frame* 页帧指针，如果分配失败则返回nullptr
   */
  Frame *alloc(int buffer_pool_id, PageNum page_num);

  /**
   * @brief 释放一个页面
   * 
   * 尽管frame中已经包含了buffer_pool_id和page_num，但是依然要求
   * 传入，因为frame可能忘记初始化或者没有初始化
   * 
   * @param buffer_pool_id buffer Pool标识
   * @param page_num 页面编号
   * @param frame 要释放的页帧指针
   * @return 释放结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC free(int buffer_pool_id, PageNum page_num, Frame *frame);

  /**
   * @brief 清理一些页面以腾出空间
   * 
   * 如果不能从空闲链表中分配新的页面，就使用这个接口，
   * 尝试从pin count=0的页面中淘汰一些
   * 
   * @param count 想要purge多少个页面
   * @param purger 需要在释放frame之前，对页面做些什么操作。当前是刷新脏数据到磁盘
   * @return 返回本次清理了多少个页面
   */
  int purge_frames(int count, function<RC(Frame *frame)> purger);

  /**
   * @brief 获取当前帧管理器中的帧数量
   * @return 帧数量
   */
  size_t frame_num() const { return frames_.count(); }

  /**
   * @brief 测试使用。返回已经从内存申请的个数
   * @return 已分配的总帧数量
   */
  size_t total_frame_num() const { return allocator_.get_size(); }

private:
  /**
   * @brief 内部使用的获取帧的方法
   * @param frame_id 帧ID
   * @return 帧指针
   */
  Frame *get_internal(const FrameId &frame_id);
  
  /**
   * @brief 内部使用的释放帧的方法
   * @param frame_id 帧ID
   * @param frame 帧指针
   * @return 释放结果
   */
  RC free_internal(const FrameId &frame_id, Frame *frame);

private:
  class BPFrameIdHasher
  {
  public:
    size_t operator()(const FrameId &frame_id) const { return frame_id.hash(); }
  };

  using FrameLruCache  = common::LruCache<FrameId, Frame *, BPFrameIdHasher>;
  using FrameAllocator = common::MemPoolSimple<Frame>;

  mutex          lock_;         //! 保护帧管理器的互斥锁
  FrameLruCache  frames_;       //! 页帧的LRU缓存
  FrameAllocator allocator_;    //! 页帧分配器
};

/**
 * @brief 用于遍历BufferPool中的所有页面
 * @ingroup BufferPool
 */
class BufferPoolIterator
{
public:
  /**
   * @brief 构造函数
   */
  BufferPoolIterator();
  
  /**
   * @brief 析构函数
   */
  ~BufferPoolIterator();

  /**
   * @brief 初始化迭代器
   * @param bp 要遍历的DiskBufferPool对象
   * @param start_page 起始页面号，默认为0
   * @return 初始化结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC      init(DiskBufferPool &bp, PageNum start_page = 0);
  
  /**
   * @brief 检查是否还有下一个页面
   * @return 如果有下一个页面则返回true，否则返回false
   */
  bool    has_next();
  
  /**
   * @brief 获取下一个页面的编号
   * @return 下一个页面的编号，如果没有则返回-1
   */
  PageNum next();
  
  /**
   * @brief 重置迭代器到起始位置
   * @return 重置结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC      reset();

private:
  common::Bitmap bitmap_;       //! 页面分配位图，用于判断页面是否已分配
  PageNum        current_page_num_ = -1;  //! 当前页面编号
};

/**
 * @brief BufferPool的实现
 * @ingroup BufferPool
 * @details 一个文件被划分成多个相同大小的页面，并在需要访问的时候，会从文件读取到内存中。
 * DiskBufferPool 就负责管理磁盘文件，以及负责管理页面在文件与内存中的交互，比如读取、写回。
 */
class DiskBufferPool final
{
public:
  /**
   * @brief 构造函数
   * @param bp_manager BufferPool管理器
   * @param frame_manager 页帧管理器
   * @param dblwr_manager 双写缓冲区管理器
   * @param log_handler 日志处理器
   */
  DiskBufferPool(BufferPoolManager &bp_manager, BPFrameManager &frame_manager, DoubleWriteBuffer &dblwr_manager,
      LogHandler &log_handler);
  
  /**
   * @brief 析构函数
   */
  ~DiskBufferPool();

  /**
   * @brief 根据文件名打开一个分页文件
   * @param file_name 文件名
   * @return 打开结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC open_file(const char *file_name);

  /**
   * @brief 关闭分页文件
   * @return 关闭结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC close_file();

  /**
   * @brief 根据文件ID和页号获取指定页面到缓冲区，返回页面句柄指针。
   * @param page_num 页面编号
   * @param frame 输出参数，用于存储获取到的页帧指针
   * @return 获取结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC get_this_page(PageNum page_num, Frame **frame);

  /**
   * @brief 在指定文件中分配一个新的页面，并将其放入缓冲区，返回页面句柄指针。
   * @details 分配页面时，如果文件中有空闲页，就直接分配一个空闲页；
   * 如果文件中没有空闲页，则扩展文件规模来增加新的空闲页。
   * @param frame 输出参数，用于存储分配到的页帧指针
   * @return 分配结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC allocate_page(Frame **frame);

  /**
   * @brief 释放某个页面，将此页面设置为未分配状态
   * 
   * @param page_num 待释放的页面
   * @return 释放结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC dispose_page(PageNum page_num);

  /**
   * @brief 释放指定文件关联的页的内存
   * 如果已经脏， 则刷到磁盘，除了pinned page
   * @param page_num 页面编号
   * @return 释放结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC purge_page(PageNum page_num);
  
  /**
   * @brief 释放所有页面的内存
   * @return 释放结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC purge_all_pages();

  /**
   * @brief 用于解除pageHandle对应页面的驻留缓冲区限制
   * 
   * 在调用GetThisPage或AllocatePage函数将一个页面读入缓冲区后，
   * 该页面被设置为驻留缓冲区状态，以防止其在处理过程中被置换出去，
   * 因此在该页面使用完之后应调用此函数解除该限制，使得该页面此后可以正常地被淘汰出缓冲区
   * 
   * @param frame 要解除限制的页帧
   * @return 解除限制结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC unpin_page(Frame *frame);

  /**
   * @brief 检查是否所有页面都是pin count == 0状态(除了第1个页面)
   * 调试使用
   * @return 检查结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC check_all_pages_unpinned();

  /**
   * @brief 获取文件描述符
   * @return 文件描述符
   */
  int file_desc() const;

  /**
   * @brief 如果页面是脏的，就将数据刷新到double write buffer
   * @param frame 要刷新的页帧
   * @return 刷新结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC flush_page(Frame &frame);

  /**
   * @brief 刷新所有页面到double write buffer，即使pin count不是0
   * @return 刷新结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC flush_all_pages();

  /**
   * @brief 回放日志时处理page0中已被认定为不存在的page
   * @param page_num 页面编号
   * @return 处理结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC recover_page(PageNum page_num);

  /**
   * @brief 刷新页面到磁盘
   * @param page_num 页面编号
   * @param page 页面数据
   * @return 刷新结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC write_page(PageNum page_num, Page &page);

  /**
   * @brief 重做页面分配操作
   * @param lsn 日志序列号
   * @param page_num 页面编号
   * @return 重做结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC redo_allocate_page(LSN lsn, PageNum page_num);
  
  /**
   * @brief 重做页面释放操作
   * @param lsn 日志序列号
   * @param page_num 页面编号
   * @return 重做结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC redo_deallocate_page(LSN lsn, PageNum page_num);

public:
  /**
   * @brief 获取BufferPool的ID
   * @return BufferPool的ID
   */
  int32_t id() const { return buffer_pool_id_; }

  /**
   * @brief 获取文件名
   * @return 文件名
   */
  const char *filename() const { return file_name_.c_str(); }

protected:
  /**
   * @brief 分配一个帧
   * @param page_num 页面编号
   * @param buf 输出参数，用于存储分配到的帧指针
   * @return 分配结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC allocate_frame(PageNum page_num, Frame **buf);

  /**
   * @brief 刷新指定页面到磁盘(flush)，并且释放关联的Frame
   * @param page_num 页面编号
   * @param used_frame 要释放的帧
   * @return 释放结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC purge_frame(PageNum page_num, Frame *used_frame);
  
  /**
   * @brief 检查页面编号是否有效
   * @param page_num 页面编号
   * @return 检查结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC check_page_num(PageNum page_num);

  /**
   * @brief 加载指定页面的数据到内存中
   * @param page_num 页面编号
   * @param frame 要加载数据的帧
   * @return 加载结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC load_page(PageNum page_num, Frame *frame);

  /**
   * @brief 如果页面是脏的，就将数据刷新到磁盘
   * @param frame 要刷新的帧
   * @return 刷新结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC flush_page_internal(Frame &frame);

private:
  BufferPoolManager   &bp_manager_;     /// BufferPool 管理器
  BPFrameManager      &frame_manager_;  /// Frame 管理器
  DoubleWriteBuffer   &dblwr_manager_;  /// Double Write Buffer 管理器
  BufferPoolLogHandler log_handler_;    /// BufferPool 日志处理器

  int file_desc_ = -1;  /// 文件描述符
  /// 由于在最开始打开文件时，没有正确的buffer pool id不能加载header frame，所以单独从文件中读取此标识
  int32_t       buffer_pool_id_ = -1; 
  Frame        *hdr_frame_      = nullptr;  /// 文件头页面
  BPFileHeader *file_header_    = nullptr;  /// 文件头
  set<PageNum>  disposed_pages_;            /// 已经释放的页面

  string file_name_;  /// 文件名

  common::Mutex lock_;    /// 保护文件操作的互斥锁
  common::Mutex wr_lock_; /// 保护写操作的互斥锁

private:
  friend class BufferPoolIterator;
};

/**
 * @brief BufferPool的管理类
 * @ingroup BufferPool
 */
class BufferPoolManager final
{
public:
  /**
   * @brief 构造函数
   * @param memory_size 内存大小，默认为0
   */
  BufferPoolManager(int memory_size = 0);
  
  /**
   * @brief 析构函数
   */
  ~BufferPoolManager();

  /**
   * @brief 初始化BufferPool管理器
   * @param dblwr_buffer 双写缓冲区
   * @return 初始化结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC init(unique_ptr<DoubleWriteBuffer> dblwr_buffer);

  /**
   * @brief 创建一个新的文件
   * @param file_name 文件名
   * @return 创建结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC create_file(const char *file_name);
  
  /**
   * @brief 打开一个文件
   * @param log_handler 日志处理器
   * @param file_name 文件名
   * @param bp 输出参数，用于存储打开的DiskBufferPool指针
   * @return 打开结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC open_file(LogHandler &log_handler, const char *file_name, DiskBufferPool *&bp);
  
  /**
   * @brief 关闭一个文件
   * @param file_name 文件名
   * @return 关闭结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC close_file(const char *file_name);

  /**
   * @brief 刷新一个页面
   * @param frame 要刷新的帧
   * @return 刷新结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC flush_page(Frame &frame);

  /**
   * @brief 获取帧管理器
   * @return 帧管理器的引用
   */
  BPFrameManager    &get_frame_manager() { return frame_manager_; }
  
  /**
   * @brief 获取双写缓冲区
   * @return 双写缓冲区的指针
   */
  DoubleWriteBuffer *get_dblwr_buffer() { return dblwr_buffer_.get(); }

  /**
   * @brief 根据ID获取对应的BufferPool对象
   * @details 在做redo时，需要根据ID获取对应的BufferPool对象，然后让bufferPool对象自己做redo
   * @param id buffer pool id
   * @param bp buffer pool 对象
   * @return 获取结果，成功返回RC::SUCCESS，否则返回错误码
   */
  RC get_buffer_pool(int32_t id, DiskBufferPool *&bp);

private:
  BPFrameManager frame_manager_{"BufPool"};  /// 帧管理器

  unique_ptr<DoubleWriteBuffer> dblwr_buffer_;  /// 双写缓冲区

  common::Mutex                            lock_;                /// 保护BufferPoolManager的互斥锁
  unordered_map<string, DiskBufferPool *>  buffer_pools_;        /// 文件名到BufferPool的映射
  unordered_map<int32_t, DiskBufferPool *> id_to_buffer_pools_;  /// ID到BufferPool的映射
  atomic<int32_t>                          next_buffer_pool_id_{1};  // 系统启动时，会打开所有的表，这样就可以知道当前系统最大的ID是多少了
};
