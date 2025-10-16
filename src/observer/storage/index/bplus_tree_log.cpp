/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/**
 * @file bplus_tree_log.cpp
 * @brief B+树日志管理和事务处理实现
 * 
 * @details 
 * 本文件实现了B+树的日志管理系统，主要包含以下功能模块：
 * 
 * 1. **BplusTreeLogger类**：负责记录B+树各种操作的日志，支持日志的追加、提交、回滚和重做
 * 2. **BplusTreeMiniTransaction类**：实现B+树的迷你事务机制，提供自动提交/回滚功能
 * 3. **BplusTreeLogReplayer类**：负责重放B+树操作日志，用于系统重启后的数据库恢复
 * 
 * 在整个数据库系统中，这些模块的作用是：
 * - 保证B+树操作的原子性和持久性
 * - 支持事务的提交和回滚
 * - 提供崩溃恢复机制，确保数据一致性
 * - 通过日志先行（WAL，Write-Ahead Logging）机制保障数据安全
 */

//
// Created by wangyunlai.wyl on 2024/02/05.
//

#include "common/log/log.h"
#include "common/lang/defer.h"
#include "common/lang/algorithm.h"
#include "common/lang/sstream.h"
#include "storage/index/bplus_tree_log.h"
#include "storage/index/bplus_tree.h"
#include "storage/clog/log_handler.h"
#include "storage/clog/log_entry.h"
#include "storage/index/bplus_tree_log_entry.h"
#include "common/lang/serializer.h"
#include "storage/clog/vacuous_log_handler.h"

using namespace common;
using namespace bplus_tree;

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeLogger
/**
 * @class BplusTreeLogger
 * @brief B+树操作日志记录器
 * @details 负责记录B+树的所有修改操作，支持日志的追加、提交、回滚和重做
 */

/**
 * @brief BplusTreeLogger构造函数
 * @details 初始化日志记录器，设置日志处理器和缓冲池ID
 * @param log_handler 日志处理器对象引用
 * @param buffer_pool_id 缓冲池ID
 */
BplusTreeLogger::BplusTreeLogger(LogHandler &log_handler, int32_t buffer_pool_id)
    : log_handler_(log_handler), buffer_pool_id_(buffer_pool_id)
{}

/**
 * @brief BplusTreeLogger析构函数
 * @details 清理日志记录器资源
 */
BplusTreeLogger::~BplusTreeLogger() {}

/**
 * @brief 记录初始化文件头页面的日志
 * @details 创建并追加初始化文件头页面的日志条目
 * @param frame 页面帧指针
 * @param header 索引文件头信息
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::init_header_page(Frame *frame, const IndexFileHeader &header)
{
  return append_log_entry(make_unique<InitHeaderPageLogEntryHandler>(frame, header));
}

/**
 * @brief 记录更新根页面的日志
 * @details 创建并追加更新根页面的日志条目
 * @param frame 页面帧指针
 * @param root_page_num 新的根页面编号
 * @param old_page_num 旧的根页面编号
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::update_root_page(Frame *frame, PageNum root_page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<UpdateRootPageLogEntryHandler>(frame, root_page_num, old_page_num));
}

/**
 * @brief 记录初始化空叶子节点的日志
 * @details 创建并追加初始化空叶子节点的日志条目
 * @param node_handler 节点处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::leaf_init_empty(IndexNodeHandler &node_handler)
{
  return append_log_entry(make_unique<LeafInitEmptyLogEntryHandler>(node_handler.frame()));
}

/**
 * @brief 记录节点插入元素的日志
 * @details 创建并追加节点插入元素的日志条目
 * @param node_handler 节点处理器对象
 * @param index 插入位置的索引
 * @param items 要插入的元素数据
 * @param item_num 元素数量
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::node_insert_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num)
{
  return append_log_entry(make_unique<NormalOperationLogEntryHandler>(
      node_handler.frame(), LogOperation::Type::NODE_INSERT, index, items, item_num));
}

/**
 * @brief 记录节点删除元素的日志
 * @details 创建并追加节点删除元素的日志条目
 * @param node_handler 节点处理器对象
 * @param index 删除位置的索引
 * @param items 被删除的元素数据
 * @param item_num 元素数量
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::node_remove_items(IndexNodeHandler &node_handler, int index, span<const char> items, int item_num)
{
  return append_log_entry(make_unique<NormalOperationLogEntryHandler>(
      node_handler.frame(), LogOperation::Type::NODE_REMOVE, index, items, item_num));
}

/**
 * @brief 记录设置叶子节点下一页的日志
 * @details 创建并追加设置叶子节点下一页的日志条目
 * @param node_handler 节点处理器对象
 * @param page_num 新的下一页编号
 * @param old_page_num 旧的下一页编号
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::leaf_set_next_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<LeafSetNextPageLogEntryHandler>(node_handler.frame(), page_num, old_page_num));
}

/**
 * @brief 记录初始化空内部节点的日志
 * @details 创建并追加初始化空内部节点的日志条目
 * @param node_handler 节点处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::internal_init_empty(IndexNodeHandler &node_handler)
{
  return append_log_entry(make_unique<InternalInitEmptyLogEntryHandler>(node_handler.frame()));
}

/**
 * @brief 记录创建新根节点的日志
 * @details 创建并追加创建新根节点的日志条目
 * @param node_handler 节点处理器对象
 * @param first_page_num 第一个子节点页面编号
 * @param key 分界键值
 * @param page_num 第二个子节点页面编号
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::internal_create_new_root(
    IndexNodeHandler &node_handler, PageNum first_page_num, span<const char> key, PageNum page_num)
{
  return append_log_entry(
      make_unique<InternalCreateNewRootLogEntryHandler>(node_handler.frame(), first_page_num, key, page_num));
}

/**
 * @brief 记录更新内部节点键值的日志
 * @details 创建并追加更新内部节点键值的日志条目
 * @param node_handler 节点处理器对象
 * @param index 键值的索引位置
 * @param key 新的键值数据
 * @param old_key 旧的键值数据
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::internal_update_key(
    IndexNodeHandler &node_handler, int index, span<const char> key, span<const char> old_key)
{
  return append_log_entry(make_unique<InternalUpdateKeyLogEntryHandler>(node_handler.frame(), index, key, old_key));
}

/**
 * @brief 记录设置父节点页面的日志
 * @details 创建并追加设置父节点页面的日志条目
 * @param node_handler 节点处理器对象
 * @param page_num 新的父节点页面编号
 * @param old_page_num 旧的父节点页面编号
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::set_parent_page(IndexNodeHandler &node_handler, PageNum page_num, PageNum old_page_num)
{
  return append_log_entry(make_unique<SetParentPageLogEntryHandler>(node_handler.frame(), page_num, old_page_num));
}

/**
 * @brief 追加日志条目到日志列表
 * @details 将日志条目添加到内部日志列表中，待后续提交
 * @param entry 日志条目对象指针
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::append_log_entry(unique_ptr<bplus_tree::LogEntryHandler> entry)
{
  if (!need_log_) {
    return RC::SUCCESS;
  }

  entries_.push_back(std::move(entry));
  return RC::SUCCESS;
}

/**
 * @brief 提交所有日志条目
 * @details 将内部日志列表中的所有日志条目序列化并写入日志存储，然后更新页面的LSN
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::commit()
{
  if (entries_.empty()) {
    return RC::SUCCESS;
  }

  LSN        lsn = 0;
  Serializer buffer;
  buffer.write_int32(buffer_pool_id_);

  for (auto &entry : entries_) {
    entry->serialize(buffer);
  }

  Serializer::BufferType &buffer_data = buffer.data();

  RC rc = log_handler_.append(lsn, LogModule::Id::BPLUS_TREE, std::move(buffer_data));
  if (RC::SUCCESS != rc) {
    LOG_WARN("failed to append log entry. rc=%s", strrc(rc));
    return rc;
  }

  for (auto &entry : entries_) {
    entry->frame()->set_lsn(lsn);
  }

  entries_.clear();
  return RC::SUCCESS;
}

/**
 * @brief 回滚所有日志条目
 * @details 逆序执行所有日志条目的回滚操作，恢复到操作前的状态
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  need_log_ = false;

  for (auto iter = entries_.rbegin(), itend = entries_.rend(); iter != itend; ++iter) {
    auto &entry = *iter;
    entry->rollback(mtr, tree_handler);
  }

  entries_.clear();
  need_log_ = true;
  return RC::SUCCESS;
}

/**
 * @brief 重做日志条目
 * @details 从日志条目中恢复B+树的状态，用于系统重启后的数据库恢复
 * @param bpm 缓冲池管理器对象
 * @param entry 日志条目对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::redo(BufferPoolManager &bpm, const LogEntry &entry)
{
  ASSERT(entry.module().id() == LogModule::Id::BPLUS_TREE, "invalid log entry: %s", entry.to_string().c_str());

  Deserializer buffer(entry.data(), entry.payload_size());
  int32_t      buffer_pool_id = -1;
  int          ret            = buffer.read_int32(buffer_pool_id);
  if (ret != 0) {
    LOG_ERROR("failed to read buffer pool id. ret=%d", ret);
    return RC::IOERR_READ;
  }

  DiskBufferPool *buffer_pool = nullptr;
  RC              rc          = bpm.get_buffer_pool(buffer_pool_id, buffer_pool);
  if (OB_FAIL(rc) || buffer_pool == nullptr) {
    LOG_WARN("failed to get buffer pool. rc=%s, buffer_pool_id=%d", strrc(rc), buffer_pool_id);
    return rc;
  }

  VacuousLogHandler log_handler;
  BplusTreeHandler  tree_handler;
  rc = tree_handler.open(log_handler, *buffer_pool);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to open bplus tree handler. rc=%s", strrc(rc));
    return rc;
  }

  BplusTreeMiniTransaction mtr(tree_handler);
  rc = mtr.logger().__redo(entry.lsn(), mtr, tree_handler, buffer);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to redo log entry. rc=%s", strrc(rc));
    return rc;
  }

  ASSERT(mtr.logger().entries_.empty(), "entries should be empty after redo");
  return rc;
}

/**
 * @brief 内部重做方法
 * @details 实际执行日志重做操作的内部方法
 * @param lsn 日志序列号
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @param redo_buffer 重做数据缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogger::__redo(LSN lsn, BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler, Deserializer &redo_buffer)
{
  need_log_ = false;

  DEFER(need_log_ = true);

  RC rc = RC::SUCCESS;
  vector<Frame *> frames;
  while (redo_buffer.remain() > 0) {
    unique_ptr<LogEntryHandler> entry;

    rc = LogEntryHandler::from_buffer(tree_handler.buffer_pool(), redo_buffer, entry);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to deserialize log entry. rc=%s", strrc(rc));
      break;
    }
    Frame *frame = entry->frame();
    if (frame != nullptr) {
      if (frame->lsn() >= lsn) {
        LOG_TRACE("no need to redo. frame=%p:%s, redo lsn=%ld", frame, frame->to_string().c_str(), lsn);
        frame->unpin();
        continue;
      } else {
        frames.push_back(frame);
      }
    } else {
      LOG_TRACE("frame is null, skip the redo action");
      continue;
    }

    rc = entry->redo(mtr, tree_handler);
    if (OB_FAIL(rc)) {
      LOG_WARN("failed to redo log entry. rc=%s, lsn=%d, entry=%s", strrc(rc), lsn, entry->to_string().c_str());
      break;
    }

    // 在这里能设置frame的LSN，因为一个页面可能会有多个操作
    // frame->set_lsn(lsn);
  }

  if (OB_SUCC(rc)) {
    for (Frame *frame : frames) {
      frame->set_lsn(lsn);
      frame->unpin();
    }
  }

  return RC::SUCCESS;
}

/**
 * @brief 将日志条目转换为字符串表示
 * @details 生成包含日志条目详细信息的字符串描述
 * @param entry 日志条目对象
 * @return 日志条目的字符串描述
 */
string BplusTreeLogger::log_entry_to_string(const LogEntry &entry)
{
  stringstream ss;
  Deserializer buffer(entry.data(), entry.payload_size());
  int32_t      buffer_pool_id = -1;
  int          ret            = buffer.read_int32(buffer_pool_id);
  if (ret != 0) {
    LOG_ERROR("failed to read buffer pool id. ret=%d", ret);
    return ss.str();
  }

  ss << "buffer_pool_id:" << buffer_pool_id;
  while (buffer.remain() > 0) {
    unique_ptr<LogEntryHandler> entry;

    RC rc = LogEntryHandler::from_buffer(buffer, entry);
    if (RC::SUCCESS != rc) {
      LOG_WARN("failed to deserialize log entry. rc=%s", strrc(rc));
      return ss.str();
    }

    ss << ",";
    ss << entry->to_string();
  }
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeMiniTransaction
/**
 * @class BplusTreeMiniTransaction
 * @brief B+树迷你事务
 * @details 实现B+树的轻量级事务支持，提供自动提交/回滚功能
 */

/**
 * @brief BplusTreeMiniTransaction构造函数
 * @details 初始化B+树迷你事务对象
 * @param tree_handler B+树处理器对象引用
 * @param operation_result 操作结果指针，用于自动提交或回滚
 */
BplusTreeMiniTransaction::BplusTreeMiniTransaction(BplusTreeHandler &tree_handler, RC *operation_result /* =nullptr */)
    : tree_handler_(tree_handler),
      operation_result_(operation_result),
      latch_memo_(&tree_handler.buffer_pool()),
      logger_(tree_handler.log_handler(), tree_handler.buffer_pool().id())
{}

/**
 * @brief BplusTreeMiniTransaction析构函数
 * @details 根据操作结果自动提交或回滚事务
 */
BplusTreeMiniTransaction::~BplusTreeMiniTransaction()
{
  if (nullptr == operation_result_) {
    return;
  }
  
  if (OB_SUCC(*operation_result_)) {
    commit();
  } else {
    rollback();
  }
}

/**
 * @brief 提交事务
 * @details 提交所有日志条目
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeMiniTransaction::commit() { return logger_.commit(); }

/**
 * @brief 回滚事务
 * @details 回滚所有已记录的操作
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeMiniTransaction::rollback() { return logger_.rollback(*this, tree_handler_); }

///////////////////////////////////////////////////////////////////////////////
// class BplusTreeLogReplayer
/**
 * @class BplusTreeLogReplayer
 * @brief B+树日志重放器
 * @details 负责重放B+树操作日志，用于系统重启后的数据库恢复
 */

/**
 * @brief BplusTreeLogReplayer构造函数
 * @details 初始化B+树日志重放器
 * @param bpm 缓冲池管理器对象引用
 */
BplusTreeLogReplayer::BplusTreeLogReplayer(BufferPoolManager &bpm) : buffer_pool_manager_(bpm) {}

/**
 * @brief 重放日志条目
 * @details 调用BplusTreeLogger::redo方法重放日志条目
 * @param entry 日志条目对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC BplusTreeLogReplayer::replay(const LogEntry &entry) { return BplusTreeLogger::redo(buffer_pool_manager_, entry); }
