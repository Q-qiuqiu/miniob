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
#include <dirent.h>
#include <regex.h>
#include <string.h>
#include <sys/stat.h>

#include "common/defs.h"
#include "common/log/log.h"
#include "common/os/path.h"
#include "common/lang/string.h"
#include "common/lang/vector.h"

/**
 * @file path.cpp
 * @brief 文件路径操作相关接口的实现
 * 
 * 该文件实现了path.h中定义的一系列文件路径操作函数，包括获取文件名、目录名、
 * 绝对路径、判断是否为目录、检查并创建目录、列出目录中的文件等功能。
 * 这些函数为系统提供了统一的路径处理能力，便于跨平台使用。
 */
namespace common {

/**
 * @brief 从完整路径中获取文件名实现
 * 
 * 该函数通过查找路径中最后一个目录分隔符的位置来提取文件名。
 * 如果路径以目录分隔符结尾，则返回空字符串；如果路径中没有目录分隔符，
 * 则返回整个路径作为文件名。
 * 
 * @param[in] fullPath 完整的文件路径
 * @return string 提取出的文件名
 */
string getFileName(const string &fullPath)
{
  string szRt;
  size_t pos;
  try {
    // 查找最后一个目录分隔符的位置
    pos = fullPath.rfind(FILE_PATH_SPLIT);
    if (pos != string::npos && pos < fullPath.size() - 1) {
      // 路径中包含目录分隔符且不是最后一个字符，提取分隔符后面的部分
      szRt = fullPath.substr(pos + 1, fullPath.size() - pos - 1);
    } else if (pos == string::npos) {
      // 路径中不包含目录分隔符，返回整个路径
      szRt = fullPath;
    } else {
      // 路径以目录分隔符结尾，返回空字符串
      szRt = "";
    }

  } catch (...) {
    // 捕获所有异常，确保函数不会异常退出
  }
  return szRt;
}

/**
 * @brief 从路径中获取文件名实现（C风格字符串版本）
 * 
 * 该函数是getFileName的C风格字符串版本，使用strrchr函数查找最后一个目录分隔符，
 * 并将提取出的文件名存储在传入的引用参数中。
 * 
 * @param[in] path 完整的文件路径（C风格字符串）
 * @param[out] fileName 存储提取出的文件名的字符串引用
 */
void getFileName(const char *path, string &fileName)
{
  // 查找最后一个目录分隔符的位置
  const char *endPos = strrchr(path, FILE_PATH_SPLIT);
  if (endPos == NULL) {
    // 路径中不包含目录分隔符，返回整个路径
    fileName = path;
    return;
  }

  if (strcmp(path, FILE_PATH_SPLIT_STR) == 0) {
    // 路径就是根目录，返回空字符串
    fileName.assign("");
  } else {
    // 提取目录分隔符后面的部分作为文件名
    fileName.assign(endPos + 1);
  }

  return;
}

/**
 * @brief 从完整路径中获取目录名实现
 * 
 * 该函数通过查找路径中最后一个目录分隔符的位置来提取目录名。
 * 如果路径中没有目录分隔符，则返回整个路径；如果路径是根目录（以/开头），
 * 则返回根目录。
 * 
 * @param[in] fullPath 完整的文件路径
 * @return string 提取出的目录名
 */
string getDirName(const string &fullPath)
{
  string szRt;
  size_t pos;
  try {
    // 查找最后一个目录分隔符的位置
    pos = fullPath.rfind(FILE_PATH_SPLIT);
    if (pos != string::npos && pos > 0) {
      // 路径中包含目录分隔符且不是第一个字符，提取分隔符前面的部分
      szRt = fullPath.substr(0, pos);
    } else if (pos == string::npos) {
      // 路径中不包含目录分隔符，返回整个路径
      szRt = fullPath;
    } else {
      // 路径以目录分隔符开头（根目录），返回根目录
      szRt = FILE_PATH_SPLIT_STR;
    }

  } catch (...) {
    // 捕获所有异常，确保函数不会异常退出
  }
  return szRt;
}

/**
 * @brief 从路径中获取父目录路径实现（C风格字符串版本）
 * 
 * 该函数使用strrchr函数查找最后一个目录分隔符，并将提取出的父目录路径
 * 存储在传入的引用参数中。
 * 
 * @param[in] path 完整的文件路径（C风格字符串）
 * @param[out] parent 存储提取出的父目录路径的字符串引用
 */
void getDirName(const char *path, string &parent)
{
  // 查找最后一个目录分隔符的位置
  const char *endPos = strrchr(path, FILE_PATH_SPLIT);
  if (endPos == NULL) {
    // 路径中不包含目录分隔符，返回整个路径
    parent = path;
    return;
  }

  if (endPos == path) {
    // 路径就是根目录，返回根目录
    parent.assign(path, 1);
  } else {
    // 提取目录分隔符前面的部分作为父目录路径
    parent.assign(path, endPos - path);
  }

  return;
}

/**
 * @brief 从完整路径中获取文件路径实现
 * 
 * 该函数通过查找路径中最后一个斜杠的位置来提取文件路径部分。
 * 如果路径中不包含斜杠，则返回整个路径。
 * 
 * @param[in] fullPath 完整的文件路径
 * @return string 提取出的文件路径
 */
string getFilePath(const string &fullPath)
{
  string szRt;
  size_t pos;
  try {
    // 查找最后一个斜杠的位置
    pos = fullPath.rfind("/");
    if (pos != string::npos) {
      // 路径中包含斜杠，提取斜杠前面的部分
      szRt = fullPath.substr(0, pos);
    } else if (pos == string::npos) {
      // 路径中不包含斜杠，返回整个路径
      szRt = fullPath;
    } else {
      // 其他情况返回空字符串
      szRt = "";
    }

  } catch (...) {
    // 捕获所有异常，确保函数不会异常退出
  }
  return szRt;
}

/**
 * @brief 获取路径的绝对路径形式实现
 * 
 * 该函数尝试将相对路径转换为绝对路径。如果路径不是以/开头（相对路径），
 * 则尝试获取当前工作目录，并将其与相对路径合并。
 * 
 * @param[in] path 输入路径（可以是相对路径或绝对路径）
 * @return string 转换后的绝对路径
 */
string getAboslutPath(const char *path)
{
  string aPath(path);
  if (path[0] != '/') {
    const int MAX_SIZE = 256;
    char current_absolute_path[MAX_SIZE];

    // 尝试获取当前工作目录
    if (NULL == getcwd(current_absolute_path, MAX_SIZE)) {
      // 获取失败，保持原路径不变
    } else {
      // 合并当前工作目录和相对路径
      aPath = std::string(current_absolute_path) + "/" + aPath;
    }
  }

  return aPath;
}

/**
 * @brief 判断给定路径是否为目录实现
 * 
 * 该函数使用stat系统调用获取路径的状态信息，并检查其是否为目录。
 * 
 * @param[in] path 要检查的路径
 * @return bool 如果是目录则返回true，否则返回false
 */
bool is_directory(const char *path)
{
  struct stat st;
  // 调用stat获取路径状态，并检查是否为目录
  return (0 == stat(path, &st)) && (st.st_mode & S_IFDIR);
}

/**
 * @brief 检查目录是否存在，如果不存在则逐级创建实现
 * 
 * 该函数首先移除路径末尾的所有斜杠，然后检查目录是否存在。
 * 如果目录不存在，则从根目录开始逐级创建所需的所有父目录。
 * 
 * @param[in,out] path 要检查或创建的目录路径，会被修改为标准化的路径（去除末尾斜杠）
 * @return bool 目录存在或创建成功返回true，否则返回false
 */
bool check_directory(string &path)
{
  // 移除路径末尾的所有斜杠
  while (!path.empty() && path.back() == '/')
    path.erase(path.size() - 1, 1);

  int len = path.size();

  // 检查目录是否已存在，或者尝试直接创建整个目录
  if (0 == mkdir(path.c_str(), 0777) || is_directory(path.c_str()))
    return true;

  bool sep_state = false;
  // 逐级创建目录
  for (int i = 0; i < len; i++) {
    if (path[i] != '/') {
      if (sep_state)
        sep_state = false;
      continue;
    }

    if (sep_state)
      continue;

    // 临时将当前斜杠替换为字符串结束符，以创建当前级别的目录
    path[i] = '\0';
    if (0 != mkdir(path.c_str(), 0777) && !is_directory(path.c_str()))
      return false;

    // 恢复斜杠，并标记已处理过一个分隔符
    path[i]   = '/';
    sep_state = true;
  }

  // 尝试创建最终的目录
  if (0 != mkdir(path.c_str(), 0777) && !is_directory(path.c_str()))
    return false;
  return true;
}

/**
 * @brief 列出指定目录下符合正则表达式模式的所有文件实现
 * 
 * 该函数扫描指定目录，返回所有文件名匹配给定正则表达式模式的文件列表。
 * 注意：此函数不会递归到子目录中，只会列出当前目录下的文件，并且会跳过
 * 以点开头的隐藏文件。
 * 
 * @param[in] path 要扫描的目录路径
 * @param[in] filter_pattern 用于过滤文件名的正则表达式模式，如果为nullptr则不过滤
 * @param[out] files 存储匹配到的文件名列表的vector引用
 * @return int 成功时返回匹配到的文件数量，失败时返回-1
 */
int list_file(const char *path, const char *filter_pattern, vector<string> &files)
{
  regex_t reg;
  // 如果提供了过滤模式，编译正则表达式
  if (filter_pattern) {
    const int res = regcomp(&reg, filter_pattern, REG_NOSUB);
    if (res) {
      // 正则表达式编译失败，记录错误日志
      char errbuf[256];
      regerror(res, &reg, errbuf, sizeof(errbuf));
      LOG_ERROR("regcomp return error. filter pattern %s. errmsg %d:%s", filter_pattern, res, errbuf);
      return -1;
    }
  }

  // 打开目录
  DIR *pdir = opendir(path);
  if (!pdir) {
    // 目录打开失败，记录错误日志
    if (filter_pattern)
      regfree(&reg);  // 如果已编译正则表达式，需要释放资源
    LOG_ERROR("open directory failure. path %s, errmsg %d:%s", path, errno, strerror(errno));
    return -1;
  }

  // 清空结果列表
  files.clear();

  // 注意：readdir_r在某些系统中已被废弃，所以这里使用readdir
  // 由于readdir不是线程安全的，未来应该考虑使用C++的目录遍历方式
  // TODO: 使用C++17的filesystem库替代
  struct dirent *pentry;
  char tmp_path[PATH_MAX];
  // 遍历目录中的所有条目
  while ((pentry = readdir(pdir)) != NULL) {
    // 跳过以点开头的文件（.、..和隐藏文件）
    if ('.' == pentry->d_name[0])
      continue;

    // 构建完整的文件路径
    snprintf(tmp_path, sizeof(tmp_path), "%s/%s", path, pentry->d_name);
    // 跳过子目录
    if (is_directory(tmp_path))
      continue;

    // 检查文件名是否匹配过滤模式
    if (!filter_pattern || 0 == regexec(&reg, pentry->d_name, 0, NULL, 0))
      files.push_back(pentry->d_name);
  }

  // 释放正则表达式资源
  if (filter_pattern)
    regfree(&reg);

  // 关闭目录
  closedir(pdir);
  // 返回匹配到的文件数量
  return files.size();
}

}  // namespace common
