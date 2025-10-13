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
// Created by Longda on 2021/3/27.
//

#pragma once

#include "common/lang/string.h"

/**
 * @file path.h
 * @brief 文件路径操作相关接口定义
 * 
 * 该文件定义了一系列文件路径操作的函数接口，包括获取文件名、目录名、
 * 绝对路径、判断是否为目录、检查并创建目录、列出目录中的文件等功能。
 * 这些函数为系统提供了统一的路径处理能力，便于跨平台使用。
 */
namespace common {

/**
 * @brief 从完整路径中获取文件名
 * 
 * 该函数从给定的完整路径中提取文件名部分。
 * 如果路径以目录分隔符结尾，则返回空字符串。
 * 
 * 示例：
 * - 输入: "/test/happy/"  --> 返回: ""
 * - 输入: "/test/happy"   --> 返回: "happy"
 * - 输入: "test/happy"    --> 返回: "happy"
 * - 输入: "happy"         --> 返回: "happy"
 * - 输入: ""              --> 返回: ""
 * 
 * @param[in] fullPath 完整的文件路径
 * @return string 提取出的文件名
 */
string getFileName(const string &fullPath);

/**
 * @brief 从路径中获取文件名（C风格字符串版本）
 * 
 * 该函数是getFileName的C风格字符串版本，将提取出的文件名存储在传入的引用参数中。
 * 
 * @param[in] path 完整的文件路径（C风格字符串）
 * @param[out] fileName 存储提取出的文件名的字符串引用
 */
void   getFileName(const char *path, string &fileName);

/**
 * @brief 从完整路径中获取目录路径
 * 
 * 该函数从给定的完整路径中提取目录路径部分。
 * 如果路径中不包含目录分隔符，则返回原路径。
 * 
 * 示例：
 * - 输入: "/test/happy"   --> 返回: "/test"
 * - 输入: "test/happy"    --> 返回: "test"
 * - 输入: "happy"         --> 返回: "happy"
 * - 输入: ""              --> 返回: ""
 * 
 * @param[in] fullPath 完整的文件路径
 * @return string 提取出的目录路径
 */
string getFilePath(const string &fullPath);

/**
 * @brief 从路径中获取父目录路径（C风格字符串版本）
 * 
 * 该函数从给定的C风格字符串路径中提取父目录路径，并存储在传入的引用参数中。
 * 
 * @param[in] path 完整的文件路径（C风格字符串）
 * @param[out] parent 存储提取出的父目录路径的字符串引用
 */
void   getDirName(const char *path, string &parent);

/**
 * @brief 获取路径的绝对路径形式
 * 
 * 该函数将相对路径转换为绝对路径。
 * 
 * @param[in] path 输入路径（可以是相对路径或绝对路径）
 * @return string 转换后的绝对路径
 */
string getAboslutPath(const char *path);

/**
 * @brief 判断给定路径是否为目录
 * 
 * 该函数检查给定的路径是否指向一个目录。
 * 
 * @param[in] path 要检查的路径
 * @return bool 如果是目录则返回true，否则返回false
 */
bool is_directory(const char *path);

/**
 * @brief 检查目录是否存在，如果不存在则逐级创建
 * 
 * 该函数首先移除路径末尾的所有斜杠，然后检查目录是否存在。
 * 如果目录不存在，则从根目录开始逐级创建所需的所有父目录。
 * 
 * @param[in,out] path 要检查或创建的目录路径，会被修改为标准化的路径（去除末尾斜杠）
 * @return bool 目录存在或创建成功返回true，否则返回false
 */
bool check_directory(string &path);

/**
 * @brief 列出指定目录下符合正则表达式模式的所有文件
 * 
 * 该函数扫描指定目录，返回所有文件名匹配给定正则表达式模式的文件列表。
 * 注意：此函数不会递归到子目录中，只会列出当前目录下的文件。
 * 
 * @param[in] path 要扫描的目录路径
 * @param[in] filter_pattern 用于过滤文件名的正则表达式模式，如果为nullptr则不过滤
 * @param[out] files 存储匹配到的文件名列表的vector引用
 * @return int 成功时返回匹配到的文件数量，失败时返回-1
 */
int list_file(const char *path, const char *filter_pattern, vector<string> &files);  // io/io.h::getFileList

}  // namespace common
