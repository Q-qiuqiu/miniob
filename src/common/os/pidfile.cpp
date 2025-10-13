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

#include <assert.h>
#include <errno.h>
#include <fstream>
#include <libgen.h>
#include <paths.h>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include "common/log/log.h"
#include "common/os/pidfile.h"
#include "common/lang/iostream.h"
#include "common/lang/fstream.h"

/**
 * @file pidfile.cpp
 * @brief PID文件管理接口的实现
 * 
 * 该文件实现了pidfile.h中定义的PID文件管理接口，包括创建、删除和获取PID文件路径等功能。
 * PID文件用于记录进程的ID号，便于系统管理和监控进程状态，特别是确保同一时间只有一个程序实例在运行。
 */
namespace common {

/**
 * @brief 获取当前PID文件的路径实现
 * 
 * 该函数返回一个指向内部静态字符串的引用，该字符串存储了当前进程PID文件的路径。
 * 如果PID文件尚未创建，该字符串为空。
 * 
 * @return string& PID文件路径的字符串引用
 */
string &getPidPath()
{
  // 使用静态变量存储PID文件路径，确保在程序运行期间保持一致
  static string path;

  return path;
}

/**
 * @brief 设置PID文件路径实现
 * 
 * 该函数根据提供的程序名称设置PID文件的路径。PID文件将被创建在系统的临时目录下，
 * 文件名格式为"程序名.pid"。如果progName为NULL，则清空PID文件路径。
 * 
 * @param[in] progName 程序名称，用于构造PID文件名
 */
void setPidPath(const char *progName)
{
  // 获取内部存储的PID文件路径
  string &path = getPidPath();

  if (progName != NULL) {
    // 构造PID文件路径：系统临时目录 + 程序名 + .pid
    // _PATH_TMP 是系统临时目录的路径，通常是 /tmp/，可能为 POSIX 系统中的标准路径
    path = string(_PATH_TMP) + progName + ".pid";
  } else {
    // 如果progName为NULL，清空PID文件路径
    path = "";
  }
}

/**
 * @brief 为当前进程创建PID文件实现
 * 
 * 该函数首先检查程序名是否有效，然后设置PID文件路径，创建文件并写入当前进程的PID。
 * 如果文件创建成功，返回0；否则返回系统错误码。
 * 
 * @param[in] progName 程序名称，用于构造PID文件名
 * @return int 成功时返回0，失败时返回错误码
 */
int writePidFile(const char *progName)
{
  // 确保程序名不为空
  assert(progName);
  // 创建输出文件流用于写入PID文件
  ofstream ostr;
  // 默认返回错误
  int rv = 1;

  // 设置PID文件路径
  setPidPath(progName);
  // 获取设置的PID文件路径
  string path = getPidPath();
  // 打开PID文件，如果文件已存在则清空内容
  ostr.open(path.c_str(), ios::trunc);
  // 检查文件是否成功打开
  if (ostr.good()) {
    // 写入当前进程的PID到文件中
    ostr << getpid() << endl;
    // 关闭文件
    ostr.close();
    // 设置返回值为成功
    rv = 0;
  } else {
    // 获取系统错误码
    rv = errno;
    // 输出错误信息到标准错误流
    cerr << "error opening PID file " << path.c_str() << SYS_OUTPUT_ERROR << endl;
  }

  return rv;
}

/**
 * @brief 删除当前进程的PID文件实现
 * 
 * 该函数检查PID文件路径是否非空，如果非空则删除对应的文件并清空PID文件路径。
 * 通常在程序正常退出时调用此函数清理资源。
 */
void removePidFile(void)
{
  // 获取PID文件路径
  string path = getPidPath();
  // 检查路径是否非空
  if (!path.empty()) {
    // 调用系统函数删除文件
    unlink(path.c_str());
    // 清空PID文件路径
    setPidPath(NULL);
  }
  return;
}

}  // namespace common