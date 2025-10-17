/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Meiyi & Longda on 2021/5/11.
//
#pragma once

#include "storage/db/db.h"               ///< 包含数据库类定义
#include "common/lang/span.h"           ///< 包含span类型定义
#include "common/lang/map.h"            ///< 包含map容器扩展
#include "common/lang/string.h"         ///< 包含字符串处理函数
#include "common/lang/memory.h"         ///< 包含内存管理相关函数

class Trx;     ///< 前向声明事务类
class TrxKit;  ///< 前向声明事务工具类

/**
 * @brief 默认存储引擎处理器
 * @details 数据库存储引擎的核心入口类，作为SQL层与存储层交互的主要接口。
 * 负责数据库的创建、删除、打开、关闭等基本操作，以及表的创建和删除等元数据管理。
 * 参考MySQL的handler层设计思想，但当前实现较为基础。
 */
class DefaultHandler
{
public:
  /**
   * @brief 构造函数
   * @details 初始化DefaultHandler的基本成员变量
   */
  DefaultHandler();

  /**
   * @brief 析构函数
   * @details 自动调用destroy()方法释放所有资源
   */
  virtual ~DefaultHandler() noexcept;

  /**
   * @brief 初始化存储引擎
   * @details 设置存储目录，初始化事务模型和日志处理器，创建系统数据库
   * @param base_dir 存储引擎的根目录，所有数据库相关数据文件都放在这个目录下
   * @param trx_kit_name 事务模型名称，指定使用的事务处理方式
   * @param log_handler_name 日志处理器名称，指定使用的日志处理方式
   * @param storage_engine 存储引擎名称，指定使用的存储引擎类型
   * @return RC 操作结果状态码，成功返回SUCCESS，失败返回相应错误码
   */
  RC   init(const char *base_dir, const char *trx_kit_name, const char *log_handler_name, const char *storage_engine);
  
  /**
   * @brief 销毁存储引擎
   * @details 同步所有数据到磁盘，释放所有打开的数据库资源
   */
  void destroy();

  /**
   * @brief 创建数据库
   * @details 在存储目录下创建一个名为dbname的数据库目录
   * @param dbname 数据库名称，不能为空或空白字符串
   * @return RC 操作结果状态码，成功返回SUCCESS，数据库已存在返回SCHEMA_DB_EXIST，参数错误返回INVALID_ARGUMENT
   */
  RC create_db(const char *dbname);

  /**
   * @brief 删除数据库
   * @details 当前尚未实现此功能
   * @param dbname 数据库名称
   * @return RC 当前返回INTERNAL错误码
   */
  RC drop_db(const char *dbname);

  /**
   * @brief 打开数据库
   * @details 加载数据库信息到内存，初始化数据库相关组件
   * @param dbname 数据库名称，不能为空或空白字符串
   * @return RC 操作结果状态码，成功返回SUCCESS，数据库不存在返回SCHEMA_DB_NOT_EXIST
   */
  RC open_db(const char *dbname);

  /**
   * @brief 关闭指定数据库
   * @details 该操作将关闭数据库中打开的所有文件，将缓冲区页面更新到磁盘
   * @param dbname 数据库名称
   * @return RC 当前返回UNIMPLEMENTED，功能未实现
   */
  RC close_db(const char *dbname);

  /**
   * @brief 在指定数据库下创建表
   * @param dbname 数据库名称
   * @param relation_name 表名
   * @param attributes 属性信息数组，定义表的结构
   * @return RC 操作结果状态码，成功返回SUCCESS，数据库未打开返回SCHEMA_DB_NOT_OPENED
   */
  RC create_table(const char *dbname, const char *relation_name, span<const AttrInfoSqlNode> attributes);

  /**
   * @brief 删除指定数据库下的表
   * @details 当前没有实现此功能，需要删除表在内存中和磁盘中的所有资源
   * @param dbname 数据库名称
   * @param relation_name 表名
   * @return RC 当前返回UNIMPLEMENTED，功能未实现
   */
  RC drop_table(const char *dbname, const char *relation_name);

public:
  /**
   * @brief 查找已打开的数据库
   * @param dbname 数据库名称
   * @return Db* 找到返回数据库指针，未找到返回nullptr
   */
  Db    *find_db(const char *dbname) const;
  
  /**
   * @brief 查找指定数据库中的表
   * @param dbname 数据库名称
   * @param table_name 表名
   * @return Table* 找到返回表指针，未找到或参数错误返回nullptr
   */
  Table *find_table(const char *dbname, const char *table_name) const;

  /**
   * @brief 同步所有打开的数据库
   * @details 将所有打开的数据库的数据同步到磁盘
   * @return RC 操作结果状态码，成功返回SUCCESS，任一数据库同步失败返回相应错误码
   */
  RC sync();

private:
  filesystem::path  base_dir_;          ///< 存储引擎的根目录路径
  filesystem::path  db_dir_;            ///< 数据库文件的根目录路径，通常为base_dir_/db
  string            trx_kit_name_;      ///< 事务模型的名称，用于创建事务管理器
  string            log_handler_name_;  ///< 日志处理器的名称，用于创建日志管理器
  map<string, Db *> opened_dbs_;        ///< 存储所有已打开的数据库，键为数据库名，值为数据库指针
  string            storage_engine_;    ///< 存储引擎的名称，指定使用的存储实现
};
