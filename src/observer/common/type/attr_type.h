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

/**
 * @brief 属性的类型定义
 * @file attr_type.h
 * @details 该文件定义了数据库中支持的各种数据类型，用于表的属性定义、数据存储和操作。
 */

/**
 * @brief 属性的类型枚举
 * @details AttrType 枚举列出了数据库支持的各种数据类型，包括字符串、整数、浮点数等。
 */
enum class AttrType
{
  UNDEFINED,  ///< 未定义类型
  CHARS,      ///< 字符串类型，可变长度的字符序列
  INTS,       ///< 整数类型，4字节有符号整数
  FLOATS,     ///< 浮点数类型，4字节单精度浮点数
  VECTORS,    ///< 向量类型，用于存储多维数据
  BOOLEANS,   ///< 布尔类型，程序内部使用，不直接由解析器生成
  MAXTYPE,    ///< 类型边界标记，请在 UNDEFINED 与 MAXTYPE 之间增加新类型
};

/**
 * @brief 将属性类型转换为字符串表示
 * @param type 要转换的属性类型
 * @return 对应属性类型的字符串表示，如果类型无效则返回"unknown"
 */
const char *attr_type_to_string(AttrType type);

/**
 * @brief 从字符串解析属性类型
 * @param s 表示属性类型的字符串
 * @return 对应的属性类型枚举值，如果无法解析则返回 UNDEFINED
 * @note 此函数不区分大小写进行比较
 */
AttrType    attr_type_from_string(const char *s);
