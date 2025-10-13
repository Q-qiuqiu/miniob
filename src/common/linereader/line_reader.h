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

#ifndef COMMON_LINE_READER_H
#define COMMON_LINE_READER_H

#include "replxx.hxx"

namespace common {
/**
 * @brief 行读取工具类
 * 
 * 这是一个单例类，提供了基于replxx库的行读取功能，支持命令历史记录、命令补全等特性。
 * 主要用于交互式命令行界面中读取用户输入。
 */
class MiniobLineReader
{
  // 以下私有方法确保这是一个单例模式
private:
  MiniobLineReader();
  ~MiniobLineReader();
  MiniobLineReader(const MiniobLineReader &)            = delete;  ///< 禁止拷贝构造
  MiniobLineReader &operator=(const MiniobLineReader &) = delete;  ///< 禁止赋值操作符

public:
  /**
   * @brief 获取单例实例
   * @return 单例实例的引用
   */
  static MiniobLineReader &instance();

  /**
   * @brief 使用历史文件初始化行读取器
   * @param[in] history_file 历史文件的路径
   * @details 初始化时会加载历史命令记录
   */
  void init(const std::string &history_file);

  /**
   * @brief 从输入读取一行
   * @param[in] prompt 显示的提示符
   * @return 用户输入的字符串
   * @details 支持命令补全和历史记录功能
   */
  std::string my_readline(const std::string &prompt);

  /**
   * @brief 检查命令是否为退出命令
   * @param[in] cmd 输入的命令
   * @return 如果是退出命令则返回true
   * @details 支持多种退出命令格式，如exit、bye、\q等
   */
  bool is_exit_command(const std::string &cmd);

private:
  /**
   * @brief 检查是否应该自动保存历史记录
   * @return 如果因为时间间隔应该保存历史记录则返回true
   * @details 根据配置的时间间隔自动保存历史记录
   */
  bool check_and_save_history();

private:
  replxx::Replxx reader_;                ///< replxx库的实例，提供行编辑功能
  std::string    history_file_;          ///< 历史文件的路径
  time_t         previous_history_save_time_;  ///< 上一次保存历史记录的时间
  int            history_save_interval_;  ///< 历史记录自动保存的时间间隔（秒）
};
}  // namespace common

#endif  // COMMON_LINE_READER_H
