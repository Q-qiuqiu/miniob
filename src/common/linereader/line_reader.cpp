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
// Created by Willaaaaaaa in 2025
//

#include "common/linereader/line_reader.h"
#include "common/lang/string.h"

namespace common {
/**
 * @brief MiniobLineReader的构造函数
 */
MiniobLineReader::MiniobLineReader() : history_file_(""), previous_history_save_time_(0), history_save_interval_(5) {}

/**
 * @brief MiniobLineReader的析构函数
 * @details 在析构时保存历史记录到文件
 */
MiniobLineReader::~MiniobLineReader() { reader_.history_save(history_file_); }

/**
 * @brief 获取MiniobLineReader的单例实例
 * @return MiniobLineReader的单例引用
 */
MiniobLineReader &MiniobLineReader::instance()
{
  static MiniobLineReader instance;
  return instance;
}

/**
 * @brief 初始化行读取器
 * @param[in] history_file 历史文件的路径
 * @details 设置历史文件路径并加载历史记录
 */
void MiniobLineReader::init(const std::string &history_file)
{
  history_file_ = history_file;
  reader_.history_load(history_file_);
}

/**
 * @brief 从输入读取一行
 * @param[in] prompt 显示的提示符
 * @return 用户输入的字符串
 * @details 使用replxx库读取一行输入，支持命令补全和历史记录功能
 */
std::string MiniobLineReader::my_readline(const std::string &prompt)
{
  const char *cinput = nullptr;
  cinput             = reader_.input(prompt);
  if (cinput == nullptr) {
    return "";
  }

  std::string line = cinput;
  cinput           = nullptr;

  if (line.empty()) {
    return "";
  }

  bool is_valid_input = false;
  for (auto c : line) {
    if (!isspace(c)) {
      is_valid_input = true;
      break;
    }
  }

  if (is_valid_input) {
    reader_.history_add(line);
    check_and_save_history();
  }

  return line;
}

/**
 * @brief 检查命令是否为退出命令
 * @param[in] cmd 输入的命令
 * @return 如果是退出命令则返回true
 * @details 支持多种退出命令格式：exit、bye、\q、interrupted
 */
bool MiniobLineReader::is_exit_command(const std::string &cmd)
{
  std::string lower_cmd = cmd;
  common::str_to_lower(lower_cmd);

  bool is_exit = lower_cmd.compare(0, 4, "exit") == 0 || lower_cmd.compare(0, 3, "bye") == 0 ||
                 lower_cmd.compare(0, 2, "\\q") == 0 || lower_cmd.compare(0, 11, "interrupted") == 0;

  return is_exit;
}

/**
 * @brief 检查是否应该自动保存历史记录
 * @return 如果因为时间间隔应该保存历史记录则返回true
 * @details 检查当前时间与上次保存时间的差值，如果超过配置的时间间隔则保存历史记录
 */
bool MiniobLineReader::check_and_save_history()
{
  time_t current_time = time(nullptr);
  if (current_time - previous_history_save_time_ > history_save_interval_) {
    reader_.history_save(history_file_);
    previous_history_save_time_ = current_time;
    return true;
  }
  return false;
}
}  // namespace common
