/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include "storage/record/record.h"
#include "storage/common/condition_filter.h"

/**
 * @brief 记录扫描器抽象基类
 * @details 该类定义了遍历表中记录的标准接口，是所有具体记录扫描器实现的基础。
 *          通过这个接口，可以以统一的方式访问和遍历表中的记录，而不需要关心底层存储结构的细节。
 * @ingroup RecordManager
 * @note 这是一个纯虚函数接口类，需要由具体的子类实现其功能。
 */
class RecordScanner
{
public:
  /**
   * @brief 默认构造函数
   * @details 初始化记录扫描器的基本状态。
   */
  RecordScanner()          = default;
  
  /**
   * @brief 虚析构函数
   * @details 确保派生类的资源能够正确释放，防止内存泄漏。
   */
  virtual ~RecordScanner() = default;

  /**
   * @brief 打开记录扫描器
   * @details 初始化扫描状态，准备开始扫描操作。
   * @return RC 返回操作结果状态码
   *         - RC::SUCCESS: 操作成功
   *         - 其他错误码: 操作失败，具体错误信息取决于实现
   */
  virtual RC open_scan() = 0;

  /**
   * @brief 关闭记录扫描器
   * @details 释放扫描器占用的资源，重置扫描状态。
   * @return RC 返回操作结果状态码
   *         - RC::SUCCESS: 操作成功
   *         - 其他错误码: 操作失败，具体错误信息取决于实现
   */
  virtual RC close_scan() = 0;

  /**
   * @brief 获取下一条记录
   * @details 移动到下一条符合条件的记录，并将其内容填充到传入的record对象中。
   *          当没有更多记录时，应返回相应的状态码。
   * @param[out] record 用于存储返回记录的引用
   * @return RC 返回操作结果状态码
   *         - RC::SUCCESS: 成功获取到一条记录
   *         - RC::RECORD_EOF: 已到达记录末尾，没有更多记录
   *         - 其他错误码: 操作失败，具体错误信息取决于实现
   */
  virtual RC next(Record &record) = 0;
};