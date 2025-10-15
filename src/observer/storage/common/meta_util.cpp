/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

// Created by wangyunlai.wyl on 2021/5/18.
//

#include "storage/common/meta_util.h"
#include "common/lang/filesystem.h"

/**
 * @brief 构建数据库元数据文件的完整路径
 * @details 将基础目录路径与数据库名称和.db后缀组合，生成数据库元数据文件的完整路径。
 * @param[in] base_dir 基础目录路径
 * @param[in] db_name 数据库名称
 * @return 数据库元数据文件的完整路径字符串
 */
string db_meta_file(const char *base_dir, const char *db_name)
{
  filesystem::path db_dir = filesystem::path(base_dir);
  return db_dir / (string(db_name) + DB_META_SUFFIX);
}

/**
 * @brief 构建表元数据文件的完整路径
 * @details 将基础目录路径与表名称和.table后缀组合，生成表元数据文件的完整路径。
 * @param[in] base_dir 基础目录路径
 * @param[in] table_name 表名称
 * @return 表元数据文件的完整路径字符串
 */
string table_meta_file(const char *base_dir, const char *table_name)
{
  return filesystem::path(base_dir) / (string(table_name) + TABLE_META_SUFFIX);
}

/**
 * @brief 构建表数据文件的完整路径
 * @details 将基础目录路径与表名称和.data后缀组合，生成表数据文件的完整路径。
 * @param[in] base_dir 基础目录路径
 * @param[in] table_name 表名称
 * @return 表数据文件的完整路径字符串
 */
string table_data_file(const char *base_dir, const char *table_name)
{
  return filesystem::path(base_dir) / (string(table_name) + TABLE_DATA_SUFFIX);
}

/**
 * @brief 构建表索引文件的完整路径
 * @details 将基础目录路径与表名称、索引名称和.index后缀组合，生成表索引文件的完整路径。
 * 格式为：表名称-索引名称.index
 * @param[in] base_dir 基础目录路径
 * @param[in] table_name 表名称
 * @param[in] index_name 索引名称
 * @return 表索引文件的完整路径字符串
 */
string table_index_file(const char *base_dir, const char *table_name, const char *index_name)
{
  return filesystem::path(base_dir) / (string(table_name) + "-" + index_name + TABLE_INDEX_SUFFIX);
}
