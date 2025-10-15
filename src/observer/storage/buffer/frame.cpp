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
// Created by lianyu on 2022/10/29.
//

#include "storage/buffer/frame.h"
#include "session/session.h"
#include "session/thread_data.h"

/**
 * @brief FrameId的构造函数
 * @param buffer_pool_id BufferPool的标识符
 * @param page_num 页面编号
 */
FrameId::FrameId(int buffer_pool_id, PageNum page_num) : buffer_pool_id_(buffer_pool_id), page_num_(page_num) {}

/**
 * @brief 判断是否与另一个FrameId相等
 * @param other 要比较的另一个FrameId
 * @return 如果相等返回true，否则返回false
 */
bool FrameId::equal_to(const FrameId &other) const
{
  return buffer_pool_id_ == other.buffer_pool_id_ && page_num_ == other.page_num_;
}

/**
 * @brief 相等运算符重载
 * @param other 要比较的另一个FrameId
 * @return 如果相等返回true，否则返回false
 */
bool FrameId::operator==(const FrameId &other) const { return this->equal_to(other); }

/**
 * @brief 计算哈希值
 * @return 哈希值
 */
size_t FrameId::hash() const { return (static_cast<size_t>(buffer_pool_id_) << 32L) | page_num_; }

/**
 * @brief 获取BufferPool的标识符
 * @return BufferPool的标识符
 */
int     FrameId::buffer_pool_id() const { return buffer_pool_id_; }

/**
 * @brief 获取页面编号
 * @return 页面编号
 */
PageNum FrameId::page_num() const { return page_num_; }

/**
 * @brief 转换为字符串表示
 * @return 字符串表示
 */
string FrameId::to_string() const
{
  stringstream ss;
  ss << "buffer_pool_id:" << buffer_pool_id() << ",page_num:" << page_num();
  return ss.str();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief 获取默认的调试事务ID
 * @details 用于调试锁定机制的事务标识符获取函数
 * @return 事务标识符
 */
intptr_t get_default_debug_xid()
{
#if 0
  ThreadData *thd = ThreadData::current();
  intptr_t xid = (thd == nullptr) ? 
                 // pthread_self的返回值类型是pthread_t，pthread_t在linux和mac上不同
                 // 在Linux上是一个整数类型，而在mac上是一个指针。为了能在两个平台上都编译通过，
                 // 就将pthread_self返回值转换两次
                 reinterpret_cast<intptr_t>(reinterpret_cast<void*>(pthread_self())) : 
                 reinterpret_cast<intptr_t>(thd);
#endif
  Session *session = Session::current_session();
  if (session == nullptr) {
    return reinterpret_cast<intptr_t>(reinterpret_cast<void *>(pthread_self()));
  } else {
    return reinterpret_cast<intptr_t>(session);
  }
}

/**
 * @brief 获取写锁（使用默认事务ID）
 */
void Frame::write_latch() { write_latch(get_default_debug_xid()); }

/**
 * @brief 获取写锁
 * @param xid 事务ID
 */
void Frame::write_latch(intptr_t xid)
{
  {
    scoped_lock debug_lock(debug_lock_);
    ASSERT(pin_count_.load() > 0,
        "frame lock. write lock failed while pin count is invalid. "
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

    ASSERT(read_lockers_.find(xid) == read_lockers_.end(),
        "frame lock write while holding the read lock."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());
  }

  lock_.lock();

#ifdef DEBUG
  write_locker_ = xid;
  ++write_recursive_count_;
  TRACE("frame write lock success."
        "this=%p, pin=%d, frameId=%s, write locker=%lx(recursive=%d), xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), write_locker_, write_recursive_count_, xid, lbt());
#endif
}

/**
 * @brief 释放写锁（使用默认事务ID）
 */
void Frame::write_unlatch() { write_unlatch(get_default_debug_xid()); }

/**
 * @brief 释放写锁
 * @param xid 事务ID
 */
void Frame::write_unlatch(intptr_t xid)
{
  // 因为当前已经加着写锁，而且写锁只有一个，所以不再加debug_lock来做校验
  debug_lock_.lock();

  ASSERT(pin_count_.load() > 0,
      "frame lock. write unlock failed while pin count is invalid."
      "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
      this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

  ASSERT(write_locker_ == xid,
      "frame unlock write while not the owner."
      "write_locker=%lx, this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
      write_locker_, this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

  TRACE("frame write unlock success. this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

  if (--write_recursive_count_ == 0) {
    write_locker_ = 0;
  }
  debug_lock_.unlock();

  lock_.unlock();
}

/**
 * @brief 获取读锁（使用默认事务ID）
 */
void Frame::read_latch() { read_latch(get_default_debug_xid()); }

/**
 * @brief 获取读锁
 * @param xid 事务ID
 */
void Frame::read_latch(intptr_t xid)
{
  {
    scoped_lock debug_lock(debug_lock_);
    ASSERT(pin_count_ > 0,
        "frame lock. read lock failed while pin count is invalid."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

    ASSERT(xid != write_locker_,
        "frame lock read while holding the write lock."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());
  }

  lock_.lock_shared();

  {
#ifdef DEBUG
    scoped_lock debug_lock(debug_lock_);
    ++read_lockers_[xid];
    TRACE("frame read lock success."
          "this=%p, pin=%d, frameId=%s, xid=%lx, recursive=%d, lbt=%s",
          this, pin_count_.load(), frame_id_.to_string().c_str(), xid, read_lockers_[xid], lbt());
#endif
  }
}

/**
 * @brief 尝试获取读锁
 * @return 如果获取成功返回true，否则返回false
 */
bool Frame::try_read_latch()
{
  intptr_t xid = get_default_debug_xid();
  {
    scoped_lock debug_lock(debug_lock_);
    ASSERT(pin_count_ > 0,
        "frame try lock. read lock failed while pin count is invalid."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

    ASSERT(xid != write_locker_,
        "frame try to lock read while holding the write lock."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());
  }

  bool ret = lock_.try_lock_shared();
  if (ret) {
#ifdef DEBUG
    debug_lock_.lock();
    ++read_lockers_[xid];
    TRACE("frame read lock success."
          "this=%p, pin=%d, frameId=%s, xid=%lx, recursive=%d, lbt=%s",
          this, pin_count_.load(), frame_id_.to_string().c_str(), xid, read_lockers_[xid], lbt());
    debug_lock_.unlock();
#endif
  }

  return ret;
}

/**
 * @brief 释放读锁（使用默认事务ID）
 */
void Frame::read_unlatch() { read_unlatch(get_default_debug_xid()); }

/**
 * @brief 释放读锁
 * @param xid 事务ID
 */
void Frame::read_unlatch(intptr_t xid)
{
  {
    scoped_lock debug_lock(debug_lock_);
    ASSERT(pin_count_.load() > 0,
        "frame lock. read unlock failed while pin count is invalid."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

#ifdef DEBUG
    auto read_lock_iter  = read_lockers_.find(xid);
    int  recursive_count = read_lock_iter != read_lockers_.end() ? read_lock_iter->second : 0;
    ASSERT(recursive_count > 0,
        "frame unlock while not holding read lock."
        "this=%p, pin=%d, frameId=%s, xid=%lx, recursive=%d, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, recursive_count, lbt());

    if (1 == recursive_count) {
      read_lockers_.erase(xid);
    } else {
      read_lockers_[xid] = recursive_count - 1;
    }
#endif
  }

  TRACE("frame read unlock success."
        "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

  lock_.unlock_shared();
}

/**
 * @brief 增加页帧的引用计数
 * @details 当有用户使用该页帧时，需要增加引用计数，防止页帧被淘汰
 */
void Frame::pin()
{
  scoped_lock debug_lock(debug_lock_);

  [[maybe_unused]] intptr_t xid       = get_default_debug_xid();
  [[maybe_unused]] int      pin_count = ++pin_count_;

  TRACE("after frame pin. "
        "this=%p, write locker=%lx, read locker has xid %d? pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, write_locker_, read_lockers_.find(xid) != read_lockers_.end(), 
        pin_count, frame_id_.to_string().c_str(), xid, lbt());
}

/**
 * @brief 减少页帧的引用计数
 * @details 当用户不再使用该页帧时，减少引用计数
 * @return 如果引用计数变为0返回true，否则返回false
 */
int Frame::unpin()
{
  [[maybe_unused]] intptr_t xid = get_default_debug_xid();

  ASSERT(pin_count_.load() > 0,
      "try to unpin a frame that pin count <= 0."
      "this=%p, pin=%d, frameId=%s, xid=%lx, lbt=%s",
      this, pin_count_.load(), frame_id_.to_string().c_str(), xid, lbt());

  scoped_lock debug_lock(debug_lock_);

  int pin_count = --pin_count_;
  TRACE("after frame unpin. "
        "this=%p, write locker=%lx, read locker has xid? %d, pin=%d, frameId=%s, xid=%lx, lbt=%s",
        this, write_locker_, read_lockers_.find(xid) != read_lockers_.end(), 
        pin_count, frame_id_.to_string().c_str(), xid, lbt());

  if (0 == pin_count) {
    ASSERT(write_locker_ == 0,
           "frame unpin to 0 failed while someone hold the write lock. write locker=%lx, frameId=%s, xid=%lx",
           write_locker_, frame_id_.to_string().c_str(), xid);
    ASSERT(read_lockers_.empty(),
           "frame unpin to 0 failed while someone hold the read locks. reader num=%d, frameId=%s, xid=%lx",
           read_lockers_.size(), frame_id_.to_string().c_str(), xid);
  }
  return pin_count;
}

/**
 * @brief 获取当前时间的纳秒级时间戳
 * @return 当前时间的纳秒级时间戳
 */
unsigned long current_time()
{
  struct timespec tp;
  clock_gettime(CLOCK_MONOTONIC, &tp);
  return tp.tv_sec * 1000 * 1000 * 1000UL + tp.tv_nsec;
}

/**
 * @brief 标记页帧被访问
 * @details 更新页帧的访问时间，用于LRU算法
 */
void Frame::access() { acc_time_ = current_time(); }

/**
 * @brief 转换为字符串表示
 * @return 包含页帧信息的字符串
 */
string Frame::to_string() const
{
  stringstream ss;
  ss << "frame id:" << frame_id().to_string() << ", dirty=" << dirty() << ", pin=" << pin_count()
     << ", lsn=" << lsn() << ", this=" << this;
  return ss.str();
}
