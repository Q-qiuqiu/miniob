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
// Created by WangYunlai on 2023/06/28.
//

#include "common/value.h"

#include "common/lang/comparator.h"
#include "common/lang/exception.h"
#include "common/lang/sstream.h"
#include "common/lang/string.h"
#include "common/log/log.h"

/**
 * @brief 从整数创建Value对象的构造函数实现
 * @param[in] val 整数值
 */
Value::Value(int val) { set_int(val); }

/**
 * @brief 从浮点数创建Value对象的构造函数实现
 * @param[in] val 浮点数值
 */
Value::Value(float val) { set_float(val); }

/**
 * @brief 从布尔值创建Value对象的构造函数实现
 * @param[in] val 布尔值
 */
Value::Value(bool val) { set_boolean(val); }

/**
 * @brief 从字符串创建Value对象的构造函数实现
 * @param[in] s 字符串指针
 * @param[in] len 字符串长度，默认为0
 */
Value::Value(const char *s, int len /*= 0*/) { set_string(s, len); }

/**
 * @brief 拷贝构造函数实现
 * @param[in] other 要拷贝的Value对象
 * 
 * 拷贝other的所有属性，并根据类型进行适当的数据拷贝。
 * 对于CHARS类型，会调用set_string_from_other进行深拷贝。
 */
Value::Value(const Value &other)
{
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  switch (this->attr_type_) {
    case AttrType::CHARS: {  // 对于字符串类型，需要进行深拷贝
      set_string_from_other(other);
    } break;

    default: {  // 对于其他类型，可以直接拷贝值
      this->value_ = other.value_;
    } break;
  }
}

/**
 * @brief 移动构造函数实现
 * @param[in] other 要移动的Value对象
 * 
 * 移动other的所有资源到当前对象，并将other重置为安全状态。
 */
Value::Value(Value &&other)
{
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  this->value_     = other.value_;
  other.own_data_  = false;  // 防止other析构时释放资源
  other.length_    = 0;
}

/**
 * @brief 拷贝赋值运算符实现
 * @param[in] other 要拷贝的Value对象
 * @return 当前Value对象的引用
 * 
 * 先重置当前对象，然后拷贝other的所有属性和数据。
 */
Value &Value::operator=(const Value &other)
{
  if (this == &other) {  // 自赋值检查
    return *this;
  }
  reset();  // 先释放当前资源
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  switch (this->attr_type_) {
    case AttrType::CHARS: {  // 对于字符串类型，需要进行深拷贝
      set_string_from_other(other);
    } break;

    default: {  // 对于其他类型，可以直接拷贝值
      this->value_ = other.value_;
    } break;
  }
  return *this;
}

/**
 * @brief 移动赋值运算符实现
 * @param[in] other 要移动的Value对象
 * @return 当前Value对象的引用
 * 
 * 先重置当前对象，然后移动other的资源到当前对象。
 */
Value &Value::operator=(Value &&other)
{
  if (this == &other) {  // 自赋值检查
    return *this;
  }
  reset();  // 先释放当前资源
  this->attr_type_ = other.attr_type_;
  this->length_    = other.length_;
  this->own_data_  = other.own_data_;
  this->value_     = other.value_;
  other.own_data_  = false;  // 防止other析构时释放资源
  other.length_    = 0;
  return *this;
}

/**
 * @brief 重置Value对象实现
 * 
 * 释放可能占用的内存资源，并将所有成员变量重置为初始状态。
 */
void Value::reset()
{
  switch (attr_type_) {
    case AttrType::CHARS:
      if (own_data_ && value_.pointer_value_ != nullptr) {  // 如果是自己分配的字符串内存，则释放
        delete[] value_.pointer_value_;
        value_.pointer_value_ = nullptr;
      }
      break;
    default: break;
  }

  attr_type_ = AttrType::UNDEFINED;
  length_    = 0;
  own_data_  = false;
}

/**
 * @brief 设置值的数据实现
 * @param[in] data 数据指针
 * @param[in] length 数据长度
 * 
 * 根据当前的属性类型，将data中的数据存储到value_的相应成员中。
 */
void Value::set_data(char *data, int length)
{
  switch (attr_type_) {
    case AttrType::CHARS: {  // 字符串类型
      set_string(data, length);
    } break;
    case AttrType::INTS: {  // 整数类型
      value_.int_value_ = *(int *)data;
      length_           = length;
    } break;
    case AttrType::FLOATS: {  // 浮点数类型
      value_.float_value_ = *(float *)data;
      length_             = length;
    } break;
    case AttrType::BOOLEANS: {  // 布尔类型
      value_.bool_value_ = *(int *)data != 0;
      length_            = length;
    } break;
    default: {  // 未知类型
      LOG_WARN("unknown data type: %d", attr_type_);
    } break;
  }
}

/**
 * @brief 设置整数值实现
 * @param[in] val 整数值
 * 
 * 先重置对象，然后设置类型为INTS，并存储整数值。
 */
void Value::set_int(int val)
{
  reset();
  attr_type_        = AttrType::INTS;
  value_.int_value_ = val;
  length_           = sizeof(val);
}

/**
 * @brief 设置浮点数值实现
 * @param[in] val 浮点数值
 * 
 * 先重置对象，然后设置类型为FLOATS，并存储浮点数值。
 */
void Value::set_float(float val)
{
  reset();
  attr_type_          = AttrType::FLOATS;
  value_.float_value_ = val;
  length_             = sizeof(val);
}

/**
 * @brief 设置布尔值实现
 * @param[in] val 布尔值
 * 
 * 先重置对象，然后设置类型为BOOLEANS，并存储布尔值。
 */
void Value::set_boolean(bool val)
{
  reset();
  attr_type_         = AttrType::BOOLEANS;
  value_.bool_value_ = val;
  length_            = sizeof(val);
}

/**
 * @brief 设置字符串值实现
 * @param[in] s 字符串指针
 * @param[in] len 字符串长度，默认为0
 * 
 * 先重置对象，然后设置类型为CHARS，并根据需要分配内存存储字符串。
 * 如果len为0，则自动计算字符串长度。
 */
void Value::set_string(const char *s, int len /*= 0*/)
{
  reset();
  attr_type_ = AttrType::CHARS;
  if (s == nullptr) {  // 空字符串处理
    value_.pointer_value_ = nullptr;
    length_               = 0;
  } else {
    own_data_ = true;  // 标记为自己分配内存
    if (len > 0) {
      len = strnlen(s, len);  // 计算指定长度内的实际字符串长度
    } else {
      len = strlen(s);  // 自动计算字符串长度
    }
    value_.pointer_value_ = new char[len + 1];  // 分配内存，+1 用于存储结束符
    length_               = len;
    memcpy(value_.pointer_value_, s, len);  // 复制字符串内容
    value_.pointer_value_[len] = '\0';  // 添加结束符
  }
}

/**
 * @brief 从另一个Value对象设置值实现
 * @param[in] value 源Value对象
 * 
 * 根据value的类型，调用相应的set_xxx方法设置值。
 */
void Value::set_value(const Value &value)
{
  switch (value.attr_type_) {
    case AttrType::INTS: {
      set_int(value.get_int());
    } break;
    case AttrType::FLOATS: {
      set_float(value.get_float());
    } break;
    case AttrType::CHARS: {
      set_string(value.get_string().c_str());
    } break;
    case AttrType::BOOLEANS: {
      set_boolean(value.get_boolean());
    } break;
    default: {
      ASSERT(false, "got an invalid value type");
    } break;
  }
}

/**
 * @brief 从另一个Value对象设置字符串值实现
 * @param[in] other 源Value对象
 * 
 * 假设当前对象的类型已经是CHARS，从other复制字符串内容。
 */
void Value::set_string_from_other(const Value &other)
{
  ASSERT(attr_type_ == AttrType::CHARS, "attr type is not CHARS");
  if (own_data_ && other.value_.pointer_value_ != nullptr && length_ != 0) {
    this->value_.pointer_value_ = new char[this->length_ + 1];
    memcpy(this->value_.pointer_value_, other.value_.pointer_value_, this->length_);
    this->value_.pointer_value_[this->length_] = '\0';
  }
}

/**
 * @brief 获取值的原始数据指针实现
 * @return 数据指针
 * 
 * 根据当前的属性类型，返回相应的数据指针。
 */
const char *Value::data() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {  // 字符串类型，直接返回指针
      return value_.pointer_value_;
    } break;
    default: {  // 其他类型，返回value_的地址
      return (const char *)&value_;
    } break;
  }
}

/**
 * @brief 将值转换为字符串表示实现
 * @return 字符串表示形式
 * 
 * 调用相应的DataType实例的to_string方法进行转换。
 * 如果转换失败，记录警告日志并返回空字符串。
 */
string Value::to_string() const
{
  string res;
  RC     rc = DataType::type_instance(this->attr_type_)->to_string(*this, res);
  if (OB_FAIL(rc)) {
    LOG_WARN("failed to convert value to string. type=%s", attr_type_to_string(this->attr_type_));
    return "";
  }
  return res;
}

/**
 * @brief 比较两个值的大小实现
 * @param[in] other 要比较的另一个值
 * @return 比较结果，小于0表示当前值小于other，等于0表示相等，大于0表示当前值大于other
 * 
 * 调用相应的DataType实例的compare方法进行比较。
 */
int Value::compare(const Value &other) const { return DataType::type_instance(this->attr_type_)->compare(*this, other); }

/**
 * @brief 获取整数值实现
 * @return 转换后的整数值
 * 
 * 根据当前的属性类型，将值转换为整数返回。
 * 对于无法转换的情况，返回0并记录日志。
 */
int Value::get_int() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {  // 字符串转换为整数
      try {
        return (int)(stol(value_.pointer_value_));
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to number. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return 0;
      }
    }
    case AttrType::INTS: {  // 直接返回整数值
      return value_.int_value_;
    }
    case AttrType::FLOATS: {  // 浮点数转换为整数
      return (int)(value_.float_value_);
    }
    case AttrType::BOOLEANS: {  // 布尔值转换为整数
      return (int)(value_.bool_value_);
    }
    default: {  // 未知类型
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

/**
 * @brief 获取浮点数值实现
 * @return 转换后的浮点数值
 * 
 * 根据当前的属性类型，将值转换为浮点数返回。
 * 对于无法转换的情况，返回0.0并记录日志。
 */
float Value::get_float() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {  // 字符串转换为浮点数
      try {
        return stof(value_.pointer_value_);
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to float. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return 0.0;
      }
    } break;
    case AttrType::INTS: {  // 整数转换为浮点数
      return float(value_.int_value_);
    } break;
    case AttrType::FLOATS: {  // 直接返回浮点数值
      return value_.float_value_;
    } break;
    case AttrType::BOOLEANS: {  // 布尔值转换为浮点数
      return float(value_.bool_value_);
    } break;
    default: {  // 未知类型
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return 0;
    }
  }
  return 0;
}

/**
 * @brief 获取字符串值实现
 * @return 字符串表示形式
 * 
 * 调用to_string方法获取字符串表示。
 */
string Value::get_string() const { return this->to_string(); }

/**
 * @brief 获取布尔值实现
 * @return 转换后的布尔值
 * 
 * 根据当前的属性类型，将值转换为布尔值返回。
 * 对于数字类型，非零值为true，零为false。
 * 对于字符串，尝试转换为数字判断，或者检查是否为空字符串。
 */
bool Value::get_boolean() const
{
  switch (attr_type_) {
    case AttrType::CHARS: {  // 字符串转换为布尔值
      try {
        float val = stof(value_.pointer_value_);
        if (val >= EPSILON || val <= -EPSILON) {  // 浮点数非零
          return true;
        }

        int int_val = stol(value_.pointer_value_);
        if (int_val != 0) {  // 整数非零
          return true;
        }

        return value_.pointer_value_ != nullptr;  // 非空字符串
      } catch (exception const &ex) {
        LOG_TRACE("failed to convert string to float or integer. s=%s, ex=%s", value_.pointer_value_, ex.what());
        return value_.pointer_value_ != nullptr;  // 非空字符串
      }
    } break;
    case AttrType::INTS: {  // 整数转换为布尔值
      return value_.int_value_ != 0;
    } break;
    case AttrType::FLOATS: {  // 浮点数转换为布尔值
      float val = value_.float_value_;
      return val >= EPSILON || val <= -EPSILON;
    } break;
    case AttrType::BOOLEANS: {  // 直接返回布尔值
      return value_.bool_value_;
    } break;
    default: {  // 未知类型
      LOG_WARN("unknown data type. type=%d", attr_type_);
      return false;
    }
  }
  return false;
}
