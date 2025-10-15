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
// Created by Wangyunlai on 2021/5/12.
//

#pragma once

#include "common/lang/serializable.h"
#include "common/sys/rc.h"
#include "common/types.h"
#include "common/lang/span.h"
#include "storage/field/field_meta.h"
#include "storage/index/index_meta.h"

/**
 * @brief 表元数据类
 * @details 该类用于存储和管理表的元数据信息，包括表的ID、名称、字段、索引等
 * @note 继承自Serializable接口，支持序列化和反序列化功能
 */
class TableMeta : public common::Serializable
{
public:
  /**
   * @brief 默认构造函数
   */
  TableMeta()          = default;
  
  /**
   * @brief 虚析构函数
   */
  virtual ~TableMeta() = default;

  /**
   * @brief 拷贝构造函数
   * @param other 另一个TableMeta对象
   */
  TableMeta(const TableMeta &other);

  /**
   * @brief 交换两个TableMeta对象的内容
   * @param other 另一个TableMeta对象
   */
  void swap(TableMeta &other) noexcept;

  /**
   * @brief 初始化表元数据
   * @param table_id 表ID
   * @param name 表名
   * @param trx_fields 事务相关字段
   * @param attributes 表的属性信息
   * @param primary_keys 主键字段列表
   * @param storage_format 存储格式
   * @param storage_engine 存储引擎
   * @return 成功返回RC::SUCCESS，失败返回相应的错误码
   */
  RC init(int32_t table_id, const char *name, const vector<FieldMeta> *trx_fields,
      span<const AttrInfoSqlNode> attributes, const vector<string> &primary_keys, StorageFormat storage_format,
      StorageEngine storage_engine);

  /**
   * @brief 添加索引元数据
   * @param index 索引元数据对象
   * @return 成功返回RC::SUCCESS
   */
  RC add_index(const IndexMeta &index);

public:
  /**
   * @brief 获取表ID
   * @return 表的ID值
   */
  int32_t             table_id() const { return table_id_; }
  
  /**
   * @brief 获取表名
   * @return 表名的字符串指针
   */
  const char         *name() const;
  
  /**
   * @brief 获取第一个事务字段
   * @return 事务字段的元数据指针
   */
  const FieldMeta    *trx_field() const;
  
  /**
   * @brief 根据索引获取字段元数据
   * @param index 字段索引
   * @return 字段元数据的常量指针
   */
  const FieldMeta    *field(int index) const;
  
  /**
   * @brief 根据名称获取字段元数据
   * @param name 字段名称
   * @return 字段元数据的常量指针，若不存在则返回nullptr
   */
  const FieldMeta    *field(const char *name) const;
  
  /**
   * @brief 根据偏移量查找字段元数据
   * @param offset 字段在记录中的偏移量
   * @return 字段元数据的常量指针，若不存在则返回nullptr
   */
  const FieldMeta    *find_field_by_offset(int offset) const;
  
  /**
   * @brief 获取所有字段元数据
   * @return 字段元数据列表的指针
   */
  auto                field_metas() const -> const vector<FieldMeta>                *{ return &fields_; }
  
  /**
   * @brief 获取事务字段范围
   * @return 事务字段的span对象
   */
  auto                trx_fields() const -> span<const FieldMeta>;
  
  /**
   * @brief 获取存储格式
   * @return 存储格式枚举值
   */
  const StorageFormat storage_format() const { return storage_format_; }
  
  /**
   * @brief 获取存储引擎
   * @return 存储引擎枚举值
   */
  const StorageEngine storage_engine() const { return storage_engine_; }

  /**
   * @brief 获取字段总数（包括系统字段）
   * @return 字段总数
   */
  int field_num() const;  // sys field included
  
  /**
   * @brief 获取系统字段数量
   * @return 系统字段数量
   */
  int sys_field_num() const;

  /**
   * @brief 根据名称获取索引元数据
   * @param name 索引名称
   * @return 索引元数据的常量指针，若不存在则返回nullptr
   */
  const IndexMeta *index(const char *name) const;
  
  /**
   * @brief 根据字段名查找索引元数据
   * @param field 字段名称
   * @return 索引元数据的常量指针，若不存在则返回nullptr
   */
  const IndexMeta *find_index_by_field(const char *field) const;
  
  /**
   * @brief 根据索引获取索引元数据
   * @param i 索引位置
   * @return 索引元数据的常量指针
   */
  const IndexMeta *index(int i) const;
  
  /**
   * @brief 获取索引数量
   * @return 索引数量
   */
  int              index_num() const;

  /**
   * @brief 获取主键字段列表
   * @return 主键字段名称的常量引用
   */
  const vector<string> &primary_keys() const { return primary_keys_; }

  /**
   * @brief 获取记录大小
   * @return 记录的字节大小
   */
  int record_size() const;

public:
  /**
   * @brief 序列化表元数据到输出流
   * @param os 输出流
   * @return 序列化的字节数，失败返回-1
   */
  int  serialize(ostream &os) const override;
  
  /**
   * @brief 从输入流反序列化表元数据
   * @param is 输入流
   * @return 反序列化的字节数，失败返回-1
   */
  int  deserialize(istream &is) override;
  
  /**
   * @brief 获取序列化大小
   * @return 序列化大小（当前返回-1，表示未实现）
   */
  int  get_serial_size() const override;
  
  /**
   * @brief 转换为字符串表示
   * @param output 输出字符串
   */
  void to_string(string &output) const override;
  
  /**
   * @brief 输出表元数据的详细描述
   * @param os 输出流
   */
  void desc(ostream &os) const;

protected:
  int32_t           table_id_ = -1;          ///< 表的ID
  string            name_;                   ///< 表名
  vector<FieldMeta> trx_fields_;             ///< 事务相关字段
  vector<FieldMeta> fields_;                 ///< 所有字段（包含系统字段）
  vector<IndexMeta> indexes_;                ///< 所有索引
  vector<string>    primary_keys_;           ///< 主键字段列表
  StorageFormat     storage_format_;         ///< 存储格式
  StorageEngine     storage_engine_;         ///< 存储引擎
  int               record_size_ = 0;        ///< 记录大小（字节）
};
