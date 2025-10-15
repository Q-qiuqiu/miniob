/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/log/log.h"
#include "storage/common/column.h"

/**
 * @brief 使用字段元数据构造列
 * @param[in] meta 字段元数据，包含类型和长度信息
 * @param[in] size 初始容量大小，默认值从DEFAULT_CAPACITY获取
 */
Column::Column(const FieldMeta &meta, size_t size)
    : data_(nullptr),
      count_(0),
      capacity_(0),
      own_(true),
      attr_type_(meta.type()),
      attr_len_(meta.len()),
      column_type_(Type::NORMAL_COLUMN)
{
  // TODO: optimized the memory usage if it doesn't need to allocate memory
  data_     = new char[size * attr_len_];
  capacity_ = size;
}

/**
 * @brief 使用属性类型和长度构造列
 * @param[in] attr_type 属性类型
 * @param[in] attr_len 属性长度
 * @param[in] capacity 初始容量大小
 */
Column::Column(AttrType attr_type, int attr_len, size_t capacity)
{
  attr_type_   = attr_type;
  attr_len_    = attr_len;
  data_        = new char[capacity * attr_len_];
  count_       = 0;
  capacity_    = capacity;
  own_         = true;
  column_type_ = Type::NORMAL_COLUMN;
}

/**
 * @brief 使用字段元数据初始化列
 * @param[in] meta 字段元数据
 * @param[in] size 初始容量大小，默认值从DEFAULT_CAPACITY获取
 * @details 先调用reset()清空当前资源，然后根据字段元数据重新分配内存并初始化
 */
void Column::init(const FieldMeta &meta, size_t size)
{
  reset();
  data_        = new char[size * meta.len()];
  count_       = 0;
  capacity_    = size;
  attr_type_   = meta.type();
  attr_len_    = meta.len();
  own_         = true;
  column_type_ = Type::NORMAL_COLUMN;
}

/**
 * @brief 使用属性类型和长度初始化列
 * @param[in] attr_type 属性类型
 * @param[in] attr_len 属性长度
 * @param[in] capacity 初始容量大小，默认值从DEFAULT_CAPACITY获取
 * @details 先调用reset()清空当前资源，然后根据属性类型和长度重新分配内存并初始化
 */
void Column::init(AttrType attr_type, int attr_len, size_t capacity)
{
  reset();
  data_        = new char[capacity * attr_len];
  count_       = 0;
  capacity_    = capacity;
  own_         = true;
  attr_type_   = attr_type;
  attr_len_    = attr_len;
  column_type_ = Type::NORMAL_COLUMN;
}

/**
 * @brief 使用值初始化列（创建常量列）
 * @param[in] value 用于初始化的值
 * @details 先调用reset()清空当前资源，然后创建一个只包含一个值的常量列
 */
void Column::init(const Value &value)
{
  reset();
  attr_type_ = value.attr_type();
  attr_len_  = value.length();
  data_      = new char[attr_len_];
  count_     = 1;
  capacity_  = 1;
  own_       = true;
  memcpy(data_, value.data(), attr_len_);
  column_type_ = Type::CONSTANT_COLUMN;
}

/**
 * @brief 重置列，释放资源并重置所有成员变量
 * @details 如果拥有数据内存（own_为true），则释放data_指向的内存
 *          重置所有成员变量为默认初始状态
 */
void Column::reset()
{
  if (data_ != nullptr && own_) {
    delete[] data_;
  }
  data_      = nullptr;
  count_     = 0;
  capacity_  = 0;
  own_       = false;
  attr_type_ = AttrType::UNDEFINED;
  attr_len_  = -1;
}

/**
 * @brief 向列中追加一个值
 * @param[in] data 要追加的数据指针
 * @return 操作成功返回RC::SUCCESS，否则返回错误码
 * @details 内部调用append方法，传入count=1
 */
RC Column::append_one(char *data) { return append(data, 1); }

/**
 * @brief 向列中追加多个值
 * @param[in] data 要被写入数据的起始地址
 * @param[in] count 要写入数据的长度（这里指列值的个数，而不是字节）
 * @return 操作成功返回RC::SUCCESS，否则返回错误码
 * @details 检查列是否拥有内存以及是否有足够空间，然后通过memcpy批量复制数据
 */
RC Column::append(char *data, int count)
{
  if (!own_) {
    LOG_WARN("append data to non-owned column");
    return RC::INTERNAL;
  }
  if (count_ + count > capacity_) {
    LOG_WARN("append data to full column");
    return RC::INTERNAL;
  }
  // Using a larger integer type to avoid overflow
  size_t total_bytes = static_cast<size_t>(count) * static_cast<size_t>(attr_len_);

  memcpy(data_ + count_ * attr_len_, data, total_bytes);
  count_ += count;
  return RC::SUCCESS;
}

/**
 * @brief 获取指定索引位置的列值
 * @param[in] index 要获取的值的索引位置
 * @return 索引位置的值对象，如果索引无效则返回空值
 */
Value Column::get_value(int index) const
{
  if (index >= count_ || index < 0) {
    return Value();
  }
  return Value(attr_type_, &data_[index * attr_len_], attr_len_);
}

/**
 * @brief 引用另一个列对象
 * @param[in] column 要引用的列对象
 * @details 先调用reset()清空当前资源，然后共享另一个列的内存和属性信息，不拥有内存所有权
 */
void Column::reference(const Column &column)
{
  if (this == &column) {
    return;
  }
  reset();

  this->data_     = column.data();
  this->capacity_ = column.capacity();
  this->count_    = column.count();
  this->own_      = false;

  this->column_type_ = column.column_type();
  this->attr_type_   = column.attr_type();
  this->attr_len_    = column.attr_len();
}