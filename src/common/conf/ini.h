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
// Created by Longda on 2010
//

#if !defined(__COMMON_CONF_INI_H__)
#define __COMMON_CONF_INI_H__

#include <stdio.h>

#include <iostream>

#include "common/lang/map.h"
#include "common/lang/set.h"
#include "common/lang/string.h"

namespace common {

/**
 * @brief INI配置文件解析模块
 * 
 * 该模块提供了对INI格式配置文件的解析、读取和修改功能。
 * INI文件格式遵循以下规则：
 * [section]
 * VARNAME=VALUE
 * 
 * 支持的特性：
 * - 注释行（以#开头）
 * - 多行配置（以\结尾）
 * - 多个配置文件的加载
 */
class Ini
{
public:
  /**
   * 为简化逻辑，加载配置时不加锁，因此不要并行修改数据
   */
  Ini();
  ~Ini();

  /**
   * @brief 加载一个INI配置文件
   * @param[in] ini_file 要加载的INI文件路径
   * @return 成功返回0，失败返回非0值
   * @details 支持加载多个INI配置文件，配置会被合并
   */
  int load(const string &ini_file);

  /**
   * @brief 获取指定section的所有键值对
   * @param[in] section 配置段名称，默认为空段
   * @return 键值对映射，如果section不存在，返回空映射
   */
  const map<string, string> &get(const string &section = DEFAULT_SECTION);

  /**
   * @brief 获取指定section中指定键的值
   * @param[in] key 要获取的配置键名
   * @param[in] default_value 键不存在时返回的默认值
   * @param[in] section 配置段名称，默认为空段
   * @return 配置值，如果键不存在则返回默认值
   */
  string get(const string &key, const string &default_value, const string &section = DEFAULT_SECTION);

  /**
   * @brief 向指定section中添加或修改键值对
   * @param[in] key 配置键名
   * @param[in] value 配置值
   * @param[in] section 配置段名称，默认为空段
   * @return 成功返回0，失败返回非0值
   * @details 如果键已存在则替换值，如果section不存在则创建
   */
  int put(const string &key, const string &value, const string &section = DEFAULT_SECTION);

  /**
   * @brief 将所有配置输出到字符串中
   * @param[out] output_str 输出参数，用于存储配置内容
   */
  void to_string(string &output_str);

  static const string DEFAULT_SECTION;  ///< 默认配置段名称

  static const int MAX_CFG_LINE_LEN = 1024;       ///< 单行最大长度
  static const char CFG_DELIMIT_TAG = ',';        ///< 值分隔标记
  static const char CFG_COMMENT_TAG = '#';        ///< 注释标记
  static const char CFG_CONTINUE_TAG = '\\';      ///< 行继续标记
  static const char CFG_SESSION_START_TAG = '[';  ///< 配置段开始标记
  static const char CFG_SESSION_END_TAG = ']';    ///< 配置段结束标记

protected:
  /**
   * @brief 向sections_中插入一个空的配置段
   * @param[in] session_name 配置段名称
   */
  void insert_session(const string &session_name);

  /**
   * @brief 切换到指定的配置段
   * @param[in] session_name 配置段名称
   * @return 指向配置段的指针，如果操作失败返回nullptr
   * @details 如果配置段不存在，则创建一个新的
   */
  map<string, string> *switch_session(const string &session_name);

  /**
   * @brief 向配置段中插入一个条目
   * @param[in] session_map 配置段映射
   * @param[in] line 配置行，格式为"key=value"
   * @return 成功返回0，失败返回非0值
   */
  int insert_entry(map<string, string> *session_map, const string &line);

  typedef map<string, map<string, string>> SessionsMap;  ///< 配置段映射类型定义

private:
  static const map<string, string> empty_map_;  ///< 空映射，用于不存在的配置段

  set<string> file_names_;  ///< 已加载的文件列表
  SessionsMap sections_;    ///< 所有配置段的映射
};

/**
 * @brief 获取全局配置对象
 * @return 全局配置对象的引用
 */
Ini *&get_properties();

}  // namespace common
#endif  //__COMMON_CONF_INI_H__
