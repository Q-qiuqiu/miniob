/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/common/chunk.h"

/**
 * @brief 添加一列到数据块
 * @param[in] col 列对象的唯一指针
 * @param[in] col_id 列ID
 * @details 将列对象添加到columns_向量中，并将对应的列ID添加到column_ids_向量中
 *          使用std::move转移col的所有权
 */
void Chunk::add_column(unique_ptr<Column> col, int col_id)
{
  columns_.push_back(std::move(col));
  column_ids_.push_back(col_id);
}

/**
 * @brief 引用另一个数据块
 * @param[in] chunk 要引用的数据块
 * @return 操作成功返回RC::SUCCESS，否则返回错误码
 * @details 先调用reset()清空当前数据块，然后调整columns_向量大小为chunk的列数
 *          为每个位置创建Column对象并引用chunk中对应位置的列，同时复制列ID
 */
RC Chunk::reference(Chunk &chunk)
{
  reset();
  this->columns_.resize(chunk.column_num());
  for (size_t i = 0; i < columns_.size(); ++i) {
    if (nullptr == columns_[i]) {
      columns_[i] = make_unique<Column>();
    }
    columns_[i]->reference(chunk.column(i));
    column_ids_.push_back(chunk.column_ids(i));
  }
  return RC::SUCCESS;
}

/**
 * @brief 获取数据块中的行数
 * @return 数据块中的行数
 * @details 如果数据块为空（没有列），则返回0；否则返回第一列的行数
 *          数据块中所有列的行数应该是相同的
 */
int Chunk::rows() const
{
  if (!columns_.empty()) {
    return columns_[0]->count();
  }
  return 0;
}

/**
 * @brief 获取数据块的容量
 * @return 数据块的容量
 * @details 如果数据块为空（没有列），则返回0；否则返回第一列的容量
 *          数据块中所有列的容量应该是相同的
 */
int Chunk::capacity() const
{
  if (!columns_.empty()) {
    return columns_[0]->capacity();
  }
  return 0;
}

/**
 * @brief 重置数据块中的数据，不会修改数据块的列属性
 * @details 调用每个列的reset_data()方法，清空数据内容但保留列的结构和元信息
 */
void Chunk::reset_data()
{
  for (auto &col : columns_) {
    col->reset_data();
  }
}

/**
 * @brief 重置整个数据块，清空所有列和列ID信息
 * @details 清空columns_向量和column_ids_向量，完全重置数据块到初始状态
 */
void Chunk::reset()
{
  columns_.clear();
  column_ids_.clear();
}