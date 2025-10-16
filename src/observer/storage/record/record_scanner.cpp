/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/**
 * @file record_scanner.cpp
 * @brief 记录扫描器实现文件
 * @details 此文件是RecordScanner抽象基类的实现声明文件。由于RecordScanner是纯虚函数接口类，
 *          实际的实现由其派生类提供。此文件主要用于确保编译系统能够正确识别该类的实现位置。
 */

#include "storage/record/record_scanner.h"  ///< 包含记录扫描器的头文件定义