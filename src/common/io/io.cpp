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
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common/io/io.h"
#include "common/lang/string.h"
#include "common/log/log.h"
#include "common/math/regex.h"

namespace common {

/**
 * @brief 从文件中读取数据的实现
 * @param[in] fileName 要读取的文件名
 * @param[out] outputData 输出参数，用于存储读取到的数据
 * @param[out] fileSize 输出参数，用于存储文件大小
 * @return 成功返回0，失败返回-1
 * @details 通过分块读取的方式读取文件内容，使用realloc动态调整内存大小，
 *          读取完成后会在数据末尾添加\0字符，方便作为字符串处理
 */
int readFromFile(const string &fileName, char *&outputData, size_t &fileSize)
{
  FILE *file = fopen(fileName.c_str(), "rb");
  if (file == NULL) {
    cerr << "Failed to open file " << fileName << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
    return -1;
  }

  // fseek( file, 0, SEEK_END );
  // size_t fsSize = ftell( file );
  // fseek( file, 0, SEEK_SET );
  // 定义缓冲区和读取相关变量
  char   buffer[4 * ONE_KILO];  // 4KB的读取缓冲区
  size_t readSize = 0;          // 已读取的数据大小
  size_t oneRead  = 0;          // 单次读取的数据大小

  char *data = NULL;
  do {
    memset(buffer, 0, sizeof(buffer));
    oneRead = fread(buffer, 1, sizeof(buffer), file);
    if (ferror(file)) {
      cerr << "Failed to read data" << fileName << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      fclose(file);
      if (data != NULL) {
        free(data);
        data = NULL;
      }
      return -1;
    }

    // 动态调整内存大小
    data = (char *)realloc(data, readSize + oneRead);
    if (data == NULL) {
      cerr << "Failed to alloc memory for " << fileName << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      free(data);
      fclose(file);
      return -1;
    } else {
      // 复制新读取的数据到已分配的内存
      memcpy(data + readSize, buffer, oneRead);
      readSize += oneRead;
    }

  } while (feof(file) == 0);

  fclose(file);

  // 为字符串添加结束符
  data           = (char *)realloc(data, readSize + 1);
  data[readSize] = '\0';
  outputData     = data;
  fileSize       = readSize;
  return 0;
}

/**
 * @brief 将数据写入文件的实现
 * @param[in] fileName 要写入的文件名
 * @param[in] data 要写入的数据
 * @param[in] dataSize 要写入的数据大小
 * @param[in] openMode 文件打开模式
 * @return 成功返回0，失败返回-1
 * @details 通过循环写入的方式确保所有数据都被写入文件，处理写入不完整的情况
 */
int writeToFile(const string &fileName, const char *data, uint32_t dataSize, const char *openMode)
{
  FILE *file = fopen(fileName.c_str(), openMode);
  if (file == NULL) {
    cerr << "Failed to open file " << fileName << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
    return -1;
  }

  uint32_t    leftSize = dataSize;
  const char *buffer   = data;
  while (leftSize > 0) {
    int writeCount = fwrite(buffer, 1, leftSize, file);
    if (writeCount <= 0) {
      cerr << "Failed to open file " << fileName << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      fclose(file);
      return -1;
    } else {
      leftSize -= writeCount;
      buffer += writeCount;
    }
  }

  fclose(file);

  return 0;
}

/**
 * @brief 获取文件中非空行数量的实现
 * @param[in] fileName 要统计的文件名
 * @param[out] lineNum 输出参数，用于存储非空行数量
 * @return 成功返回0，失败返回-1
 * @details 使用ifstream逐行读取文件，对每一行进行strip处理后判断是否为空
 */
int getFileLines(const string &fileName, uint64_t &lineNum)
{
  lineNum = 0;

  char line[4 * ONE_KILO] = {0};

  ifstream ifs(fileName.c_str());
  if (!ifs) {
    return -1;
  }

  while (ifs.good()) {
    line[0] = 0;
    ifs.getline(line, sizeof(line));
    char *lineStrip = strip(line);
    if (strlen(lineStrip)) {
      lineNum++;
    }
  }

  ifs.close();
  return 0;
}

/**
 * @brief 获取目录下符合条件的文件数量的实现
 * @param[out] fileNum 输出参数，用于存储文件数量
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式
 * @param[in] recursive 是否递归搜索子目录
 * @return 成功返回0，失败返回-1
 * @details 遍历目录，统计符合条件的普通文件数量，支持递归搜索子目录
 */
int getFileNum(int64_t &fileNum, const string &path, const string &pattern, bool recursive)
{
  try {
    DIR *dirp = NULL;
    dirp      = opendir(path.c_str());
    if (dirp == NULL) {
      cerr << "Failed to opendir " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      return -1;
    }

    string    fullPath;
    struct dirent *entry = NULL;
    struct stat    fs;
    while ((entry = readdir(dirp)) != NULL) {
      // 跳过"."、".."和以"."开头的隐藏文件
      if (!strncmp(entry->d_name, ".", 1)) {
        continue;
      }

      // 构建完整路径
      fullPath = path;
      if (path[path.size() - 1] != FILE_PATH_SPLIT) {
        fullPath += FILE_PATH_SPLIT;
      }
      fullPath += entry->d_name;
      memset(&fs, 0, sizeof(fs));
      if (stat(fullPath.c_str(), &fs) < 0) {
        cout << "Failed to stat " << fullPath << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
        continue;
      }

      // 处理子目录
      if (fs.st_mode & S_IFDIR) {
        if (recursive == 0) {
          continue;
        }

        if (getFileNum(fileNum, fullPath, pattern, recursive) < 0) {
          closedir(dirp);
          return -1;
        }
      }

      // 只处理普通文件
      if (!(fs.st_mode & S_IFREG)) {
        // not regular files
        continue;
      }

      // 检查文件名是否符合模式
      if (pattern.empty() == false && regex_match(entry->d_name, pattern.c_str())) {
        // Don't match
        continue;
      }

      fileNum++;
    }

    closedir(dirp);

    return 0;
  } catch (...) {
    cerr << "Failed to get file num " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
  }
  return -1;
}

/**
 * @brief 获取目录下符合条件的文件列表的实现
 * @param[out] fileList 输出参数，用于存储文件路径列表
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式
 * @param[in] recursive 是否递归搜索子目录
 * @return 成功返回0，失败返回-1
 * @details 遍历目录，收集符合条件的普通文件路径，支持递归搜索子目录
 */
int getFileList(vector<string> &fileList, const string &path, const string &pattern, bool recursive)
{
  try {
    DIR *dirp = NULL;
    dirp      = opendir(path.c_str());
    if (dirp == NULL) {
      cerr << "Failed to opendir " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      return -1;
    }

    string    fullPath;
    struct dirent *entry = NULL;
    struct stat    fs;
    while ((entry = readdir(dirp)) != NULL) {
      // 跳过"."、".."和以"."开头的隐藏文件
      if (!strncmp(entry->d_name, ".", 1)) {
        continue;
      }

      // 构建完整路径
      fullPath = path;
      if (path[path.size() - 1] != FILE_PATH_SPLIT) {
        fullPath += FILE_PATH_SPLIT;
      }
      fullPath += entry->d_name;
      memset(&fs, 0, sizeof(fs));
      if (stat(fullPath.c_str(), &fs) < 0) {
        cout << "Failed to stat " << fullPath << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
        continue;
      }

      // 处理子目录
      if (fs.st_mode & S_IFDIR) {
        if (recursive == 0) {
          continue;
        }

        if (getFileList(fileList, fullPath, pattern, recursive) < 0) {
          closedir(dirp);
          return -1;
        }
      }

      // 只处理普通文件
      if (!(fs.st_mode & S_IFREG)) {
        // regular files
        continue;
      }

      // 检查文件名是否符合模式
      if (pattern.empty() == false && regex_match(entry->d_name, pattern.c_str())) {
        // Don't match
        continue;
      }

      fileList.push_back(fullPath);
    }

    closedir(dirp);
    return 0;
  } catch (...) {
    cerr << "Failed to get file list " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
  }
  return -1;
}

/**
 * @brief 获取目录下子目录列表的实现
 * @param[out] dirList 输出参数，用于存储目录路径列表
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式
 * @return 成功返回0，失败返回-1
 * @details 遍历目录，收集符合条件的子目录路径，不递归搜索
 */
int getDirList(vector<string> &dirList, const string &path, const string &pattern)
{
  try {
    DIR *dirp = NULL;
    dirp      = opendir(path.c_str());
    if (dirp == NULL) {
      cerr << "Failed to opendir " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
      return -1;
    }

    string    fullPath;
    struct dirent *entry = NULL;
    struct stat    fs;
    while ((entry = readdir(dirp)) != NULL) {
      // 跳过"."、".."和以"."开头的隐藏目录
      if (!strncmp(entry->d_name, ".", 1)) {
        continue;
      }

      // 构建完整路径
      fullPath = path;
      if (path[path.size() - 1] != FILE_PATH_SPLIT) {
        fullPath += FILE_PATH_SPLIT;
      }
      fullPath += entry->d_name;
      memset(&fs, 0, sizeof(fs));
      if (stat(fullPath.c_str(), &fs) < 0) {
        cout << "Failed to stat " << fullPath << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
        continue;
      }

      // 只处理目录
      if ((fs.st_mode & S_IFDIR) == 0) {
        continue;
      }

      // 检查目录名是否符合模式
      if (pattern.empty() == false && regex_match(entry->d_name, pattern.c_str())) {
        // Don't match
        continue;
      }

      dirList.push_back(fullPath);
    }

    closedir(dirp);
    return 0;
  } catch (...) {
    cerr << "Failed to get file list " << path << SYS_OUTPUT_FILE_POS << SYS_OUTPUT_ERROR << endl;
  }
  return -1;
}

/**
 * @brief 创建或更新文件时间戳的实现
 * @param[in] path 要创建或更新的文件路径
 * @return 成功返回0，失败返回-1
 * @details 通过以追加模式打开文件来创建或更新文件时间戳，类似于Unix的touch命令
 */
int touch(const string &path)
{
  // CWE367: A check occurs on a file's attributes before
  // the file is used in a privileged operation, but things
  // may have changed

  // struct stat fs;

  // memset(&fs, 0, sizeof(fs));
  // if (stat(path.c_str(), &fs) == 0) {
  //   return 0;
  // }

  // create the file
  FILE *file = fopen(path.c_str(), "a");
  if (file == NULL) {
    return -1;
  }
  fclose(file);
  return 0;
}

/**
 * @brief 获取文件大小的实现
 * @param[in] filePath 文件路径
 * @param[out] fileLen 输出参数，用于存储文件大小
 * @return 成功返回0，失败返回错误码
 * @details 使用stat系统调用获取文件信息，检查文件是否存在以及是否为普通文件
 */
int getFileSize(const char *filePath, int64_t &fileLen)
{
  if (filePath == NULL || *filePath == '\0') {
    cerr << "invalid filepath" << endl;
    return -EINVAL;
  }
  struct stat statBuf;
  memset(&statBuf, 0, sizeof(statBuf));

  int rc = stat(filePath, &statBuf);
  if (rc) {
    cerr << "Failed to get stat of " << filePath << "," << errno << ":" << strerror(errno) << endl;
    return rc;
  }

  if (S_ISDIR(statBuf.st_mode)) {
    cerr << filePath << " is directory " << endl;
    return -EINVAL;
  }

  fileLen = statBuf.st_size;
  return 0;
}

/**
 * @brief 一次性写入所有指定数据到文件描述符的实现
 * @param[in] fd 文件描述符
 * @param[in] buf 要写入的数据缓冲区
 * @param[in] size 要写入的数据大小
 * @return 成功返回0，失败返回errno
 * @details 通过循环写入确保所有数据都被写入，处理EAGAIN和EINTR等信号中断情况
 */
int writen(int fd, const void *buf, int size)
{
  const char *tmp = (const char *)buf;
  while (size > 0) {
    const ssize_t ret = ::write(fd, tmp, size);
    if (ret >= 0) {
      tmp += ret;
      size -= ret;
      continue;
    }
    const int err = errno;
    if (EAGAIN != err && EINTR != err)
      return err;
  }
  return 0;
}

/**
 * @brief 一次性从文件描述符读取指定长度数据的实现
 * @param[in] fd 文件描述符
 * @param[out] buf 数据缓冲区，用于存储读取的数据
 * @param[in] size 要读取的数据长度
 * @return 成功返回0，返回-1表示读取到文件尾且未读取到指定大小数据，其他值表示errno
 * @details 通过循环读取确保读取指定大小的数据，处理EAGAIN、EINTR和文件结束等情况
 */
int readn(int fd, void *buf, int size)
{
  char *tmp = (char *)buf;
  while (size > 0) {
    const ssize_t ret = ::read(fd, tmp, size);
    if (ret > 0) {
      tmp += ret;
      size -= ret;
      continue;
    }
    if (0 == ret)
      return -1;  // end of file

    const int err = errno;
    if (EAGAIN != err && EINTR != err)
      return err;
  }
  return 0;
}
}  // namespace common
