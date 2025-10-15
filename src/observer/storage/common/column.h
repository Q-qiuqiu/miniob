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

#include <string.h>

#include "storage/field/field_meta.h"

/**
 * @brief Column 类更像是一个通用的列数据容器，它在数据库的查询执行过程中非常有用，
 * 尤其是在需要批量处理列数据的场景下。
 * 例如，在查询执行计划中，可能需要对某一列的数据进行过滤、排序、聚合等操作，
 * 使用 Column 类可以方便地管理这些中间数据。

 * 虽然列式存储的核心思想也是按列组织和存储数据，
 * 但完整的列式存储引擎通常还需要考虑更多因素，
 * 如压缩、编码、向量化执行等。
 * 而当前的 Column 类实现相对简单，主要关注于内存中列数据的基本管理。
 */

/**
 * @brief 列类，包含连续内存中的多个相同类型的值
 * @details 用于存储表中某一列的所有值，支持普通列和常量列两种类型
 * @note 当前仅支持定长类型
 */
// TODO: `Column` currently only support fixed-length type.
class Column
{
public:
  /**
   * @brief 列类型枚举
   */
  enum class Type
  {
    NORMAL_COLUMN,   ///< 普通列，表示一系列定长值的列表
    CONSTANT_COLUMN  ///< 常量列，表示单个值
  };

  /**
   * @brief 默认构造函数
   */
  Column()               = default;
  
  /**
   * @brief 拷贝构造函数（禁用）
   */
  Column(const Column &) = delete;
  
  /**
   * @brief 移动构造函数（禁用）
   */
  Column(Column &&)      = delete;

  /**
   * @brief 带字段元数据的构造函数
   * @param[in] meta 字段元数据
   * @param[in] size 初始容量大小
   */
  Column(const FieldMeta &meta, size_t size = DEFAULT_CAPACITY);
  
  /**
   * @brief 带属性类型和长度的构造函数
   * @param[in] attr_type 属性类型
   * @param[in] attr_len 属性长度
   * @param[in] size 初始容量大小
   */
  Column(AttrType attr_type, int attr_len, size_t size = DEFAULT_CAPACITY);

  /**
   * @brief 使用字段元数据初始化列
   * @param[in] meta 字段元数据
   * @param[in] size 初始容量大小
   */
  void init(const FieldMeta &meta, size_t size = DEFAULT_CAPACITY);
  
  /**
   * @brief 使用属性类型和长度初始化列
   * @param[in] attr_type 属性类型
   * @param[in] attr_len 属性长度
   * @param[in] size 初始容量大小
   */
  void init(AttrType attr_type, int attr_len, size_t size = DEFAULT_CAPACITY);
  
  /**
   * @brief 使用值初始化列（常量列）
   * @param[in] value 初始值
   */
  void init(const Value &value);

  /**
   * @brief 析构函数
   */
  virtual ~Column() { reset(); }

  /**
   * @brief 重置列，释放资源并重置所有成员变量
   */
  void reset();

  /**
   * @brief 向列中追加一个值
   * @param[in] data 要追加的数据指针
   * @return 操作成功返回RC::SUCCESS，否则返回错误码
   */
  RC append_one(char *data);

  /**
   * @brief 向 Column 追加写入数据
   * @param[in] data 要被写入数据的起始地址
   * @param[in] count 要写入数据的长度（这里指列值的个数，而不是字节）
   * @return 操作成功返回RC::SUCCESS，否则返回错误码
   */
  RC append(char *data, int count);

  /**
   * @brief 获取 index 位置的列值
   * @param[in] index 要获取的值的索引位置
   * @return 索引位置的值对象
   */
  Value get_value(int index) const;

  /**
   * @brief 获取列数据的实际大小（字节）
   * @return 列数据的字节数
   */
  int data_len() const { return count_ * attr_len_; }

  /**
   * @brief 获取列数据的指针
   * @return 指向列数据的指针
   */
  char *data() const { return data_; }

  /**
   * @brief 重置列数据，但不修改元信息
   */
  void reset_data() { count_ = 0; }

  /**
   * @brief 引用另一个 Column
   * @param[in] column 要引用的列对象
   */
  void reference(const Column &column);

  /**
   * @brief 设置列类型
   * @param[in] column_type 列类型
   */
  void set_column_type(Type column_type) { column_type_ = column_type; }
  
  /**
   * @brief 设置列值数量
   * @param[in] count 列值数量
   */
  void set_count(int count) { count_ = count; }

  /**
   * @brief 获取列值数量
   * @return 列值数量
   */
  int      count() const { return count_; }
  
  /**
   * @brief 获取列容量
   * @return 列容量
   */
  int      capacity() const { return capacity_; }
  
  /**
   * @brief 获取列属性类型
   * @return 列属性类型
   */
  AttrType attr_type() const { return attr_type_; }
  
  /**
   * @brief 获取列属性长度
   * @return 列属性长度
   */
  int      attr_len() const { return attr_len_; }
  
  /**
   * @brief 获取列类型
   * @return 列类型
   */
  Type     column_type() const { return column_type_; }

private:
  /**
   * @brief 默认容量大小
   */
  static constexpr size_t DEFAULT_CAPACITY = 8192;

  /**
   * @brief 列数据缓冲区指针
   */
  char *data_ = nullptr;
  
  /**
   * @brief 当前列值数量
   */
  int count_ = 0;
  
  /**
   * @brief 当前容量，count_ <= capacity_
   */
  int capacity_ = 0;
  
  /**
   * @brief 是否拥有内存
   */
  bool own_ = true;
  
  /**
   * @brief 列属性类型
   */
  AttrType attr_type_ = AttrType::UNDEFINED;
  
  /**
   * @brief 列属性类型长度（目前只支持定长）
   */
  int attr_len_ = -1;
  
  /**
   * @brief 列类型
   */
  Type column_type_ = Type::NORMAL_COLUMN;
};