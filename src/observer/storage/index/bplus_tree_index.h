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

#pragma once

#include "storage/index/bplus_tree.h"
#include "storage/index/index.h"

/**
 * @brief B+树索引实现类
 * @details 基于B+树数据结构实现的索引，支持高效的插入、删除和范围查询操作
 * @ingroup Index
 */
class BplusTreeIndex : public Index
{
public:
  /**
   * @brief 默认构造函数
   */
  BplusTreeIndex() = default;
  
  /**
   * @brief 析构函数
   * @details 释放索引资源，关闭索引
   */
  virtual ~BplusTreeIndex() noexcept;

  /**
   * @brief 创建一个新的B+树索引
   * @param table 索引所属的表
   * @param file_name 索引文件的名称
   * @param index_meta 索引的元数据
   * @param field_meta 索引字段的元数据
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC create(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta) override;
  
  /**
   * @brief 打开一个已存在的B+树索引
   * @param table 索引所属的表
   * @param file_name 索引文件的名称
   * @param index_meta 索引的元数据
   * @param field_meta 索引字段的元数据
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC open(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta) override;
  
  /**
   * @brief 关闭索引
   * @return 操作结果，成功返回SUCCESS
   */
  RC close();

  /**
   * @brief 向索引中插入一个条目
   * @param record 包含索引键值的记录数据
   * @param rid 记录的唯一标识符
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC insert_entry(const char *record, const RID *rid) override;
  
  /**
   * @brief 从索引中删除一个条目
   * @param record 包含索引键值的记录数据
   * @param rid 记录的唯一标识符
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC delete_entry(const char *record, const RID *rid) override;

  /**
   * @brief 创建一个索引扫描器，用于范围查询
   * @param left_key 左边界键值
   * @param left_len 左边界键值的长度
   * @param left_inclusive 是否包含左边界
   * @param right_key 右边界键值
   * @param right_len 右边界键值的长度
   * @param right_inclusive 是否包含右边界
   * @return 指向索引扫描器的指针，失败返回nullptr
   */
  IndexScanner *create_scanner(const char *left_key, int left_len, bool left_inclusive, const char *right_key,
      int right_len, bool right_inclusive) override;

  /**
   * @brief 将索引数据同步到磁盘
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC sync() override;

private:
  bool             inited_ = false;  ///< 索引是否已初始化
  Table           *table_  = nullptr; ///< 索引所属的表
  BplusTreeHandler index_handler_;   ///< B+树索引处理器，负责实际的索引操作
};

/**
 * @brief B+树索引扫描器实现类
 * @details 用于在B+树索引上进行范围扫描操作
 * @ingroup Index
 */
class BplusTreeIndexScanner : public IndexScanner
{
public:
  /**
   * @brief 构造函数
   * @param tree_handle B+树处理器，用于访问索引数据
   */
  BplusTreeIndexScanner(BplusTreeHandler &tree_handle);
  
  /**
   * @brief 析构函数
   * @details 关闭扫描器，释放资源
   */
  ~BplusTreeIndexScanner() noexcept override;

  /**
   * @brief 获取下一个记录的标识符
   * @param rid 输出参数，用于存储获取到的记录标识符
   * @return 操作结果，成功返回SUCCESS，没有更多记录返回RECORD_EOF
   */
  RC next_entry(RID *rid) override;
  
  /**
   * @brief 销毁扫描器
   * @return 操作结果，成功返回SUCCESS
   */
  RC destroy() override;

  /**
   * @brief 打开扫描器并设置扫描范围
   * @param left_key 左边界键值
   * @param left_len 左边界键值的长度
   * @param left_inclusive 是否包含左边界
   * @param right_key 右边界键值
   * @param right_len 右边界键值的长度
   * @param right_inclusive 是否包含右边界
   * @return 操作结果，成功返回SUCCESS，失败返回错误码
   */
  RC open(const char *left_key, int left_len, bool left_inclusive, const char *right_key, int right_len,
      bool right_inclusive);

private:
  BplusTreeScanner tree_scanner_; ///< B+树扫描器，负责实际的扫描操作
};
