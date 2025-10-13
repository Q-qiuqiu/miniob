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

#include <errno.h>
#include <string.h>

#include <fstream>

#include "common/conf/ini.h"
#include "common/defs.h"
#include "common/lang/iostream.h"
#include "common/lang/string.h"
#include "common/lang/utility.h"
#include "common/lang/fstream.h"
#include "common/log/log.h"

namespace common {

const string              Ini::DEFAULT_SECTION = string("");
const map<string, string> Ini::empty_map_;

/**
 * @brief 构造函数，初始化Ini对象
 */
Ini::Ini() {}

/**
 * @brief 析构函数，释放Ini对象资源
 */
Ini::~Ini() {}

/**
 * @brief 向配置中插入一个空的配置段
 * @param[in] session_name 要插入的配置段名称
 */
void Ini::insert_session(const string &session_name)
{
  map<string, string>               session_map;
  pair<string, map<string, string>> entry = pair<string, map<string, string>>(session_name, session_map);

  sections_.insert(entry);
}

/**
 * @brief 切换到指定的配置段
 * @param[in] session_name 要切换到的配置段名称
 * @return 指向配置段的指针，如果操作失败返回nullptr
 * @details 如果指定的配置段不存在，则创建一个新的配置段
 */
map<string, string> *Ini::switch_session(const string &session_name)
{
  SessionsMap::iterator it = sections_.find(session_name);
  if (it != sections_.end()) {
    return &it->second;
  }

  insert_session(session_name);

  it = sections_.find(session_name);
  if (it != sections_.end()) {
    return &it->second;
  }

  // 正常情况下不应该走到这里
  return nullptr;
}

/**
 * @brief 获取指定配置段的所有键值对
 * @param[in] section 配置段名称
 * @return 配置段的键值对映射，如果配置段不存在返回空映射
 */
const map<string, string> &Ini::get(const string &section)
{
  SessionsMap::iterator it = sections_.find(section);
  if (it == sections_.end()) {
    return empty_map_;
  }

  return it->second;
}

/**
 * @brief 获取指定配置段中指定键的值
 * @param[in] key 配置键名
 * @param[in] defaultValue 键不存在时的默认值
 * @param[in] section 配置段名称
 * @return 配置值，如果键不存在则返回默认值
 */
string Ini::get(const string &key, const string &defaultValue, const string &section)
{
  map<string, string> section_map = get(section);

  map<string, string>::iterator it = section_map.find(key);
  if (it == section_map.end()) {
    return defaultValue;
  }

  return it->second;
}

/**
 * @brief 向指定配置段中添加或修改键值对
 * @param[in] key 配置键名
 * @param[in] value 配置值
 * @param[in] section 配置段名称
 * @return 成功返回0，失败返回非0值
 */
int Ini::put(const string &key, const string &value, const string &section)
{
  map<string, string> *section_map = switch_session(section);

  section_map->insert(pair<string, string>(key, value));

  return 0;
}

/**
 * @brief 向配置段中插入一个配置项
 * @param[in] session_map 配置段映射
 * @param[in] line 配置行，格式为"key=value"
 * @return 成功返回0，失败返回非0值
 * @details 解析配置行，提取键值对并添加到配置段中
 */
int Ini::insert_entry(map<string, string> *session_map, const string &line)
{
  if (session_map == nullptr) {
    cerr << __FILE__ << __FUNCTION__ << " session map is null" << endl;
    return -1;
  }
  size_t equal_pos = line.find_first_of('=');
  if (equal_pos == string::npos) {
    cerr << __FILE__ << __FUNCTION__ << "Invalid configuration line " << line << endl;
    return -1;
  }

  string key   = line.substr(0, equal_pos);
  string value = line.substr(equal_pos + 1);

  strip(key);
  strip(value);

  session_map->insert(pair<string, string>(key, value));

  return 0;
}

/**
 * @brief 加载INI配置文件
 * @param[in] file_name 要加载的配置文件路径
 * @return 成功返回0，失败返回非0值
 * @details 解析配置文件，支持注释行、多行配置和配置段
 */
int Ini::load(const string &file_name)
{
  ifstream ifs;

  try {
    // 多行配置的处理标志
    bool continue_last_line = false;

    // 初始化为默认配置段
    map<string, string> *current_session = switch_session(DEFAULT_SECTION);

    char line[MAX_CFG_LINE_LEN];
    string line_entry;

    ifs.open(file_name.c_str());
    while (ifs.good()) {
      memset(line, 0, sizeof(line));
      ifs.getline(line, sizeof(line));

      // 去除首尾空白字符
      char *read_buf = strip(line);

      // 忽略空行
      if (strlen(read_buf) == 0) {
        continue;
      }

      // 忽略注释行
      if (read_buf[0] == CFG_COMMENT_TAG) {
        continue;
      }

      // 处理配置段
      if (read_buf[0] == CFG_SESSION_START_TAG && read_buf[strlen(read_buf) - 1] == CFG_SESSION_END_TAG) {
        read_buf[strlen(read_buf) - 1] = '\0';
        string session_name            = string(read_buf + 1);

        current_session = switch_session(session_name);

        continue;
      }

      // 处理多行配置
      if (continue_last_line == false) {
        // 不需要延续上一行
        line_entry = read_buf;
      } else {
        // 需要延续上一行
        line_entry += read_buf;
      }

      // 检查是否需要延续到下一行
      if (read_buf[strlen(read_buf) - 1] == CFG_CONTINUE_TAG) {
        // 此行未结束，需要延续
        continue_last_line = true;

        // 移除行尾的延续标记
        line_entry = line_entry.substr(0, line_entry.size() - 1);
        continue;
      } else {
        // 此行结束，处理配置项
        continue_last_line = false;
        insert_entry(current_session, line_entry);
      }
    }
    ifs.close();

    // 记录已加载的文件
    file_names_.insert(file_name);
    cout << "Successfully load " << file_name << endl;
  } catch (...) {
    // 异常处理
    if (ifs.is_open()) {
      ifs.close();
    }
    cerr << "Failed to load " << file_name << SYS_OUTPUT_ERROR << endl;
    return -1;
  }

  return 0;
}

/**
 * @brief 将所有配置输出到字符串中
 * @param[out] output_str 输出参数，用于存储配置内容
 * @details 按格式输出所有配置段和配置项
 */
void Ini::to_string(string &output_str)
{
  output_str.clear();

  output_str += "Begin dump configuration\n";

  // 遍历所有配置段
  for (SessionsMap::iterator it = sections_.begin(); it != sections_.end(); it++) {
    output_str += CFG_SESSION_START_TAG;
    output_str += it->first;
    output_str += CFG_SESSION_END_TAG;
    output_str += "\n";

    // 遍历配置段中的所有键值对
    map<string, string> &section_map = it->second;
    for (map<string, string>::iterator sub_it = section_map.begin(); sub_it != section_map.end(); sub_it++) {
      output_str += sub_it->first;
      output_str += "=";
      output_str += sub_it->second;
      output_str += "\n";
    }
    output_str += "\n";
  }

  output_str += "Finish dump configuration \n";

  return;
}

/**
 * @brief 获取全局配置对象
 * @return 全局配置对象的引用
 * @details 提供全局单例访问点，用于访问配置信息
 */
Ini *&get_properties()
{
  static Ini *properties = new Ini();
  return properties;
}

}  // namespace common
