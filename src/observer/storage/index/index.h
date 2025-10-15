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
// Created by Meiyi & Wangyunlai on 2021/5/11.
//

#pragma once

#include <stddef.h>
#include <vector>

#include "common/sys/rc.h"
#include "storage/field/field_meta.h"
#include "storage/index/index_meta.h"
#include "storage/record/record_manager.h"

/**
 * @brief 前向声明：索引扫描器类
 */
class IndexScanner;

/**
 * @brief 索引
 * @defgroup Index
 * @details 索引可能会有很多种实现，比如B+树、哈希表等，这里定义了一个基类，用于描述索引的基本操作。
 */

/**
 * @brief 索引基类
 * @ingroup Index
 * @details 定义了所有索引实现必须遵循的接口规范，提供了索引的基本操作方法
 */
class Index
{
public:
  /**
   * @brief 默认构造函数
   */
  Index()          = default;
  
  /**
   * @brief 虚析构函数
   */
  virtual ~Index() = default;

  /**
   * @brief 创建索引
   * @param table 表对象指针
   * @param file_name 索引文件名
   * @param index_meta 索引元数据
   * @param field_meta 字段元数据
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   * @note 基类默认实现返回RC::UNSUPPORTED，子类需要重写此方法
   */
  virtual RC create(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta)
  {
    return RC::UNSUPPORTED;
  }
  
  /**
   * @brief 打开索引
   * @param table 表对象指针
   * @param file_name 索引文件名
   * @param index_meta 索引元数据
   * @param field_meta 字段元数据
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   * @note 基类默认实现返回RC::UNSUPPORTED，子类需要重写此方法
   */
  virtual RC open(Table *table, const char *file_name, const IndexMeta &index_meta, const FieldMeta &field_meta)
  {
    return RC::UNSUPPORTED;
  }

  /**
   * @brief 检查当前索引是否为向量索引
   * @return 是否为向量索引
   * @note 基类默认实现返回false，向量索引子类需要重写此方法
   */
  virtual bool is_vector_index() { return false; }

  /**
   * @brief 获取索引元数据
   * @return 索引元数据的常量引用
   */
  const IndexMeta &index_meta() const { return index_meta_; }

  /**
   * @brief 插入一条数据
   * @param record 插入的记录，当前假设记录是定长的
   * @param rid 插入的记录的位置
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   */
  virtual RC insert_entry(const char *record, const RID *rid) = 0;

  /**
   * @brief 删除一条数据
   * @param record 删除的记录，当前假设记录是定长的
   * @param rid 删除的记录的位置
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   */
  virtual RC delete_entry(const char *record, const RID *rid) = 0;

  /**
   * @brief 创建一个索引数据的扫描器
   * @param left_key 要扫描的左边界
   * @param left_len 左边界的长度
   * @param left_inclusive 是否包含左边界
   * @param right_key 要扫描的右边界
   * @param right_len 右边界的长度
   * @param right_inclusive 是否包含右边界
   * @return 索引扫描器指针，如果创建失败返回nullptr
   */
  virtual IndexScanner *create_scanner(const char *left_key, int left_len, bool left_inclusive, const char *right_key,
      int right_len, bool right_inclusive) = 0;

  /**
   * @brief 同步索引数据到磁盘
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   */
  virtual RC sync() = 0;

protected:
  /**
   * @brief 初始化索引基类
   * @param index_meta 索引元数据
   * @param field_meta 字段元数据
   * @return 操作结果，成功返回RC::SUCCESS
   */
  RC init(const IndexMeta &index_meta, const FieldMeta &field_meta);

protected:
  IndexMeta index_meta_;  ///< 索引的元数据
  FieldMeta field_meta_;  ///< 当前实现仅考虑一个字段的索引
};

/**
 * @brief 索引扫描器
 * @ingroup Index
 * @details 用于遍历索引中的数据项，提供了顺序访问索引条目的接口
 */
class IndexScanner
{
public:
  /**
   * @brief 默认构造函数
   */
  IndexScanner()          = default;
  
  /**
   * @brief 虚析构函数
   */
  virtual ~IndexScanner() = default;

  /**
   * @brief 遍历索引中的下一个元素
   * @param rid 输出参数，存储获取到的记录ID
   * @return 操作结果，成功返回RC::SUCCESS，如果没有更多元素返回RC::RECORD_EOF
   */
  virtual RC next_entry(RID *rid) = 0;
  
  /**
   * @brief 销毁索引扫描器
   * @return 操作结果，成功返回RC::SUCCESS
   */
  virtual RC destroy()            = 0;
};
