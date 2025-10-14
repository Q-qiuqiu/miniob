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

#include "common/lang/array.h"
#include "common/lang/memory.h"
#include "common/lang/string.h"
#include "common/sys/rc.h"
#include "common/type/attr_type.h"

class Value;

/**
 * @brief 定义了数据类型相关的操作，比如比较运算、算术运算等
 * @defgroup DataType
 * @details 数据类型定义的算术运算中，比如 add、subtract 等，将按照当前数据类型设置最终结果值的类型。
 * 参与运算的参数类型不一定相同，不同的类型进行运算是否能够支持需要参考各个类型的实现。
 */
class DataType
{
public:
  /**
   * @brief 构造函数
   * @param attr_type 属性类型
   */
  explicit DataType(AttrType attr_type) : attr_type_(attr_type) {}

  /**
   * @brief 析构函数
   */
  virtual ~DataType() = default;

  /**
   * @brief 获取指定属性类型的 DataType 实例
   * @param attr_type 属性类型
   * @return DataType* 指定属性类型的实例指针
   */
  inline static DataType *type_instance(AttrType attr_type)
  {
    return type_instances_.at(static_cast<int>(attr_type)).get();
  }

  /**
   * @brief 获取当前数据类型的属性类型
   * @return AttrType 当前数据类型的属性类型
   */
  inline AttrType get_attr_type() const { return attr_type_; }

  /**
   * @brief 比较两个值的大小
   * @param left 左边的值
   * @param right 右边的值
   * @return 
   *  -1 表示 left < right
   *  0 表示 left = right
   *  1 表示 left > right
   *  INT32_MAX 表示未实现的比较
   */
  virtual int compare(const Value &left, const Value &right) const { return INT32_MAX; }

  /**
   * @brief 计算 left + right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC add(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left - right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC subtract(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left * right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC multiply(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 left / right，并将结果保存到 result 中
   * @param left 左边的操作数
   * @param right 右边的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC divide(const Value &left, const Value &right, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算 -val，并将结果保存到 result 中
   * @param val 要取负值的操作数
   * @param result 用于存储计算结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC negative(const Value &val, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 type 类型，并将结果保存到 result 中
   * @param val 要转换的值
   * @param type 目标类型
   * @param result 用于存储转换结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC cast_to(const Value &val, AttrType type, Value &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 将 val 转换为 string，并将结果保存到 result 中
   * @param val 要转换的值
   * @param result 用于存储转换结果
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC to_string(const Value &val, string &result) const { return RC::UNSUPPORTED; }

  /**
   * @brief 计算从 type 到 attr_type 的隐式转换的 cost，如果无法转换，返回 INT32_MAX
   * @param type 源类型
   * @return 转换成本，相同类型返回 0，无法转换返回 INT32_MAX
   */
  virtual int cast_cost(AttrType type)
  {
    if (type == attr_type_) {
      return 0;
    }
    return INT32_MAX;
  }

  /**
   * @brief 从字符串设置值
   * @param val 要设置值的对象
   * @param data 字符串数据
   * @return 执行结果，成功返回 RC::SUCCESS，否则返回错误码
   */
  virtual RC set_value_from_str(Value &val, const string &data) const { return RC::UNSUPPORTED; }

protected:
  ///< 属性类型
  AttrType attr_type_;

  ///< 存储所有数据类型实例的静态数组
  static array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> type_instances_;
};
