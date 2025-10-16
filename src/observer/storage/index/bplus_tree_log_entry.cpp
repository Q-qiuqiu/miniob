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
 * @file bplus_tree_log_entry.cpp
 * @brief B+树日志条目处理实现
 * 
 * @details 
 * 本文件和bplus_tree_log_entry.h共同实现了B+树操作日志的记录、序列化、反序列化、回滚和重做功能，
 * 在整个数据库中起到以下重要作用：
 * 
 * 1. **事务恢复支持**：记录B+树所有修改操作的日志，在系统崩溃后重启时可以通过重放日志恢复数据库到一致状态
 * 2. **事务回滚支持**：保存操作前的状态信息，允许事务在需要时回滚到操作前的状态
 * 3. **数据一致性保证**：通过日志机制确保B+树操作的原子性和持久性
 * 4. **模块化设计**：为每种B+树操作类型设计了专门的日志处理类，便于扩展和维护
 * 
 * 日志系统设计采用了命令模式，每种B+树操作对应一个日志处理类，都实现了统一的接口，支持序列化、反序列化、
 * 回滚和重做操作。这种设计使得日志系统能够灵活地处理各种B+树操作，同时保持良好的可扩展性。
 */

//
// Created by wangyunlai.wyl on 2024/02/20.
//

#include "storage/index/bplus_tree_log_entry.h"
#include "common/lang/serializer.h"

using namespace common;

namespace bplus_tree {

/**
 * @brief 将日志操作类型转换为字符串表示
 * @details 生成一个包含操作类型索引和名称的字符串
 * @return 日志操作类型的字符串描述
 */
string LogOperation::to_string() const
{
  stringstream ss;
  ss << std::to_string(index()) << ":";
  switch (type_) {
    case Type::INIT_HEADER_PAGE: ss << "INIT_HEADER_PAGE"; break;
    case Type::UPDATE_ROOT_PAGE: ss << "UPDATE_ROOT_PAGE"; break;
    case Type::SET_PARENT_PAGE: ss << "SET_PARENT_PAGE"; break;
    case Type::LEAF_INIT_EMPTY: ss << "LEAF_INIT_EMPTY"; break;
    case Type::LEAF_SET_NEXT_PAGE: ss << "LEAF_SET_NEXT_PAGE"; break;
    case Type::INTERNAL_INIT_EMPTY: ss << "INTERNAL_INIT_EMPTY"; break;
    case Type::INTERNAL_CREATE_NEW_ROOT: ss << "INTERNAL_CREATE_NEW_ROOT"; break;
    case Type::INTERNAL_UPDATE_KEY: ss << "INTERNAL_UPDATE_KEY"; break;
    case Type::NODE_INSERT: ss << "NODE_INSERT"; break;
    case Type::NODE_REMOVE: ss << "NODE_REMOVE"; break;
    default: ss << "INVALID"; break;
  }
  return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
// LogEntry
/**
 * @brief LogEntryHandler构造函数
 * @details 初始化日志处理器对象，设置操作类型和关联的页面帧
 * @param operation 日志操作类型
 * @param frame 关联的页面帧指针
 */
LogEntryHandler::LogEntryHandler(LogOperation operation, Frame *frame) : operation_type_(operation), frame_(frame)
{
  if (frame_ != nullptr) {
    set_page_num(frame->page_num());
  }
}

/**
 * @brief 获取日志关联的页面编号
 * @details 优先从页面帧获取页面编号，如果页面帧为空，则返回内部存储的页面编号
 * @return 页面编号
 */
PageNum LogEntryHandler::page_num() const
{
  if (frame_ != nullptr) {
    return frame_->page_num();
  }
  return page_num_;
}

/**
 * @brief 序列化整个日志条目
 * @details 先序列化日志头，然后序列化日志内容
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LogEntryHandler::serialize(Serializer &buffer) const
{
  RC rc = serialize_header(buffer);
  if (OB_FAIL(rc)) {
    return rc;
  }
  return serialize_body(buffer);
}

/**
 * @brief 序列化日志头
 * @details 将操作类型和页面编号序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LogEntryHandler::serialize_header(Serializer &buffer) const
{
  int32_t type     = this->operation_type().index();
  PageNum page_num = frame_->page_num();

  int ret = buffer.write_int32(type);
  if (ret < 0) {
    return RC::INTERNAL;
  }
  ret = buffer.write_int32(page_num);
  if (ret < 0) {
    return RC::INTERNAL;
  }
  return RC::SUCCESS;
}

/**
 * @brief 将日志条目转换为字符串表示
 * @details 生成包含操作类型和页面编号的字符串描述
 * @return 日志条目的字符串描述
 */
string LogEntryHandler::to_string() const
{
  stringstream ss;
  ss << "operation=" << operation_type().to_string() << ", page_num=" << page_num();
  return ss.str();
}

/**
 * @brief 从缓冲区反序列化日志条目（不需要Frame）
 * @details 创建一个不关联任何页面帧的日志处理器对象
 * @param deserializer 反序列化器对象
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LogEntryHandler::from_buffer(Deserializer &deserializer, unique_ptr<LogEntryHandler> &handler)
{
  auto fake_frame_getter = [](PageNum, Frame *&frame) -> RC {
    frame = nullptr;
    return RC::SUCCESS;
  };
  return from_buffer(fake_frame_getter, deserializer, handler);
}

/**
 * @brief 从缓冲区反序列化日志条目（使用磁盘缓冲池）
 * @details 使用磁盘缓冲池获取关联的页面帧，并创建日志处理器对象
 * @param buffer_pool 磁盘缓冲池对象
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LogEntryHandler::from_buffer(
    DiskBufferPool &buffer_pool, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  auto frame_getter = [&buffer_pool](PageNum page_num, Frame *&frame) -> RC {
    return buffer_pool.get_this_page(page_num, &frame);
  };
  return from_buffer(frame_getter, buffer, handler);
}

/**
 * @brief 从缓冲区反序列化日志条目（使用自定义的frame获取器）
 * @details 根据操作类型创建对应的日志处理器对象
 * @param frame_getter 获取页面帧的函数对象
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LogEntryHandler::from_buffer(
    function<RC(PageNum, Frame *&)> frame_getter, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int32_t type     = -1;
  PageNum page_num = -1;
  int     ret      = buffer.read_int32(type);
  if (ret != 0) {
    return RC::INVALID_ARGUMENT;
  }

  if (type < 0 || type >= static_cast<int32_t>(LogOperation::Type::MAX_TYPE)) {
    return RC::INVALID_ARGUMENT;
  }

  ret = buffer.read_int32(page_num);
  if (ret != 0) {
    return RC::INVALID_ARGUMENT;
  }

  Frame *frame = nullptr;
  RC     rc    = frame_getter(page_num, frame);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to get frame. page_num=%d, rc=%s", page_num, strrc(rc));
    return rc;
  }

  LogOperation operation(type);

  switch (operation.type()) {
    case LogOperation::Type::INIT_HEADER_PAGE: {
      rc = InitHeaderPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::UPDATE_ROOT_PAGE: {
      rc = UpdateRootPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::SET_PARENT_PAGE: {
      rc = SetParentPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::LEAF_INIT_EMPTY: {
      rc = LeafInitEmptyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::LEAF_SET_NEXT_PAGE: {
      rc = LeafSetNextPageLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_INIT_EMPTY: {
      rc = InternalInitEmptyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_CREATE_NEW_ROOT: {
      rc = InternalCreateNewRootLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::INTERNAL_UPDATE_KEY: {
      rc = InternalUpdateKeyLogEntryHandler::deserialize(frame, buffer, handler);
    } break;

    case LogOperation::Type::NODE_INSERT:
    case LogOperation::Type::NODE_REMOVE: {
      rc = NormalOperationLogEntryHandler::deserialize(frame, operation, buffer, handler);
    } break;

    default: {
      LOG_ERROR("unknown log operation. operation=%d:%s", operation.index(), operation.to_string().c_str());
      return RC::INTERNAL;
    }
  }

  if (OB_SUCC(rc) && handler) {
    handler->set_page_num(page_num);
  }
  return rc;
}

///////////////////////////////////////////////////////////////////////////////
// InitHeaderPageLogEntryHandler
/**
 * @brief InitHeaderPageLogEntryHandler构造函数
 * @details 初始化文件头日志处理器对象，设置操作类型、页面帧和文件头信息
 * @param frame 关联的页面帧指针
 * @param file_header 索引文件头信息
 */
InitHeaderPageLogEntryHandler::InitHeaderPageLogEntryHandler(Frame *frame, const IndexFileHeader &file_header)
    : LogEntryHandler(LogOperation::Type::INIT_HEADER_PAGE, frame), file_header_(file_header)
{}

/**
 * @brief 序列化初始化文件头日志内容
 * @details 将文件头信息序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InitHeaderPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write(span<const char>(reinterpret_cast<const char *>(&file_header_), sizeof(file_header_)));
  return RC::SUCCESS;
}

/**
 * @brief 将初始化文件头日志条目转换为字符串表示
 * @details 生成包含基本日志信息和文件头信息的字符串描述
 * @return 日志条目的字符串描述
 */
string InitHeaderPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", file_header=" << file_header_.to_string();
  return ss.str();
}

/**
 * @brief 反序列化初始化文件头日志
 * @details 从缓冲区读取文件头信息并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InitHeaderPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  IndexFileHeader header;
  int             ret = buffer.read(span<char>(reinterpret_cast<char *>(&header), sizeof(header)));
  if (ret != 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<InitHeaderPageLogEntryHandler>(frame, header);
  return RC::SUCCESS;
}

/**
 * @brief 回滚初始化文件头操作
 * @details 初始化文件头操作通常不需要回滚，因此该方法不执行任何操作
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InitHeaderPageLogEntryHandler::rollback(BplusTreeMiniTransaction &, BplusTreeHandler &)
{
  // do nothing
  return RC::SUCCESS;
}

/**
 * @brief 重做初始化文件头操作
 * @details 在系统重启时，根据日志内容重新初始化文件头
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InitHeaderPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_init_header_page(mtr, frame(), file_header_);
}

///////////////////////////////////////////////////////////////////////////////
// SetParentPageLogEntryHandler
/**
 * @brief SetParentPageLogEntryHandler构造函数
 * @details 初始化设置父节点日志处理器对象，设置操作类型、页面帧、新的父节点页面编号和旧的父节点页面编号
 * @param frame 关联的页面帧指针
 * @param parent_page_num 新的父节点页面编号
 * @param old_parent_page_num 旧的父节点页面编号
 */
SetParentPageLogEntryHandler::SetParentPageLogEntryHandler(
    Frame *frame, PageNum parent_page_num, PageNum old_parent_page_num)
    : NodeLogEntryHandler(LogOperation::Type::SET_PARENT_PAGE, frame),
      parent_page_num_(parent_page_num),
      old_parent_page_num_(old_parent_page_num)
{}

/**
 * @brief 序列化设置父节点日志内容
 * @details 将新的父节点页面编号序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC SetParentPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  int ret = buffer.write_int32(parent_page_num_);
  return ret == 0 ? RC::SUCCESS : RC::INTERNAL;
}

/**
 * @brief 将设置父节点日志条目转换为字符串表示
 * @details 生成包含基本日志信息和父节点页面编号的字符串描述
 * @return 日志条目的字符串描述
 */
string SetParentPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", parent_page_num=" << parent_page_num_;
  return ss.str();
}

/**
 * @brief 反序列化设置父节点日志
 * @details 从缓冲区读取父节点页面编号并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC SetParentPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret             = 0;
  int32_t parent_page_num = -1;
  if ((ret = buffer.read_int32(parent_page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<SetParentPageLogEntryHandler>(frame, parent_page_num, -1 /*old_parent_page_num*/);
  return RC::SUCCESS;
}

/**
 * @brief 回滚设置父节点操作
 * @details 将节点的父节点页面编号恢复为旧值
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC SetParentPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  return node_handler.set_parent_page_num(old_parent_page_num_);
}

/**
 * @brief 重做设置父节点操作
 * @details 在系统重启时，根据日志内容重新设置节点的父节点页面编号
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC SetParentPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  return node_handler.set_parent_page_num(parent_page_num_);
}

///////////////////////////////////////////////////////////////////////////////
// NormalOperationLogEntryHandler
/**
 * @brief NormalOperationLogEntryHandler构造函数
 * @details 初始化普通操作（插入或删除节点元素）日志处理器对象
 * @param frame 关联的页面帧指针
 * @param operation 日志操作类型
 * @param index 操作的索引位置
 * @param items 操作涉及的元素数据
 * @param item_num 元素数量
 */
NormalOperationLogEntryHandler::NormalOperationLogEntryHandler(
    Frame *frame, LogOperation operation, int index, span<const char> items, int item_num)
    : NodeLogEntryHandler(operation, frame), index_(index), item_num_(item_num), items_(items.begin(), items.end())
{}

/**
 * @brief 序列化普通操作日志内容
 * @details 将操作索引位置、元素数量、元素数据大小和元素数据序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC NormalOperationLogEntryHandler::serialize_body(Serializer &buffer) const
{
  int     ret        = 0;
  int32_t item_bytes = static_cast<int32_t>(items_.size());
  if ((ret = buffer.write_int32(index_)) < 0 || (ret = buffer.write_int32(item_num_) < 0) ||
      (ret = buffer.write_int32(item_bytes) < 0) || (ret = buffer.write(items_) < 0)) {
    return RC::INTERNAL;
  }

  return RC::SUCCESS;
}

/**
 * @brief 将普通操作日志条目转换为字符串表示
 * @details 生成包含基本日志信息、操作索引位置和元素数量的字符串描述
 * @return 日志条目的字符串描述
 */
string NormalOperationLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", index=" << index_ << ", item_num=" << item_num_;
  return ss.str();
}

/**
 * @brief 反序列化普通操作日志
 * @details 从缓冲区读取操作索引位置、元素数量、元素数据大小和元素数据，并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param operation 日志操作类型
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC NormalOperationLogEntryHandler::deserialize(
    Frame *frame, LogOperation operation, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t index      = -1;
  int32_t item_num   = -1;
  int32_t item_bytes = -1;
  if ((ret = buffer.read_int32(index)) < 0 || (ret = buffer.read_int32(item_num)) < 0 ||
      (ret = buffer.read_int32(item_bytes)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> items(item_bytes);
  if ((ret = buffer.read(items)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<NormalOperationLogEntryHandler>(frame, operation.type(), index, items, item_num);
  return RC::SUCCESS;
}

/**
 * @brief 回滚普通操作
 * @details 根据操作类型执行相反的操作：如果是插入则删除，如果是删除则插入
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC NormalOperationLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  IndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  if (operation_type().type() == LogOperation::Type::NODE_INSERT) {
    return node_handler.recover_remove_items(index_, item_num_);
  } else {  // should be NODE_REMOVE
    return node_handler.recover_insert_items(index_, items_.data(), item_num_);
  }
}

/**
 * @brief 重做普通操作
 * @details 在系统重启时，根据日志内容重新执行插入或删除操作
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC NormalOperationLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_node(mtr, tree_handler.file_header(), frame());
  LeafIndexNodeHandler     leaf_node(mtr, tree_handler.file_header(), frame());
  IndexNodeHandler         node_handler(mtr, tree_handler.file_header(), frame());
  IndexNodeHandler        *real_handler = nullptr;
  if (node_handler.is_leaf()) {
    real_handler = &leaf_node;
  } else {
    real_handler = &internal_node;
  }
  if (operation_type().type() == LogOperation::Type::NODE_INSERT) {
    return real_handler->recover_insert_items(index_, items_.data(), item_num_);
  } else {  // should be NODE_REMOVE
    return real_handler->recover_remove_items(index_, item_num_);
  }
}

///////////////////////////////////////////////////////////////////////////////
// LeafInitEmptyLogEntryHandler
/**
 * @brief LeafInitEmptyLogEntryHandler构造函数
 * @details 初始化叶子节点初始化日志处理器对象
 * @param frame 关联的页面帧指针
 */
LeafInitEmptyLogEntryHandler::LeafInitEmptyLogEntryHandler(Frame *frame)
    : NodeLogEntryHandler(LogOperation::Type::LEAF_INIT_EMPTY, frame)
{}

/**
 * @brief 重做叶子节点初始化操作
 * @details 在系统重启时，根据日志内容重新初始化空的叶子节点
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafInitEmptyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());
  RC rc = leaf_handler.init_empty();
  return rc;
}

/**
 * @brief 反序列化叶子节点初始化日志
 * @details 从缓冲区读取数据并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafInitEmptyLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  handler = make_unique<LeafInitEmptyLogEntryHandler>(frame);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// LeafSetNextPageLogEntryHandler
/**
 * @brief LeafSetNextPageLogEntryHandler构造函数
 * @details 初始化设置叶子节点兄弟节点日志处理器对象
 * @param frame 关联的页面帧指针
 * @param new_page_num 新的兄弟节点页面编号
 * @param old_page_num 旧的兄弟节点页面编号
 */
LeafSetNextPageLogEntryHandler::LeafSetNextPageLogEntryHandler(Frame *frame, PageNum new_page_num, PageNum old_page_num)
    : NodeLogEntryHandler(LogOperation::Type::LEAF_SET_NEXT_PAGE, frame),
      new_page_num_(new_page_num),
      old_page_num_(old_page_num)
{}

/**
 * @brief 序列化设置叶子节点兄弟节点日志内容
 * @details 将新的兄弟节点页面编号序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafSetNextPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(new_page_num_);
  return RC::SUCCESS;
}

/**
 * @brief 将设置叶子节点兄弟节点日志条目转换为字符串表示
 * @details 生成包含基本日志信息和新的兄弟节点页面编号的字符串描述
 * @return 日志条目的字符串描述
 */
string LeafSetNextPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", new_page_num=" << new_page_num_;
  return ss.str();
}

/**
 * @brief 反序列化设置叶子节点兄弟节点日志
 * @details 从缓冲区读取新的兄弟节点页面编号并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafSetNextPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret      = 0;
  int32_t page_num = -1;
  if ((ret = buffer.read_int32(page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<LeafSetNextPageLogEntryHandler>(frame, page_num, -1 /*old_page_num*/);
  return RC::SUCCESS;
}

/**
 * @brief 回滚设置叶子节点兄弟节点操作
 * @details 将叶子节点的兄弟节点页面编号恢复为旧值
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafSetNextPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());
  leaf_handler.set_next_page(old_page_num_);
  return RC::SUCCESS;
}

/**
 * @brief 重做设置叶子节点兄弟节点操作
 * @details 在系统重启时，根据日志内容重新设置叶子节点的兄弟节点页面编号
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC LeafSetNextPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  LeafIndexNodeHandler leaf_handler(mtr, tree_handler.file_header(), frame());

  leaf_handler.set_next_page(new_page_num_);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// InternalInitEmptyLogEntryHandler
/**
 * @brief InternalInitEmptyLogEntryHandler构造函数
 * @details 初始化内部节点初始化日志处理器对象
 * @param frame 关联的页面帧指针
 */
InternalInitEmptyLogEntryHandler::InternalInitEmptyLogEntryHandler(Frame *frame)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_INIT_EMPTY, frame)
{}

/**
 * @brief 重做内部节点初始化操作
 * @details 在系统重启时，根据日志内容重新初始化空的内部节点
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalInitEmptyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_handler(mtr, tree_handler.file_header(), frame());
  return internal_handler.init_empty();
}

/**
 * @brief 反序列化内部节点初始化日志
 * @details 从缓冲区读取数据并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalInitEmptyLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  handler = make_unique<InternalInitEmptyLogEntryHandler>(frame);
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// InternalCreateNewRootLogEntryHandler
/**
 * @brief InternalCreateNewRootLogEntryHandler构造函数
 * @details 初始化创建新根节点日志处理器对象
 * @param frame 关联的页面帧指针
 * @param first_page_num 第一个子节点页面编号
 * @param key 分界键值
 * @param page_num 第二个子节点页面编号
 */
InternalCreateNewRootLogEntryHandler::InternalCreateNewRootLogEntryHandler(
    Frame *frame, PageNum first_page_num, span<const char> key, PageNum page_num)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_CREATE_NEW_ROOT, frame),
      first_page_num_(first_page_num),
      page_num_(page_num),
      key_(key.begin(), key.end())
{}

/**
 * @brief 序列化创建新根节点日志内容
 * @details 将第一个子节点页面编号、第二个子节点页面编号、键值大小和键值序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalCreateNewRootLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(first_page_num_);
  buffer.write_int32(page_num_);
  buffer.write_int32(static_cast<int32_t>(key_.size()));
  buffer.write(key_);
  return RC::SUCCESS;
}

/**
 * @brief 将创建新根节点日志条目转换为字符串表示
 * @details 生成包含基本日志信息、第一个子节点页面编号和第二个子节点页面编号的字符串描述
 * @return 日志条目的字符串描述
 */
string InternalCreateNewRootLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", first_page_num=" << first_page_num_ << ", page_num=" << page_num_;
  return ss.str();
}

/**
 * @brief 反序列化创建新根节点日志
 * @details 从缓冲区读取第一个子节点页面编号、第二个子节点页面编号、键值大小和键值，并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalCreateNewRootLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t first_page_num = -1;
  int32_t page_num       = -1;
  int32_t key_size       = -1;
  if ((ret = buffer.read_int32(first_page_num)) < 0 || (ret = buffer.read_int32(page_num)) < 0 ||
      (ret = buffer.read_int32(key_size)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> key(key_size);
  if ((ret = buffer.read(key)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<InternalCreateNewRootLogEntryHandler>(frame, first_page_num, key, page_num);
  return RC::SUCCESS;
}

/**
 * @brief 重做创建新根节点操作
 * @details 在系统重启时，根据日志内容重新创建新的根节点
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalCreateNewRootLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler internal_handler(mtr, tree_handler.file_header(), frame());
  RC rc = internal_handler.create_new_root(first_page_num_, key_.data(), page_num_);
  return rc;
}

///////////////////////////////////////////////////////////////////////////////
// InternalUpdateKeyLogEntryHandler
/**
 * @brief InternalUpdateKeyLogEntryHandler构造函数
 * @details 初始化更新内部节点键值日志处理器对象
 * @param frame 关联的页面帧指针
 * @param index 键值的索引位置
 * @param key 新的键值数据
 * @param old_key 旧的键值数据
 */
InternalUpdateKeyLogEntryHandler::InternalUpdateKeyLogEntryHandler(
    Frame *frame, int index, span<const char> key, span<const char> old_key)
    : NodeLogEntryHandler(LogOperation::Type::INTERNAL_UPDATE_KEY, frame),
      index_(index),
      key_(key.begin(), key.end()),
      old_key_(old_key.begin(), old_key.end())
{}

/**
 * @brief 序列化更新内部节点键值日志内容
 * @details 将键值的索引位置、键值大小和新的键值数据序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalUpdateKeyLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(index_);
  buffer.write_int32(static_cast<int32_t>(key_.size()));
  buffer.write(key_);
  return RC::SUCCESS;
}

/**
 * @brief 将更新内部节点键值日志条目转换为字符串表示
 * @details 生成包含基本日志信息和键值索引位置的字符串描述
 * @return 日志条目的字符串描述
 */
string InternalUpdateKeyLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", index=" << index_;
  return ss.str();
}

/**
 * @brief 反序列化更新内部节点键值日志
 * @details 从缓冲区读取键值索引位置、键值大小和新的键值数据，并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalUpdateKeyLogEntryHandler::deserialize(
    Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int ret = 0;

  int32_t index    = -1;
  int32_t key_size = -1;
  if ((ret = buffer.read_int32(index)) < 0 || (ret = buffer.read_int32(key_size)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> key(key_size);
  if ((ret = buffer.read(key)) < 0) {
    return RC::INTERNAL;
  }

  vector<char> old_key(0);
  handler = make_unique<InternalUpdateKeyLogEntryHandler>(frame, index, key, old_key);
  return RC::SUCCESS;
}

/**
 * @brief 回滚更新内部节点键值操作
 * @details 将内部节点的键值恢复为旧值
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalUpdateKeyLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  if (nullptr == frame()) {
    return RC::INTERNAL;
  }
  InternalIndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  node_handler.set_key_at(index_, old_key_.data());
  return RC::SUCCESS;
}

/**
 * @brief 重做更新内部节点键值操作
 * @details 在系统重启时，根据日志内容重新更新内部节点的键值
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC InternalUpdateKeyLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  InternalIndexNodeHandler node_handler(mtr, tree_handler.file_header(), frame());
  node_handler.set_key_at(index_, key_.data());
  return RC::SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
// UpdateRootPageLogEntryHandler

/**
 * @brief UpdateRootPageLogEntryHandler构造函数
 * @details 初始化更新根节点日志处理器对象
 * @param frame 关联的页面帧指针
 * @param root_page_num 新的根节点页面编号
 * @param old_page_num 旧的根节点页面编号
 */
UpdateRootPageLogEntryHandler::UpdateRootPageLogEntryHandler(Frame *frame, PageNum root_page_num, PageNum old_page_num)
    : LogEntryHandler(LogOperation::Type::UPDATE_ROOT_PAGE, frame),
      root_page_num_(root_page_num),
      old_page_num_(old_page_num)
{}

/**
 * @brief 序列化更新根节点日志内容
 * @details 将新的根节点页面编号序列化到缓冲区
 * @param buffer 序列化缓冲区
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC UpdateRootPageLogEntryHandler::serialize_body(Serializer &buffer) const
{
  buffer.write_int32(root_page_num_);
  return RC::SUCCESS;
}

/**
 * @brief 将更新根节点日志条目转换为字符串表示
 * @details 生成包含基本日志信息和新的根节点页面编号的字符串描述
 * @return 日志条目的字符串描述
 */
string UpdateRootPageLogEntryHandler::to_string() const
{
  stringstream ss;
  ss << LogEntryHandler::to_string() << ", root_page_num=" << root_page_num_;
  return ss.str();
}

/**
 * @brief 反序列化更新根节点日志
 * @details 从缓冲区读取新的根节点页面编号并创建日志处理器对象
 * @param frame 关联的页面帧指针
 * @param buffer 二进制Buffer
 * @param[out] handler 返回的日志处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC UpdateRootPageLogEntryHandler::deserialize(Frame *frame, Deserializer &buffer, unique_ptr<LogEntryHandler> &handler)
{
  int     ret           = 0;
  int32_t root_page_num = -1;
  if ((ret = buffer.read_int32(root_page_num)) < 0) {
    return RC::INTERNAL;
  }

  handler = make_unique<UpdateRootPageLogEntryHandler>(frame, root_page_num, -1 /*old_page_num*/);
  return RC::SUCCESS;
}

/**
 * @brief 回滚更新根节点操作
 * @details 将根节点页面编号恢复为旧值
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC UpdateRootPageLogEntryHandler::rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_update_root_page(mtr, old_page_num_);
}

/**
 * @brief 重做更新根节点操作
 * @details 在系统重启时，根据日志内容重新更新根节点页面编号
 * @param mtr B+树迷你事务对象
 * @param tree_handler B+树处理器对象
 * @return 操作结果，成功返回RC::SUCCESS
 */
RC UpdateRootPageLogEntryHandler::redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler)
{
  return tree_handler.recover_update_root_page(mtr, root_page_num_);
}

}  // namespace bplus_tree
