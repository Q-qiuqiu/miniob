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
// Created by Wangyunlai on 2022/5/4.
//

#pragma once

#include <stddef.h>

#include "common/log/log.h"
#include "common/sys/rc.h"
#include "common/types.h"
#include "common/lang/vector.h"
#include "common/lang/sstream.h"
#include "common/lang/limits.h"
#include "storage/field/field_meta.h"
#include "storage/index/index_meta.h"

class Field;

/**
 * @brief 记录标识符（Record Identifier）
 * @details 用于唯一标识数据库中一条记录的位置
 * 一个记录存储在某个文件的特定页面的特定槽位中，RID记录了页面号和槽位号
 * 不包含文件信息，通常与特定表或文件相关联使用
 */
struct RID
{
  /**
   * @brief 记录所在的页面编号
   */
  PageNum page_num;  ///< 记录所在的页面编号
  
  /**
   * @brief 记录在页面中的槽位编号
   */
  SlotNum slot_num;  ///< 记录在页面中的槽位编号

  /**
   * @brief 默认构造函数
   * @details 创建一个默认的RID对象，page_num和slot_num均为默认值
   */
  RID() = default;
  
  /**
   * @brief 构造函数
   * @param _page_num 页面编号
   * @param _slot_num 槽位编号
   */
  RID(const PageNum _page_num, const SlotNum _slot_num) : page_num(_page_num), slot_num(_slot_num) {}

  /**
   * @brief 将RID转换为字符串表示
   * @return 格式化的字符串，包含页面号和槽位号
   */
  const string to_string() const
  {
    stringstream ss;
    ss << "PageNum:" << page_num << ", SlotNum:" << slot_num;
    return ss.str();
  }

  /**
   * @brief 相等运算符重载
   * @param other 另一个RID对象
   * @return 如果两个RID标识同一个记录位置则返回true，否则返回false
   */
  bool operator==(const RID &other) const { return page_num == other.page_num && slot_num == other.slot_num; }

  /**
   * @brief 不等运算符重载
   * @param other 另一个RID对象
   * @return 如果两个RID标识不同记录位置则返回true，否则返回false
   */
  bool operator!=(const RID &other) const { return !(*this == other); }

  /**
   * @brief 比较两个RID的大小
   * @param rid1 第一个RID对象的指针
   * @param rid2 第二个RID对象的指针
   * @return 如果rid1小于rid2返回负数，如果相等返回0，如果rid1大于rid2返回正数
   * 先比较页面号，页面号相同时再比较槽位号
   */
  static int compare(const RID *rid1, const RID *rid2)
  {
    int page_diff = rid1->page_num - rid2->page_num;
    if (page_diff != 0) {
      return page_diff;
    } else {
      return rid1->slot_num - rid2->slot_num;
    }
  }

  /**
   * @brief 获取最小的RID值
   * @details 返回一个可能不会实际使用的最小RID值
   * 虽然page_num=0和slot_num=0在技术上是合法的，但通常page_num=0用于存储元数据，
   * 对数据部分来说是不合法的。这个值主要在B+树查找中使用
   * @return 指向最小RID的静态指针
   */
  static RID *min()
  {
    static RID rid{0, 0};
    return &rid;
  }

  /**
   * @brief 获取最大的RID值
   * @details 返回一个理论上的最大RID值，使用数值类型的最大值
   * 假设实际使用中page_num和slot_num不会达到对应类型的最大值
   * @return 指向最大RID的静态指针
   */
  static RID *max()
  {
    static RID rid{numeric_limits<PageNum>::max(), numeric_limits<SlotNum>::max()};
    return &rid;
  }
};

/**
 * @brief RID的哈希函数
 * @details 为RID提供哈希运算，使其可以用作哈希表的键
 */
struct RIDHash
{
  /**
   * @brief 计算RID的哈希值
   * @param rid 要计算哈希值的RID对象
   * @return RID的哈希值
   * @note 使用异或操作组合page_num和slot_num的哈希值
   */
  size_t operator()(const RID &rid) const noexcept
  {
    return hash<PageNum>()(rid.page_num) ^ hash<SlotNum>()(rid.slot_num);
  }
};

/**
 * @brief 数据库记录类
 * @details 表示数据库中的一条记录，包含记录的位置信息和实际数据
 * 当前的记录都是连续存放的空间（内存或磁盘上）。
 * 为了提高访问的效率，Record通常直接记录指向页面上的内存，但需要保证访问时持有锁资源。
 * 同时提供了内存复制管理的方法，可通过set_data_owner设置是否由Record管理内存
 * @note 可考虑拆分成两种实现：一种需要自己管理内存，一种不需要自己管理内存
 */
class Record
{
public:
  /**
   * @brief 默认构造函数
   * @details 创建一个空的Record对象，不分配任何内存
   */
  Record() = default;
  
  /**
   * @brief 析构函数
   * @details 如果Record拥有内存（owner_为true），则释放data_指向的内存
   */
  ~Record()
  {
    if (owner_ && data_ != nullptr) {
      free(data_);
      data_ = nullptr;
    }
  }

  /**
   * @brief 拷贝构造函数
   * @details 创建一个新的Record对象，复制另一个Record对象的内容
   * 如果源对象拥有内存，则为新对象分配新的内存并复制数据
   * @param other 要复制的Record对象
   */
  Record(const Record &other)
  {
    rid_   = other.rid_;
    key_   = other.key_;
    data_  = other.data_;
    len_   = other.len_;
    owner_ = other.owner_;

    if (other.owner_) {
      char *tmp = (char *)malloc(other.len_);
      ASSERT(nullptr != tmp, "failed to allocate memory. size=%d", other.len_);
      memcpy(tmp, other.data_, other.len_);
      data_ = tmp;
    }
  }

  /**
   * @brief 拷贝赋值运算符
   * @details 将一个Record对象的内容复制到另一个已存在的Record对象
   * 如果当前对象不拥有内存或者内存大小不匹配，则调用析构函数后重新构造
   * 否则直接复制数据
   * @param other 要复制的Record对象
   * @return 当前Record对象的引用
   */
  Record &operator=(const Record &other)
  {
    if (this == &other) {
      return *this;
    }

    if (!owner_ || len_ != other.len_) {
      this->~Record();
      new (this) Record(other);
      return *this;
    }
    this->rid_ = other.rid_;
    this->key_ = other.key_;
    memcpy(data_, other.data_, other.len_);
    return *this;
  }

  /**
   * @brief 移动构造函数
   * @details 从另一个Record对象移动资源，接管其内存所有权
   * 源对象在移动后将不再拥有数据，其data_指针将被设置为nullptr
   * @param other 要移动的Record对象
   */
  Record(Record &&other)
  {
    rid_ = other.rid_;
    key_ = other.key_;
    if (!other.owner_) {
      data_        = other.data_;
      len_         = other.len_;
      other.data_  = nullptr;
      other.len_   = 0;
      this->owner_ = false;
    } else {
      data_        = other.data_;
      len_         = other.len_;
      other.data_  = nullptr;
      other.len_   = 0;
      this->owner_ = true;
    }
  }

  /**
   * @brief 移动赋值运算符
   * @details 将一个Record对象的资源移动到另一个已存在的Record对象
   * 目标对象会先释放自己的资源，然后接管源对象的资源
   * @param other 要移动的Record对象
   * @return 当前Record对象的引用
   */
  Record &operator=(Record &&other)
  {
    if (this == &other) {
      return *this;
    }

    this->~Record();
    new (this) Record(std::move(other));
    return *this;
  }

  /**
   * @brief 设置记录数据
   * @details 设置数据指针和长度，但不获取内存所有权
   * @param data 指向记录数据的指针
   * @param len 记录数据的长度，如果不提供则为0
   */
  void set_data(char *data, int len = 0)
  {
    this->data_ = data;
    this->len_  = len;
  }
  
  /**
   * @brief 设置记录数据并获取内存所有权
   * @details 设置数据指针和长度，并将owner_设置为true，表示Record负责管理内存
   * 在设置新数据前，会先释放旧数据（如果拥有所有权）
   * @param data 指向记录数据的指针
   * @param len 记录数据的长度，必须大于0
   */
  void set_data_owner(char *data, int len)
  {
    ASSERT(len != 0, "the len of data should not be 0");
    this->~Record();

    this->data_  = data;
    this->len_   = len;
    this->owner_ = true;
  }

  /**
   * @brief 复制数据到记录
   * @details 分配新内存并复制数据，获取内存所有权
   * @param data 源数据指针
   * @param len 数据长度
   * @return 操作结果，成功返回SUCCESS，内存分配失败返回NOMEM
   */
  RC copy_data(const char *data, int len)
  {
    ASSERT(len!= 0, "the len of data should not be 0");
    char *tmp = (char *)malloc(len);
    if (nullptr == tmp) {
      LOG_WARN("failed to allocate memory. size=%d", len);
      return RC::NOMEM;
    }

    memcpy(tmp, data, len);
    set_data_owner(tmp, len);
    return RC::SUCCESS;
  }

  /**
   * @brief 创建新的记录
   * @details 分配指定长度的内存，用于存储新记录
   * @param len 记录长度
   * @return 操作结果，成功返回SUCCESS，内存分配失败返回NOMEM
   */
  RC new_record(int len)
  {
    ASSERT(len!= 0, "the len of data should not be 0");
    char *tmp = (char *)malloc(len);
    if (nullptr == tmp) {
      LOG_WARN("failed to allocate memory. size=%d", len);
      return RC::NOMEM;
    }
    set_data_owner(tmp, len);
    return RC::SUCCESS;
  }

  /**
   * @brief 设置记录中指定字段的值
   * @details 修改记录中特定偏移量和长度的字段数据
   * @param field_offset 字段在记录中的偏移量
   * @param field_len 字段的长度
   * @param data 字段的新值
   * @return 操作结果，成功返回SUCCESS，没有内存所有权返回INTERNAL，参数无效返回INVALID_ARGUMENT
   */
  RC set_field(int field_offset, int field_len, char *data)
  {
    if (!owner_) {
      LOG_ERROR("cannot set field when record does not own the memory");
      return RC::INTERNAL;
    }
    if (field_offset + field_len > len_) {
      LOG_ERROR("invalid offset or length. offset=%d, length=%d, total length=%d", field_offset, field_len, len_);
      return RC::INVALID_ARGUMENT;
    }

    memcpy(data_ + field_offset, data, field_len);
    return RC::SUCCESS;
  }

  /**
   * @brief 重置记录中指定字段的值
   * @details 将记录中特定偏移量和长度的字段数据清零
   * @param field_offset 字段在记录中的偏移量
   * @param field_len 字段的长度
   * @return 操作结果，成功返回SUCCESS，没有内存所有权返回INTERNAL，参数无效返回INVALID_ARGUMENT
   */
  RC reset_filed(int field_offset, int field_len)
  {
    if (!owner_) {
      LOG_ERROR("cannot set field when record does not own the memory");
      return RC::INTERNAL;
    }
    if (field_offset + field_len > len_) {
      LOG_ERROR("invalid offset or length. offset=%d, length=%d, total length=%d", field_offset, field_len, len_);
      return RC::INVALID_ARGUMENT;
    }

    memset(data_ + field_offset, 0, field_len);
    return RC::SUCCESS;
  }

  /**
   * @brief 获取记录数据指针
   * @return 指向记录数据的可变指针
   */
  char       *data() { return this->data_; }
  
  /**
   * @brief 获取记录数据的常量指针
   * @return 指向记录数据的常量指针
   */
  const char *data() const { return this->data_; }
  
  /**
   * @brief 获取记录长度
   * @return 记录的长度（字节数）
   */
  int         len() const { return this->len_; }

  /**
   * @brief 设置记录的RID
   * @param rid 记录标识符
   */
  void set_rid(const RID &rid) { this->rid_ = rid; }
  
  /**
   * @brief 设置记录的RID
   * @param page_num 页面编号
   * @param slot_num 槽位编号
   */
  void set_rid(const PageNum page_num, const SlotNum slot_num)
  {
    this->rid_.page_num = page_num;
    this->rid_.slot_num = slot_num;
  }

  /**
   * @brief 获取记录的RID
   * @return 记录标识符的引用
   */
  RID          &rid() { return rid_; }
  
  /**
   * @brief 获取记录RID的常量引用
   * @return 记录标识符的常量引用
   */
  const RID    &rid() const { return rid_; }
  
  /**
   * @brief 设置记录的键
   * @param key 键值字符串
   */
  void          set_key(const string &key) { key_ = key; }
  
  /**
   * @brief 获取记录的键
   * @return 键值字符串的常量引用
   */
  const string &key() const { return key_; }

private:
  /**
   * @brief 记录的唯一标识符
   * @details 包含页面号和槽位号，用于定位记录在存储中的位置
   */
  RID    rid_;
  
  /**
   * @brief 记录的主键
   * @details 用于lsm-tree引擎，在后续版本中可能需要重构Record类以更好地支持这一功能
   */
  string key_;  ///< 记录的主键，用于lsm-tree引擎，需要考虑重构Record
  
  /**
   * @brief 记录的实际数据
   * @details 指向记录存储的内存区域
   */
  char  *data_  = nullptr;
  
  /**
   * @brief 记录的长度
   * @details 记录数据的字节数
   * @note 如果Record不拥有内存，此字段可能无效
   */
  int    len_   = 0;      ///< 如果不是record自己来管理内存，这个字段可能是无效的
  
  /**
   * @brief 内存所有权标志
   * @details 指示当前是否由Record来管理data_指向的内存
   * 如果为true，析构时需要释放内存；如果为false，不负责释放内存
   */
  bool   owner_ = false;  ///< 表示当前是否由record来管理内存
};
