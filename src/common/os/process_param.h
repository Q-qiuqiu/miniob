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

#pragma once

#include "common/lang/string.h"
#include "common/lang/vector.h"

namespace common {

/**
 * 进程参数配置类
 * 用于存储和管理进程运行所需的各种配置参数
 */
class ProcessParam
{

public:
  /**
   * 默认构造函数
   */
  ProcessParam() {}

  /**
   * 虚析构函数
   * 确保派生类的析构函数能够被正确调用
   */
  virtual ~ProcessParam() {}

  /**
   * 初始化进程参数的默认值
   * @param process_name 进程名称
   */
  void init_default(string &process_name);

  /**
   * 获取标准输出文件路径
   * @return 标准输出文件路径
   */
  const string &get_std_out() const { return std_out_; }

  /**
   * 设置标准输出文件路径
   * @param std_out 标准输出文件路径
   */
  void set_std_out(const string &std_out) { ProcessParam::std_out_ = std_out; }

  /**
   * 获取标准错误输出文件路径
   * @return 标准错误输出文件路径
   */
  const string &get_std_err() const { return std_err_; }

  /**
   * 设置标准错误输出文件路径
   * @param std_err 标准错误输出文件路径
   */
  void set_std_err(const string &std_err) { ProcessParam::std_err_ = std_err; }

  /**
   * 获取配置文件路径
   * @return 配置文件路径
   */
  const string &get_conf() const { return conf; }

  /**
   * 设置配置文件路径
   * @param conf 配置文件路径
   */
  void set_conf(const string &conf) { ProcessParam::conf = conf; }

  /**
   * 获取进程名称
   * @return 进程名称
   */
  const string &get_process_name() const { return process_name_; }

  /**
   * 设置进程名称
   * @param processName 进程名称
   */
  void set_process_name(const string &processName) { ProcessParam::process_name_ = processName; }

  /**
   * 检查进程是否以守护进程方式运行
   * @return 是否以守护进程方式运行
   */
  bool is_demon() const { return demon; }

  /**
   * 设置进程是否以守护进程方式运行
   * @param demon 是否以守护进程方式运行
   */
  void set_demon(bool demon) { ProcessParam::demon = demon; }

  /**
   * 获取进程参数列表
   * @return 进程参数列表
   */
  const vector<string> &get_args() const { return args; }

  /**
   * 设置进程参数列表
   * @param args 进程参数列表
   */
  void set_args(const vector<string> &args) { ProcessParam::args = args; }

  /**
   * 设置服务器端口
   * @param port 服务器端口号
   */
  void set_server_port(int port) { server_port_ = port; }

  /**
   * 获取服务器端口
   * @return 服务器端口号
   */
  int get_server_port() const { return server_port_; }

  /**
   * 设置Unix socket路径
   * @param unix_socket_path Unix socket路径
   */
  void set_unix_socket_path(const char *unix_socket_path) { unix_socket_path_ = unix_socket_path; }

  /**
   * 获取Unix socket路径
   * @return Unix socket路径
   */
  const string &get_unix_socket_path() const { return unix_socket_path_; }

  /**
   * 设置通信协议
   * @param protocol 通信协议名称
   */
  void set_protocol(const char *protocol) { protocol_ = protocol; }

  /**
   * 获取通信协议
   * @return 通信协议名称
   */
  const string &get_protocol() const { return protocol_; }

  /**
   * 设置事务工具包名称
   * @param kit_name 事务工具包名称
   */
  void set_trx_kit_name(const char *kit_name)
  {
    if (kit_name) {
      trx_kit_name_ = kit_name;
    }
  }

  /**
   * 获取事务工具包名称
   * @return 事务工具包名称
   */
  const string &trx_kit_name() const { return trx_kit_name_; }

  /**
   * 设置存储引擎类型
   * @param storage_engine 存储引擎类型名称
   */
  void set_storage_engine(const char *storage_engine)
  {
    if (storage_engine) {
      storage_engine_ = storage_engine;
    }
  }

  /**
   * 获取存储引擎类型
   * @return 存储引擎类型名称
   */
  const string &storage_engine() const { return storage_engine_; }

  /**
   * 设置线程处理模式名称
   * @param thread_handling_name 线程处理模式名称
   */
  void set_thread_handling_name(const char *thread_handling_name)
  {
    if (thread_handling_name) {
      thread_handling_name_ = thread_handling_name;
    }
  }

  /**
   * 获取线程处理模式名称
   * @return 线程处理模式名称
   */
  const string &thread_handling_name() const { return thread_handling_name_; }

  /**
   * 设置缓冲池内存大小
   * @param bytes 缓冲池内存大小（字节）
   */
  void set_buffer_pool_memory_size(int bytes) { buffer_pool_memory_size_ = bytes; }

  /**
   * 获取缓冲池内存大小
   * @return 缓冲池内存大小（字节）
   */
  int buffer_pool_memory_size() const { return buffer_pool_memory_size_; }

  /**
   * 设置持久性模式
   * @param mode 持久性模式名称
   */
  void          set_durability_mode(const char *mode) { durability_mode_ = mode; }
  /**
   * 获取持久性模式
   * @return 持久性模式名称
   */
  const string &durability_mode() const { return durability_mode_; }

private:
  string         std_out_;           ///< 标准输出文件路径
  string         std_err_;           ///< 标准错误输出文件路径
  string         conf;               ///< 配置文件路径
  string         process_name_;      ///< 进程名称
  bool           demon = false;      ///< 是否以守护进程方式运行
  vector<string> args;               ///< 进程参数列表
  int            server_port_ = -1;  ///< 服务器端口号（如果有效，将覆盖配置文件中的端口）
  string         unix_socket_path_;  ///< Unix socket路径
  string         protocol_;          ///< 通信协议
  string         trx_kit_name_;      ///< 事务工具包名称
  string         storage_engine_;    ///< 存储引擎类型
  string         thread_handling_name_; ///< 线程处理模式名称
  int            buffer_pool_memory_size_ = -1; ///< 缓冲池内存大小（字节）
  string         durability_mode_;   ///< 持久性模式
};

/**
 * 获取全局进程参数对象的引用
 * @return 全局进程参数对象的引用
 */
ProcessParam *&the_process_param();

}  // namespace common
