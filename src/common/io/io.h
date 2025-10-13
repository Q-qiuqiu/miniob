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

#include <vector>

#include "common/defs.h"
#include "common/lang/string.h"
#include "common/lang/vector.h"

namespace common {

/**
 * @file io.h
 * @brief IO操作模块
 * @details 提供文件读写、文件信息获取、目录操作等基础IO功能的接口定义
 */

/**
 * @brief 从文件中读取数据
 * @param[in] fileName 要读取的文件名
 * @param[out] data 输出参数，用于存储读取到的数据，需要调用者释放
 * @param[out] fileSize 输出参数，用于存储文件大小
 * @return 成功返回0，失败返回-1
 * @details 读取成功后，会将文件内容存储到data指向的内存中，
 *          失败时不会修改data的值，调用者需要负责释放成功读取时分配的内存
 */
int readFromFile(const string &fileName, char *&data, size_t &fileSize);

/**
 * @brief 将数据写入文件
 * @param[in] fileName 要写入的文件名
 * @param[in] data 要写入的数据
 * @param[in] dataSize 要写入的数据大小
 * @param[in] openMode 文件打开模式（如"w"、"a"、"wb"等）
 * @return 成功返回0，失败返回-1
 * @details 根据指定的打开模式将数据写入文件，支持文本和二进制模式
 */
int writeToFile(const string &fileName, const char *data, uint32_t dataSize, const char *openMode);

/**
 * @brief 获取文件中非空行的数量
 * @param[in] fileName 要统计的文件名
 * @param[out] lineNum 输出参数，用于存储非空行的数量
 * @return 成功返回0，失败返回-1
 * @details 仅统计非空行（经过strip处理后长度不为0的行）
 */
int getFileLines(const string &fileName, uint64_t &lineNum);

/**
 * @brief 获取目录下符合条件的文件列表
 * @param[out] fileList 输出参数，用于存储找到的文件路径列表
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式，为空时不进行模式匹配
 * @param[in] recursive 是否递归搜索子目录
 * @return 成功返回0，失败返回错误码
 * @details 不处理"."、".."和".*"开头的隐藏文件，只统计普通文件（不包含目录）
 */
int getFileList(vector<string> &fileList, const string &path, const string &pattern, bool recursive);

/**
 * @brief 获取目录下符合条件的文件数量
 * @param[out] fileNum 输出参数，用于存储找到的文件数量
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式，为空时不进行模式匹配
 * @param[in] recursive 是否递归搜索子目录
 * @return 成功返回0，失败返回错误码
 * @details 不处理"."、".."和".*"开头的隐藏文件，只统计普通文件（不包含目录）
 */
int getFileNum(uint64_t &fileNum, const string &path, const string &pattern, bool recursive);

/**
 * @brief 获取目录下的子目录列表
 * @param[out] dirList 输出参数，用于存储找到的子目录路径列表
 * @param[in] path 要搜索的目录路径
 * @param[in] pattern 正则表达式模式，为空时不进行模式匹配
 * @return 成功返回0，失败返回错误码
 * @details 不处理"."、".."和".*"开头的隐藏目录
 */
int getDirList(vector<string> &dirList, const string &path, const string &pattern);

/**
 * @brief 创建文件（如果文件已存在则更新时间戳）
 * @param[in] fileName 要创建或更新的文件名
 * @return 成功返回0，失败返回-1
 * @details 类似于Unix的touch命令，如果文件不存在则创建空文件，
 *          如果文件已存在则更新其访问和修改时间
 */
int touch(const string &fileName);

/**
 * @brief 获取文件大小
 * @param[in] filePath 文件路径
 * @param[out] fileLen 输出参数，用于存储文件大小（字节数）
 * @return 成功返回0，失败返回错误码
 * @details 检查文件是否存在并获取其大小，对于目录返回错误
 */
int getFileSize(const char *filePath, int64_t &fileLen);

/**
 * @brief 一次性写入所有指定数据到文件描述符
 * @param[in] fd 文件描述符
 * @param[in] buf 要写入的数据缓冲区
 * @param[in] size 要写入的数据大小
 * @return 成功返回0，失败返回errno
 * @details 确保写入指定大小的数据，处理EAGAIN和EINTR等中断情况
 */
int writen(int fd, const void *buf, int size);

/**
 * @brief 一次性从文件描述符读取指定长度的数据
 * @param[in] fd 文件描述符
 * @param[out] buf 数据缓冲区，用于存储读取的数据
 * @param[in] size 要读取的数据长度
 * @return 成功返回0，返回-1表示读取到文件尾且未读取到指定大小数据，其他值表示errno
 * @details 确保读取指定大小的数据，处理EAGAIN和EINTR等中断情况
 */
int readn(int fd, void *buf, int size);

}  // namespace common
