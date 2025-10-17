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
// Created by Meiyi & Longda on 2021/4/13.
//

/**
 * @file default_handler.cpp
 * @brief 默认存储引擎处理器的实现文件
 * @details 实现DefaultHandler类的所有成员函数，包括数据库的创建、打开、关闭等核心操作，
 * 以及表的管理功能。作为SQL层与存储层交互的主要接口。
 */

#include "storage/default/default_handler.h"         ///< 默认处理器头文件

#include "common/lang/string.h"                    ///< 字符串处理函数
#include "common/log/log.h"                       ///< 日志系统
#include "common/os/path.h"                       ///< 路径操作函数
#include "session/session.h"                      ///< 会话管理
#include "storage/common/condition_filter.h"      ///< 条件过滤器
#include "storage/index/bplus_tree.h"             ///< B+树索引实现
#include "storage/record/record_manager.h"        ///< 记录管理器
#include "storage/table/table.h"                  ///< 表实现
#include "storage/trx/trx.h"                      ///< 事务定义

using namespace std;  ///< 使用标准命名空间

/**
 * @brief DefaultHandler构造函数实现
 * @details 初始化DefaultHandler对象，无特殊初始化逻辑
 */
DefaultHandler::DefaultHandler() {}

/**
 * @brief DefaultHandler析构函数实现
 * @details 在对象销毁时自动调用destroy()方法释放所有资源
 */
DefaultHandler::~DefaultHandler() noexcept { destroy(); }

/**
 * @brief 初始化存储引擎
 * @details 设置存储目录，创建数据库目录，初始化系统数据库
 * @param base_dir 存储引擎的根目录
 * @param trx_kit_name 事务模型名称
 * @param log_handler_name 日志处理器名称
 * @param storage_engine 存储引擎名称
 * @return RC 操作结果状态码，成功返回SUCCESS，失败返回相应错误码
 */
RC DefaultHandler::init(const char *base_dir, const char *trx_kit_name, const char *log_handler_name, const char *storage_engine)
{
  // 检查目录是否存在，或者创建
  filesystem::path db_dir(base_dir);
  db_dir /= "db";  ///< 数据库目录路径为base_dir/db
  error_code ec;
  if (!filesystem::is_directory(db_dir) && !filesystem::create_directories(db_dir, ec)) {
    LOG_ERROR("Cannot access base dir: %s. msg=%d:%s", db_dir.c_str(), errno, strerror(errno));
    return RC::INTERNAL;
  }

  // 初始化成员变量
  base_dir_ = base_dir;           ///< 设置存储引擎根目录
  db_dir_   = db_dir;             ///< 设置数据库目录
  trx_kit_name_ = trx_kit_name;   ///< 设置事务模型名称
  log_handler_name_ = log_handler_name; ///< 设置日志处理器名称
  storage_engine_ = storage_engine;     ///< 设置存储引擎名称

  const char *sys_db = "sys";  ///< 系统数据库名称

  // 创建系统数据库
  RC ret = create_db(sys_db);
  if (ret != RC::SUCCESS && ret != RC::SCHEMA_DB_EXIST) {  ///< 数据库已存在是可接受的
    LOG_ERROR("Failed to create system db");
    return ret;
  }

  // 打开系统数据库
  ret = open_db(sys_db);
  if (ret != RC::SUCCESS) {
    LOG_ERROR("Failed to open system db. rc=%s", strrc(ret));
    return ret;
  }

  // 设置默认会话的当前数据库
  Session &default_session = Session::default_session();
  default_session.set_current_db(sys_db);

  LOG_INFO("Default handler init with %s success", base_dir);
  return RC::SUCCESS;
}

/**
 * @brief 销毁存储引擎
 * @details 先同步所有数据到磁盘，然后释放所有打开的数据库资源
 */
void DefaultHandler::destroy()
{
  sync();  ///< 同步所有数据到磁盘

  // 释放所有打开的数据库
  for (const auto &iter : opened_dbs_) {
    delete iter.second;  ///< 释放数据库对象内存
  }
  opened_dbs_.clear();  ///< 清空打开的数据库集合
}

/**
 * @brief 创建数据库
 * @details 在数据库目录下创建指定名称的数据库文件夹
 * @param dbname 数据库名称
 * @return RC 操作结果状态码，成功返回SUCCESS，参数错误返回INVALID_ARGUMENT，数据库已存在返回SCHEMA_DB_EXIST
 */
RC DefaultHandler::create_db(const char *dbname)
{
  // 参数有效性检查
  if (nullptr == dbname || common::is_blank(dbname)) {
    LOG_WARN("Invalid db name");
    return RC::INVALID_ARGUMENT;
  }

  // 检查数据库是否已存在
  filesystem::path dbpath = db_dir_ / dbname;  ///< 构造数据库路径
  if (filesystem::is_directory(dbpath)) {
    LOG_WARN("Db already exists: %s", dbname);
    return RC::SCHEMA_DB_EXIST;
  }

  // 创建数据库目录
  error_code ec;
  if (!filesystem::create_directories(dbpath, ec)) {
    LOG_ERROR("Create db fail: %s. error=%s", dbpath.c_str(), strerror(errno));
    return RC::IOERR_WRITE;
  }
  return RC::SUCCESS;
}

/**
 * @brief 删除数据库
 * @details 当前未实现此功能，直接返回INTERNAL错误码
 * @param dbname 数据库名称
 * @return RC 始终返回INTERNAL错误码
 */
RC DefaultHandler::drop_db(const char *dbname) { return RC::INTERNAL; }

/**
 * @brief 打开数据库
 * @details 加载数据库信息到内存，初始化数据库组件
 * @param dbname 数据库名称
 * @return RC 操作结果状态码，成功返回SUCCESS，参数错误返回INVALID_ARGUMENT，数据库不存在返回SCHEMA_DB_NOT_EXIST
 */
RC DefaultHandler::open_db(const char *dbname)
{
  // 参数有效性检查
  if (nullptr == dbname || common::is_blank(dbname)) {
    LOG_WARN("Invalid db name");
    return RC::INVALID_ARGUMENT;
  }

  // 检查数据库是否已经打开
  if (opened_dbs_.find(dbname) != opened_dbs_.end()) {
    return RC::SUCCESS;  ///< 数据库已打开，直接返回成功
  }

  // 检查数据库目录是否存在
  filesystem::path dbpath = db_dir_ / dbname;
  if (!filesystem::is_directory(dbpath)) {
    return RC::SCHEMA_DB_NOT_EXIST;
  }

  // 打开数据库
  Db *db  = new Db();
  RC  ret = RC::SUCCESS;
  if ((ret = db->init(dbname, dbpath.c_str(), trx_kit_name_.c_str(), log_handler_name_.c_str(), storage_engine_.c_str())) != RC::SUCCESS) {
    LOG_ERROR("Failed to open db: %s. error=%s", dbname, strrc(ret));
    delete db;  ///< 初始化失败，释放内存
  } else {
    opened_dbs_[dbname] = db;  ///< 将数据库添加到已打开数据库集合
  }
  return ret;
}

/**
 * @brief 关闭指定数据库
 * @details 当前未实现此功能，直接返回UNIMPLEMENTED错误码
 * @param dbname 数据库名称
 * @return RC 始终返回UNIMPLEMENTED错误码
 */
RC DefaultHandler::close_db(const char *dbname) { return RC::UNIMPLEMENTED; }

/**
 * @brief 在指定数据库下创建表
 * @details 在已打开的数据库中创建新表
 * @param dbname 数据库名称
 * @param relation_name 表名
 * @param attributes 属性信息数组，定义表的结构
 * @return RC 操作结果状态码，成功返回SUCCESS，数据库未打开返回SCHEMA_DB_NOT_OPENED
 * @note 代码中有TODO注释，提示未来可能移除DefaultHandler
 */
// TODO: remove DefaultHandler
RC DefaultHandler::create_table(const char *dbname, const char *relation_name, span<const AttrInfoSqlNode> attributes)
{
  // 查找已打开的数据库
  Db *db = find_db(dbname);
  if (db == nullptr) {
    return RC::SCHEMA_DB_NOT_OPENED;
  }
  // 委托数据库对象创建表，第三个参数为空向量（不创建索引）
  return db->create_table(relation_name, attributes, {});
}

/**
 * @brief 删除指定数据库下的表
 * @details 当前未实现此功能，直接返回UNIMPLEMENTED错误码
 * @param dbname 数据库名称
 * @param relation_name 表名
 * @return RC 始终返回UNIMPLEMENTED错误码
 */
RC DefaultHandler::drop_table(const char *dbname, const char *relation_name) { return RC::UNIMPLEMENTED; }

/**
 * @brief 查找已打开的数据库
 * @details 在已打开的数据库集合中查找指定名称的数据库
 * @param dbname 数据库名称
 * @return Db* 找到返回数据库指针，未找到返回nullptr
 */
Db *DefaultHandler::find_db(const char *dbname) const
{
  map<string, Db *>::const_iterator iter = opened_dbs_.find(dbname);
  if (iter == opened_dbs_.end()) {
    return nullptr;
  }
  return iter->second;
}

/**
 * @brief 查找指定数据库中的表
 * @details 先查找数据库，然后在数据库中查找表
 * @param dbname 数据库名称
 * @param table_name 表名
 * @return Table* 找到返回表指针，未找到或参数错误返回nullptr
 */
Table *DefaultHandler::find_table(const char *dbname, const char *table_name) const
{
  // 参数有效性检查
  if (dbname == nullptr || table_name == nullptr) {
    LOG_WARN("Invalid argument. dbname=%p, table_name=%p", dbname, table_name);
    return nullptr;
  }
  
  // 查找数据库
  Db *db = find_db(dbname);
  if (nullptr == db) {
    return nullptr;
  }

  // 在数据库中查找表
  return db->find_table(table_name);
}

/**
 * @brief 同步所有打开的数据库
 * @details 将所有打开的数据库的数据同步到磁盘，如果任一数据库同步失败则立即返回错误
 * @return RC 操作结果状态码，成功返回SUCCESS，任一数据库同步失败返回相应错误码
 */
RC DefaultHandler::sync()
{
  RC rc = RC::SUCCESS;
  
  // 遍历所有已打开的数据库并同步
  for (const auto &db_pair : opened_dbs_) {
    Db *db = db_pair.second;
    rc     = db->sync();
    if (rc != RC::SUCCESS) {
      LOG_ERROR("Failed to sync db. name=%s, rc=%d:%s", db->name(), rc, strrc(rc));
      return rc;  ///< 任一数据库同步失败，立即返回错误
    }
  }
  return rc;
}
