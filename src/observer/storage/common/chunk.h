/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "common/sys/rc.h"
#include "common/log/log.h"
#include "common/lang/memory.h"
#include "common/lang/vector.h"
#include "storage/common/column.h"

/**
 * @brief 数据块类，表示一组列的集合
 * @details Chunk 是由多个 Column 对象组成的集合，用于表示表中的一组行数据
 *          每个 Chunk 可以包含多个列，每列对应表中的一个字段
 */
class Chunk
{
public:
  /**
   * @brief 默认构造函数
   */
  Chunk()              = default;
  
  /**
   * @brief 拷贝构造函数（禁用）
   */
  Chunk(const Chunk &) = delete;
  
  /**
   * @brief 移动构造函数（禁用）
   */
  Chunk(Chunk &&)      = delete;

  /**
   * @brief 获取列的数量
   * @return 列的数量
   */
  int column_num() const { return columns_.size(); }

  /**
   * @brief 获取指定索引的列
   * @param[in] idx 列索引
   * @return 指定索引的列引用
   * @note 会断言索引是否有效
   */
  Column &column(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return *columns_[idx];
  }

  /**
   * @brief 获取指定索引的列指针
   * @param[in] idx 列索引
   * @return 指定索引的列指针
   * @note 会断言索引是否有效
   */
  Column *column_ptr(size_t idx)
  {
    ASSERT(idx < columns_.size(), "invalid column index");
    return &column(idx);
  }

  /**
   * @brief 获取指定索引的列ID
   * @param[in] i 列索引
   * @return 指定索引的列ID
   * @note 会断言索引是否有效
   */
  int column_ids(size_t i)
  {
    ASSERT(i < column_ids_.size(), "invalid column index");
    return column_ids_[i];
  }

  /**
   * @brief 添加一列到数据块
   * @param[in] col 列对象的唯一指针
   * @param[in] col_id 列ID
   */
  void add_column(unique_ptr<Column> col, int col_id);

  /**
   * @brief 引用另一个数据块
   * @param[in] chunk 要引用的数据块
   * @return 操作成功返回RC::SUCCESS，否则返回错误码
   * @details 重置当前数据块，并共享另一个数据块的列和列ID信息
   */
  RC reference(Chunk &chunk);

  /**
   * @brief 获取 Chunk 中的行数
   * @return 数据块中的行数
   * @details 如果数据块为空，则返回0；否则返回第一列的行数
   */
  int rows() const;

  /**
   * @brief 获取 Chunk 的容量
   * @return 数据块的容量
   * @details 如果数据块为空，则返回0；否则返回第一列的容量
   */
  int capacity() const;

  /**
   * @brief 从 Chunk 中获得指定行指定列的 Value
   * @param[in] col_idx 列索引
   * @param[in] row_idx 行索引
   * @return 指定位置的值对象
   * @note 没有检查 col_idx 和 row_idx 是否越界
   */
  Value get_value(int col_idx, int row_idx) const { return columns_[col_idx]->get_value(row_idx); }

  /**
   * @brief 重置 Chunk 中的数据，不会修改 Chunk 的列属性
   * @details 清空所有列的数据内容，但保留列的结构和元信息
   */
  void reset_data();

  /**
   * @brief 重置整个 Chunk，清空所有列和列ID信息
   * @details 清空所有列和列ID信息，完全重置数据块到初始状态
   */
  void reset();

private:
  /**
   * @brief 存储列对象的向量
   */
  vector<unique_ptr<Column>> columns_;
  
  /**
   * @brief 存储列ID的向量
   * @note TODO: 移除它并支持多表，`columnd_ids`存储子操作符需要输出的ID
   */
  vector<int> column_ids_;
};
