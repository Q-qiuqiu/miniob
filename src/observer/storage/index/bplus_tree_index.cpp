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
// Created by wangyunlai.wyl on 2021/5/19.
//

#include "storage/index/bplus_tree_index.h"
#include "common/log/log.h"
#include "storage/table/table.h"
#include "storage/db/db.h"

/**
 * @brief BplusTreeIndex类的析构函数
 * @details 调用close方法释放索引资源，确保索引被正确关闭
 */
BplusTreeIndex::~BplusTreeIndex() noexcept { close(); }

/**
 * @brief 创建一个新的B+树索引
 * @details 初始化索引元数据，创建索引文件，并初始化B+树索引处理器
 * @param table 索引所属的表
 * @param file_name 索引文件的名称
 * @param index_meta 索引的元数据
 * @param field_meta 索引字段的元数据
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndex::create(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta)
{
  // 检查索引是否已经初始化
  if (inited_) {
    LOG_WARN("Failed to create index due to the index has been created before. file_name:%s, index:%s, field:%s",
        file_name, index_meta.name(), index_meta.field());
    return RC::RECORD_OPENNED;
  }

  // 初始化索引的元数据
  Index::init(index_meta, field_meta);

  // 获取表所在数据库的缓冲池管理器，用于创建索引文件
  BufferPoolManager &bpm = table->db()->buffer_pool_manager();
  // 创建B+树索引处理器
  RC rc = index_handler_.create(table->db()->log_handler(), bpm, file_name, field_meta.type(), field_meta.len());
  if (RC::SUCCESS != rc) {
    LOG_WARN("Failed to create index_handler, file_name:%s, index:%s, field:%s, rc:%s",
        file_name, index_meta.name(), index_meta.field(), strrc(rc));
    return rc;
  }

  // 设置索引初始化状态和所属表
  inited_ = true;
  table_  = table;
  LOG_INFO("Successfully create index, file_name:%s, index:%s, field:%s",
    file_name, index_meta.name(), index_meta.field());
  return RC::SUCCESS;
}

/**
 * @brief 打开一个已存在的B+树索引
 * @details 初始化索引元数据，打开索引文件，并初始化B+树索引处理器
 * @param table 索引所属的表
 * @param file_name 索引文件的名称
 * @param index_meta 索引的元数据
 * @param field_meta 索引字段的元数据
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndex::open(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta)
{
  // 检查索引是否已经初始化
  if (inited_) {
    LOG_WARN("Failed to open index due to the index has been initedd before. file_name:%s, index:%s, field:%s",
        file_name, index_meta.name(), index_meta.field());
    return RC::RECORD_OPENNED;
  }

  // 初始化索引的元数据
  Index::init(index_meta, field_meta);

  // 获取表所在数据库的缓冲池管理器，用于打开索引文件
  BufferPoolManager &bpm = table->db()->buffer_pool_manager();
  // 打开B+树索引处理器
  RC rc = index_handler_.open(table->db()->log_handler(), bpm, file_name);
  if (RC::SUCCESS != rc) {
    LOG_WARN("Failed to open index_handler, file_name:%s, index:%s, field:%s, rc:%s",
        file_name, index_meta.name(), index_meta.field(), strrc(rc));
    return rc;
  }

  // 设置索引初始化状态和所属表
  inited_ = true;
  table_  = table;
  LOG_INFO("Successfully open index, file_name:%s, index:%s, field:%s",
    file_name, index_meta.name(), index_meta.field());
  return RC::SUCCESS;
}

/**
 * @brief 关闭索引
 * @details 关闭B+树索引处理器，重置初始化状态
 * @return 操作结果，成功返回SUCCESS
 */
RC BplusTreeIndex::close()
{
  if (inited_) {
    LOG_INFO("Begin to close index, index:%s, field:%s", index_meta_.name(), index_meta_.field());
    // 关闭索引处理器
    index_handler_.close();
    // 重置初始化状态
    inited_ = false;
  }
  LOG_INFO("Successfully close index.");
  return RC::SUCCESS;
}

/**
 * @brief 向索引中插入一个条目
 * @details 从记录中提取索引键值，并调用索引处理器的插入方法
 * @param record 包含索引键值的记录数据
 * @param rid 记录的唯一标识符
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndex::insert_entry(const char *record, const RID *rid)
{
  // 计算索引字段在记录中的偏移量，提取索引键值，并调用索引处理器的插入方法
  return index_handler_.insert_entry(record + field_meta_.offset(), rid);
}

/**
 * @brief 从索引中删除一个条目
 * @details 从记录中提取索引键值，并调用索引处理器的删除方法
 * @param record 包含索引键值的记录数据
 * @param rid 记录的唯一标识符
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndex::delete_entry(const char *record, const RID *rid)
{
  // 计算索引字段在记录中的偏移量，提取索引键值，并调用索引处理器的删除方法
  return index_handler_.delete_entry(record + field_meta_.offset(), rid);
}

/**
 * @brief 创建一个索引扫描器，用于范围查询
 * @details 创建并初始化一个BplusTreeIndexScanner对象，设置扫描范围
 * @param left_key 左边界键值
 * @param left_len 左边界键值的长度
 * @param left_inclusive 是否包含左边界
 * @param right_key 右边界键值
 * @param right_len 右边界键值的长度
 * @param right_inclusive 是否包含右边界
 * @return 指向索引扫描器的指针，失败返回nullptr
 */
IndexScanner *BplusTreeIndex::create_scanner(
    const char *left_key, int left_len, bool left_inclusive, const char *right_key, int right_len, bool right_inclusive)
{
  // 创建索引扫描器
  BplusTreeIndexScanner *index_scanner = new BplusTreeIndexScanner(index_handler_);
  // 打开扫描器并设置扫描范围
  RC rc = index_scanner->open(left_key, left_len, left_inclusive, right_key, right_len, right_inclusive);
  if (rc != RC::SUCCESS) {
    LOG_WARN("failed to open index scanner. rc=%d:%s", rc, strrc(rc));
    // 如果打开失败，释放扫描器资源
    delete index_scanner;
    return nullptr;
  }
  return index_scanner;
}

/**
 * @brief 将索引数据同步到磁盘
 * @details 调用索引处理器的同步方法，确保索引数据持久化到磁盘
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndex::sync() { 
  // 调用索引处理器的同步方法
  return index_handler_.sync(); 
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief BplusTreeIndexScanner类的构造函数
 * @details 初始化B+树索引扫描器，设置内部的树扫描器
 * @param tree_handler B+树处理器，用于访问索引数据
 */
BplusTreeIndexScanner::BplusTreeIndexScanner(BplusTreeHandler &tree_handler) : tree_scanner_(tree_handler) {}

/**
 * @brief BplusTreeIndexScanner类的析构函数
 * @details 关闭树扫描器，释放资源
 */
BplusTreeIndexScanner::~BplusTreeIndexScanner() noexcept { 
  // 关闭内部的树扫描器
  tree_scanner_.close(); 
}

/**
 * @brief 打开扫描器并设置扫描范围
 * @details 设置索引扫描的左边界和右边界条件
 * @param left_key 左边界键值
 * @param left_len 左边界键值的长度
 * @param left_inclusive 是否包含左边界
 * @param right_key 右边界键值
 * @param right_len 右边界键值的长度
 * @param right_inclusive 是否包含右边界
 * @return 操作结果，成功返回SUCCESS，失败返回错误码
 */
RC BplusTreeIndexScanner::open(
    const char *left_key, int left_len, bool left_inclusive, const char *right_key, int right_len, bool right_inclusive)
{
  // 调用内部树扫描器的open方法设置扫描范围
  return tree_scanner_.open(left_key, left_len, left_inclusive, right_key, right_len, right_inclusive);
}

/**
 * @brief 获取下一个记录的标识符
 * @details 从索引扫描结果中获取下一个记录的RID
 * @param rid 输出参数，用于存储获取到的记录标识符
 * @return 操作结果，成功返回SUCCESS，没有更多记录返回RECORD_EOF
 */
RC BplusTreeIndexScanner::next_entry(RID *rid) { 
  // 调用内部树扫描器的next_entry方法获取下一个记录的RID
  return tree_scanner_.next_entry(*rid); 
}

/**
 * @brief 销毁扫描器
 * @details 释放扫描器占用的内存资源
 * @return 操作结果，成功返回SUCCESS
 */
RC BplusTreeIndexScanner::destroy()
{
  // 删除自身，释放内存
  delete this;
  return RC::SUCCESS;
}
