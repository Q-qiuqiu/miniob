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

#include "common/sys/rc.h"
#include "common/lang/string.h"

/**
 * @brief 前向声明：表元数据类
 */
class TableMeta;

/**
 * @brief 前向声明：字段元数据类
 */
class FieldMeta;

/**
 * @brief 命名空间：JSON处理相关类
 */
namespace Json {
/**
 * @brief JSON值类
 */
class Value;
}  // namespace Json

/**
 * @brief 描述一个索引
 * @ingroup Index
 * @details 一个索引包含了表的哪些字段，索引的名称等。
 * 如果以后实现了多种类型的索引，还需要记录索引的类型，对应类型的一些元数据等
 */
class IndexMeta
{
public:
  /**
   * @brief 默认构造函数
   */
  IndexMeta() = default;

  /**
   * @brief 初始化索引元数据
   * @param name 索引名称
   * @param field 字段元数据对象
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   */
  RC init(const char *name, const FieldMeta &field);

public:
  /**
   * @brief 获取索引名称
   * @return 索引名称的字符串指针
   */
  const char *name() const;
  
  /**
   * @brief 获取索引对应的字段名
   * @return 字段名称的字符串指针
   */
  const char *field() const;

  /**
   * @brief 打印索引信息到输出流
   * @param os 输出流对象
   */
  void desc(ostream &os) const;

public:
  /**
   * @brief 将索引元数据转换为JSON格式
   * @param json_value 输出参数，用于存储转换后的JSON值
   */
  void      to_json(Json::Value &json_value) const;
  
  /**
   * @brief 从JSON格式解析索引元数据
   * @param table 表元数据对象
   * @param json_value 包含索引元数据的JSON值
   * @param index 输出参数，用于存储解析后的索引元数据
   * @return 操作结果，成功返回RC::SUCCESS，失败返回相应错误码
   */
  static RC from_json(const TableMeta &table, const Json::Value &json_value, IndexMeta &index);

protected:
  string name_;   // index's name 索引名称
  string field_;  // field's name 索引对应的字段名
};
