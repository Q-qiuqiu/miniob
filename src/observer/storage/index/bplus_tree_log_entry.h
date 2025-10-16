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
// Created by wangyunlai.wyl on 2024/02/20.
//

#pragma once

#include "common/types.h"
#include "common/sys/rc.h"
#include "common/lang/span.h"
#include "common/lang/string.h"
#include "storage/index/bplus_tree.h"

/**
 * @brief 前向声明：索引节点处理器类
 */
class IndexNodeHandler;
/**
 * @brief 前向声明：B+树处理器类
 */
class BplusTreeHandler;
/**
 * @brief 前向声明：B+树迷你事务类
 */
class BplusTreeMiniTransaction;

/**
 * @brief 前向声明：common命名空间中的序列化和反序列化相关类
 */
namespace common {
class Serializer;
class Deserializer;
}  // namespace common

/**
 * @brief B+树日志相关类的命名空间
 * @details 该命名空间包含了B+树操作日志的所有相关类，用于支持B+树的事务恢复和一致性保证
 */
namespace bplus_tree {

/**
 * @brief B+树日志操作类型
 * @ingroup CLog
 * @details 定义了B+树支持的所有日志操作类型，每种类型对应B+树的一种修改操作
 */
class LogOperation
{
public:
  /**
   * @brief 日志操作类型枚举
   */
  enum class Type
  {
    INIT_HEADER_PAGE,          ///< 初始化B+树文件头
    UPDATE_ROOT_PAGE,          ///< 更新根节点
    SET_PARENT_PAGE,           ///< 设置父节点
    LEAF_INIT_EMPTY,           ///< 初始化叶子节点
    LEAF_SET_NEXT_PAGE,        ///< 设置叶子节点的兄弟节点
    INTERNAL_INIT_EMPTY,       ///< 初始化内部节点
    INTERNAL_CREATE_NEW_ROOT,  ///< 创建新的根节点
    INTERNAL_UPDATE_KEY,       ///< 更新内部节点的key
    NODE_INSERT,               ///< 在节点中间(也可能是末尾)插入一些元素
    NODE_REMOVE,               ///< 在节点中间(也可能是末尾)删除一些元素

    MAX_TYPE,                  ///< 操作类型数量上限，用于边界检查
  };

public:
  /**
   * @brief 构造函数
   * @param type 日志操作类型
   */
  LogOperation(Type type) : type_(type) {}
  
  /**
   * @brief 构造函数，从整数类型转换
   * @param type 日志操作类型的整数表示
   */
  explicit LogOperation(int type) : type_(static_cast<Type>(type)) {}

  /**
   * @brief 获取操作类型
   * @return 操作类型枚举值
   */
  Type type() const { return type_; }
  
  /**
   * @brief 获取操作类型的整数索引
   * @return 操作类型的整数表示
   */
  int  index() const { return static_cast<int>(type_); }

  /**
   * @brief 将操作类型转换为字符串表示
   * @return 操作类型的字符串描述
   */
  string to_string() const;

private:
  Type type_;  ///< 日志操作类型
};

/**
 * @brief B+树日志处理辅助基类
 * @ingroup CLog
 * @details 每种操作类型的日志都有一个具体的实现类，该类为所有日志处理类提供通用接口和功能
 */
class LogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param operation 日志操作类型
   * @param frame 关联的页面帧指针，默认为nullptr
   */
  LogEntryHandler(LogOperation operation, Frame *frame = nullptr);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~LogEntryHandler() = default;

  /**
   * @brief 返回日志对应的frame
   * @details 每条日志都对应着操作关联的页面。但是在日志重放时，或者只是想将日志内容格式化时，是没有对应的页面的。
   * @return 页面帧指针
   */
  Frame       *frame() { return frame_; }
  
  /**
   * @brief 返回日志对应的frame（常量版本）
   * @return 常量页面帧指针
   */
  const Frame *frame() const { return frame_; }

  /**
   * @brief 获取页面编号
   * @return 页面编号
   */
  PageNum page_num() const;
  
  /**
   * @brief 设置页面编号
   * @param page_num 页面编号
   */
  void    set_page_num(PageNum page_num) { page_num_ = page_num; }

  /**
   * @brief 获取日志操作类型
   * @return 日志操作类型
   */
  LogOperation operation_type() const { return operation_type_; }

  /**
   * @brief 序列化日志
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize(common::Serializer &buffer) const;
  
  /**
   * @brief 序列化日志头
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_header(common::Serializer &buffer) const;

  /**
   * @brief 序列化日志内容
   * @details 所有子类应该实现这个函数
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC serialize_body(common::Serializer &buffer) const = 0;

  /**
   * @brief 回滚操作
   * @details 在事务回滚时，需要根据日志内容进行回滚
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;

  /**
   * @brief 重做操作
   * @details 在系统重启时，需要根据日志内容进行重做
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) = 0;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  virtual string to_string() const;

  /**
   * @brief 从buffer中反序列化出一个LogEntryHandler
   *
   * @param frame_getter 获取页面帧的函数对象
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @details
   * 这里有个比较别扭的地方。一个日志应该有两种表现形式，一个是内存或磁盘中的二进制数据，另一个是可以进行操作的Handler。
   * 但是这里实现的时候，只使用了handler，所以在反序列化时，有些场景下根本不需要Frame，但是为了适配，允许传入一个null
   * frame。 在LogEntryHandler类中也做了特殊处理。就是虽然有frame指针对象，但是也另外单独记录了page_num。
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC from_buffer(
      function<RC(PageNum, Frame *&)> frame_getter, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
  
  /**
   * @brief 从buffer中反序列化出一个LogEntryHandler（使用磁盘缓冲池）
   * @param buffer_pool 磁盘缓冲池对象
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC from_buffer(
      DiskBufferPool &buffer_pool, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
  
  /**
   * @brief 从buffer中反序列化出一个LogEntryHandler（不需要Frame）
   * @param deserializer 反序列化器对象
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC from_buffer(common::Deserializer &deserializer, unique_ptr<LogEntryHandler> &handler);

protected:
  LogOperation operation_type_;  ///< 日志操作类型
  Frame       *frame_ = nullptr; ///< 关联的页面帧指针

  /// page num本来存放在frame中。但是只有在运行时才能拿到frame，为了强制适配
  /// 解析文件buffer时不存在运行时的情况，直接记录page num
  PageNum page_num_ = BP_INVALID_PAGE_NUM; ///< 页面编号，单独存储用于没有frame的情况
};

/**
 * @brief 节点相关的日志操作基类
 * @ingroup CLog
 * @details 所有与节点操作相关的日志处理器的基类
 */
class NodeLogEntryHandler : public LogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param operation 日志操作类型
   * @param frame 关联的页面帧指针
   */
  NodeLogEntryHandler(LogOperation operation, Frame *frame) : LogEntryHandler(operation, frame) {}

  /**
   * @brief 虚析构函数
   */
  virtual ~NodeLogEntryHandler() = default;
};

/**
 * @brief 初始化B+树文件头日志处理类
 * @ingroup CLog
 * @details 处理B+树文件头初始化操作的日志
 */
class InitHeaderPageLogEntryHandler : public LogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param file_header 索引文件头信息
   */
  InitHeaderPageLogEntryHandler(Frame *frame, const IndexFileHeader &file_header);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~InitHeaderPageLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化初始化文件头日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取文件头信息
   * @return 文件头信息的常量引用
   */
  const IndexFileHeader &file_header() const { return file_header_; }

private:
  IndexFileHeader file_header_;  ///< 索引文件头信息
};

/**
 * @brief 更新根节点日志处理类
 * @ingroup CLog
 * @details 处理B+树根节点更新操作的日志
 */
class UpdateRootPageLogEntryHandler : public LogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param root_page_num 新的根节点页面编号
   * @param old_page_num 旧的根节点页面编号
   */
  UpdateRootPageLogEntryHandler(Frame *frame, PageNum root_page_num, PageNum old_page_num);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~UpdateRootPageLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化更新根节点日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取根节点页面编号
   * @return 根节点页面编号
   */
  PageNum root_page_num() const { return root_page_num_; }

private:
  PageNum root_page_num_ = -1;  ///< 新的根节点页面编号
  PageNum old_page_num_  = -1;  ///< 旧的根节点页面编号
};

/**
 * @brief 设置父节点日志处理类
 * @ingroup CLog
 * @details 处理设置节点父节点操作的日志
 */
class SetParentPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param parent_page_num 新的父节点页面编号
   * @param old_parent_page_num 旧的父节点页面编号
   */
  SetParentPageLogEntryHandler(Frame *frame, PageNum parent_page_num, PageNum old_parent_page_num);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~SetParentPageLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化设置父节点日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取父节点页面编号
   * @return 父节点页面编号
   */
  PageNum parent_page_num() const { return parent_page_num_; }

private:
  PageNum parent_page_num_     = -1;  ///< 新的父节点页面编号
  PageNum old_parent_page_num_ = -1;  ///< 旧的父节点页面编号
};

/**
 * @brief 插入或者删除节点元素日志处理类
 * @ingroup CLog
 * @details 处理在节点中插入或删除元素操作的日志
 */
class NormalOperationLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param operation 日志操作类型
   * @param index 操作的索引位置
   * @param items 操作涉及的元素数据
   * @param item_num 元素数量
   */
  NormalOperationLogEntryHandler(Frame *frame, LogOperation operation, int index, span<const char> items, int item_num);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~NormalOperationLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化普通操作日志
   * @param frame 关联的页面帧指针
   * @param operation 日志操作类型
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(
      Frame *frame, LogOperation operation, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取操作的索引位置
   * @return 索引位置
   */
  int         index() const { return index_; }
  
  /**
   * @brief 获取元素数量
   * @return 元素数量
   */
  int         item_num() const { return item_num_; }
  
  /**
   * @brief 获取元素数据
   * @return 元素数据的指针
   */
  const char *items() const { return items_.data(); }
  
  /**
   * @brief 获取元素数据的字节数
   * @return 元素数据的字节数
   */
  int32_t     item_bytes() const { return static_cast<int32_t>(items_.size()); }

private:
  int          index_    = -1;  ///< 操作的索引位置
  int          item_num_ = -1;  ///< 元素数量
  vector<char> items_;          ///< 元素数据
};

/**
 * @brief 叶子节点初始化日志处理类
 * @ingroup CLog
 * @details 处理叶子节点初始化操作的日志
 */
class LeafInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   */
  LeafInitEmptyLogEntryHandler(Frame *frame);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~LeafInitEmptyLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 反序列化叶子节点初始化日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
};

/**
 * @brief 设置叶子节点兄弟节点日志处理类
 * @ingroup CLog
 * @details 处理设置叶子节点兄弟节点操作的日志
 */
class LeafSetNextPageLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param new_page_num 新的兄弟节点页面编号
   * @param old_page_num 旧的兄弟节点页面编号
   */
  LeafSetNextPageLogEntryHandler(Frame *frame, PageNum new_page_num, PageNum old_page_num);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~LeafSetNextPageLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化设置叶子节点兄弟节点日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取新的兄弟节点页面编号
   * @return 兄弟节点页面编号
   */
  PageNum new_page_num() const { return new_page_num_; }

private:
  PageNum new_page_num_ = -1;  ///< 新的兄弟节点页面编号
  PageNum old_page_num_ = -1;  ///< 旧的兄弟节点页面编号
};

/**
 * @brief 初始化内部节点日志处理类
 * @ingroup CLog
 * @details 处理内部节点初始化操作的日志
 */
class InternalInitEmptyLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   */
  InternalInitEmptyLogEntryHandler(Frame *frame);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~InternalInitEmptyLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override { return RC::SUCCESS; }
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 反序列化内部节点初始化日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);
};

/**
 * @brief 创建新的根节点日志处理类
 * @ingroup CLog
 * @details 处理创建新的根节点操作的日志
 */
class InternalCreateNewRootLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param first_page_num 第一个子节点页面编号
   * @param key 分界键值
   * @param page_num 第二个子节点页面编号
   */
  InternalCreateNewRootLogEntryHandler(Frame *frame, PageNum first_page_num, span<const char> key, PageNum page_num);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~InternalCreateNewRootLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override { return RC::SUCCESS; }
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化创建新根节点日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取第一个子节点页面编号
   * @return 子节点页面编号
   */
  PageNum     first_page_num() const { return first_page_num_; }
  
  /**
   * @brief 获取第二个子节点页面编号
   * @return 子节点页面编号
   */
  PageNum     page_num() const { return page_num_; }
  
  /**
   * @brief 获取分界键值
   * @return 键值数据的指针
   */
  const char *key() const { return key_.data(); }
  
  /**
   * @brief 获取分界键值的字节数
   * @return 键值数据的字节数
   */
  int32_t     key_bytes() const { return static_cast<int32_t>(key_.size()); }

private:
  PageNum      first_page_num_ = -1;  ///< 第一个子节点页面编号
  PageNum      page_num_       = -1;  ///< 第二个子节点页面编号
  vector<char> key_;                  ///< 分界键值
};

/**
 * @brief 更新内部节点键值日志处理类
 * @ingroup CLog
 * @details 处理更新内部节点键值操作的日志
 */
class InternalUpdateKeyLogEntryHandler : public NodeLogEntryHandler
{
public:
  /**
   * @brief 构造函数
   * @param frame 关联的页面帧指针
   * @param index 键值的索引位置
   * @param key 新的键值数据
   * @param old_key 旧的键值数据
   */
  InternalUpdateKeyLogEntryHandler(Frame *frame, int index, span<const char> key, span<const char> old_key);
  
  /**
   * @brief 虚析构函数
   */
  virtual ~InternalUpdateKeyLogEntryHandler() = default;

  /**
   * @brief 序列化日志内容
   * @param buffer 序列化缓冲区
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC serialize_body(common::Serializer &buffer) const override;
  
  /**
   * @brief 回滚操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC rollback(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;
  
  /**
   * @brief 重做操作
   * @param mtr B+树迷你事务对象
   * @param tree_handler B+树处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC redo(BplusTreeMiniTransaction &mtr, BplusTreeHandler &tree_handler) override;

  /**
   * @brief 将日志条目转换为字符串表示
   * @return 日志条目的字符串描述
   */
  string to_string() const override;

  /**
   * @brief 反序列化更新内部节点键值日志
   * @param frame 关联的页面帧指针
   * @param buffer 二进制Buffer
   * @param[out] handler 返回的日志处理器对象
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC deserialize(Frame *frame, common::Deserializer &buffer, unique_ptr<LogEntryHandler> &handler);

  /**
   * @brief 获取键值的索引位置
   * @return 索引位置
   */
  int         index() const { return index_; }
  
  /**
   * @brief 获取新的键值数据
   * @return 键值数据的指针
   */
  const char *key() const { return key_.data(); }
  
  /**
   * @brief 获取键值数据的字节数
   * @return 键值数据的字节数
   */
  int32_t     key_bytes() const { return static_cast<int32_t>(key_.size()); }

private:
  int          index_ = -1;  ///< 键值的索引位置
  vector<char> key_;         ///< 新的键值数据
  vector<char> old_key_;     ///< 旧的键值数据
};

}  // namespace bplus_tree
