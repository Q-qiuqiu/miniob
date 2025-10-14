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
// Created by Wangyunlai 2023/6/27
//

#pragma once

#include "common/lang/string.h"
#include "common/lang/memory.h"
#include "common/type/attr_type.h"
#include "common/type/data_type.h"

/**
 * @brief 属性的值
 * @ingroup DataType
 * @details 与DataType（数据类型）配套完成各种算术运算、比较、类型转换等操作。
 * 这里同时记录了数据的值与类型。当需要对值做运算时，建议使用类似Value::add的操作而不是DataType::add。
 * 在进行运算前，应该设置好结果的类型，比如进行两个INT类型的除法运算时，结果类型应该设置为FLOAT。
 */
class Value final
{
public:
  friend class DataType;
  friend class IntegerType;
  friend class FloatType;
  friend class BooleanType;
  friend class CharType;
  friend class VectorType;

  /**
   * @brief 默认构造函数，创建一个未定义类型的值
   */
  Value() = default;

  /**
   * @brief 析构函数，释放可能占用的内存资源
   */
  ~Value() { reset(); }

  /**
   * @brief 构造函数，从指定数据创建一个值
   * @param[in] attr_type 属性类型
   * @param[in] data 数据指针
   * @param[in] length 数据长度，默认为4
   */
  Value(AttrType attr_type, char *data, int length = 4) : attr_type_(attr_type) { this->set_data(data, length); }

  /**
   * @brief 构造函数，从整数创建一个值
   * @param[in] val 整数值
   */
  explicit Value(int val);
  
  /**
   * @brief 构造函数，从浮点数创建一个值
   * @param[in] val 浮点数值
   */
  explicit Value(float val);
  
  /**
   * @brief 构造函数，从布尔值创建一个值
   * @param[in] val 布尔值
   */
  explicit Value(bool val);
  
  /**
   * @brief 构造函数，从字符串创建一个值
   * @param[in] s 字符串指针
   * @param[in] len 字符串长度，默认为0（表示自动计算长度）
   */
  explicit Value(const char *s, int len = 0);

  /**
   * @brief 拷贝构造函数
   * @param[in] other 要拷贝的Value对象
   */
  Value(const Value &other);
  
  /**
   * @brief 移动构造函数
   * @param[in] other 要移动的Value对象
   */
  Value(Value &&other);

  /**
   * @brief 拷贝赋值运算符
   * @param[in] other 要拷贝的Value对象
   * @return 当前Value对象的引用
   */
  Value &operator=(const Value &other);
  
  /**
   * @brief 移动赋值运算符
   * @param[in] other 要移动的Value对象
   * @return 当前Value对象的引用
   */
  Value &operator=(Value &&other);

  /**
   * @brief 重置Value对象，释放资源并将类型设置为未定义
   */
  void reset();

  /**
   * @brief 执行加法运算
   * @param[in] left 左操作数
   * @param[in] right 右操作数
   * @param[out] result 运算结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC add(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->add(left, right, result);
  }

  /**
   * @brief 执行减法运算
   * @param[in] left 左操作数
   * @param[in] right 右操作数
   * @param[out] result 运算结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC subtract(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->subtract(left, right, result);
  }

  /**
   * @brief 执行乘法运算
   * @param[in] left 左操作数
   * @param[in] right 右操作数
   * @param[out] result 运算结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC multiply(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->multiply(left, right, result);
  }

  /**
   * @brief 执行除法运算
   * @param[in] left 左操作数
   * @param[in] right 右操作数
   * @param[out] result 运算结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC divide(const Value &left, const Value &right, Value &result)
  {
    return DataType::type_instance(result.attr_type())->divide(left, right, result);
  }

  /**
   * @brief 执行取负运算
   * @param[in] value 操作数
   * @param[out] result 运算结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC negative(const Value &value, Value &result)
  {
    return DataType::type_instance(result.attr_type())->negative(value, result);
  }

  /**
   * @brief 类型转换
   * @param[in] value 源值
   * @param[in] to_type 目标类型
   * @param[out] result 转换后的结果
   * @return 操作结果，成功返回RC::SUCCESS
   */
  static RC cast_to(const Value &value, AttrType to_type, Value &result)
  {
    return DataType::type_instance(value.attr_type())->cast_to(value, to_type, result);
  }

  /**
   * @brief 设置值的类型
   * @param[in] type 目标类型
   */
  void set_type(AttrType type) { this->attr_type_ = type; }
  
  /**
   * @brief 设置值的数据
   * @param[in] data 数据指针
   * @param[in] length 数据长度
   */
  void set_data(char *data, int length);
  
  /**
   * @brief 设置值的数据（常量版本）
   * @param[in] data 常量数据指针
   * @param[in] length 数据长度
   */
  void set_data(const char *data, int length) { this->set_data(const_cast<char *>(data), length); }
  
  /**
   * @brief 从另一个Value对象设置值
   * @param[in] value 源Value对象
   */
  void set_value(const Value &value);
  
  /**
   * @brief 设置布尔值
   * @param[in] val 布尔值
   */
  void set_boolean(bool val);

  /**
   * @brief 将值转换为字符串表示
   * @return 字符串表示形式
   */
  string to_string() const;

  /**
   * @brief 比较两个值的大小
   * @param[in] other 要比较的另一个值
   * @return 比较结果，小于0表示当前值小于other，等于0表示相等，大于0表示当前值大于other
   */
  int compare(const Value &other) const;

  /**
   * @brief 获取值的原始数据指针
   * @return 数据指针
   */
  const char *data() const;

  /**
   * @brief 获取值的长度
   * @return 长度值
   */
  int      length() const { return length_; }
  
  /**
   * @brief 获取值的属性类型
   * @return 属性类型
   */
  AttrType attr_type() const { return attr_type_; }

public:
  /**
   * 获取对应的值
   * 如果当前的类型与期望获取的类型不符，就会执行转换操作
   */
  /**
   * @brief 获取整数值
   * @return 转换后的整数值
   */
  int    get_int() const;
  
  /**
   * @brief 获取浮点数值
   * @return 转换后的浮点数值
   */
  float  get_float() const;
  
  /**
   * @brief 获取字符串值
   * @return 字符串表示形式
   */
  string get_string() const;
  
  /**
   * @brief 获取布尔值
   * @return 转换后的布尔值
   */
  bool   get_boolean() const;

public:
  /**
   * @brief 设置整数值
   * @param[in] val 整数值
   */
  void set_int(int val);
  
  /**
   * @brief 设置浮点数值
   * @param[in] val 浮点数值
   */
  void set_float(float val);
  
  /**
   * @brief 设置字符串值
   * @param[in] s 字符串指针
   * @param[in] len 字符串长度，默认为0（表示自动计算长度）
   */
  void set_string(const char *s, int len = 0);
  
  /**
   * @brief 从另一个Value对象设置字符串值
   * @param[in] other 源Value对象
   */
  void set_string_from_other(const Value &other);

private:
  /// 属性类型，默认为UNDEFINED
  AttrType attr_type_ = AttrType::UNDEFINED;
  
  /// 数据长度
  int      length_    = 0;

  /**
   * @brief 值的联合体，用于存储不同类型的数据
   * @details 根据attr_type_的值，使用不同的成员
   */
  union Val
  {
    int32_t int_value_;     ///< 整数值
    float   float_value_;   ///< 浮点数值
    bool    bool_value_;    ///< 布尔值
    char   *pointer_value_; ///< 指针值，用于字符串等
  } value_ = {.int_value_ = 0};

  /// 是否申请并占有内存, 目前对于 CHARS 类型 own_data_ 为true, 其余类型 own_data_ 为false
  bool own_data_ = false;
};
