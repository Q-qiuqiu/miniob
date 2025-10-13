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
// Created by Longda on 2021/5/2.
//

#pragma once

/**
 * @brief 这个文件定义函数返回码/错误码(Return Code)
 * @enum RC
 * 
 * 该文件采用了枚举类型来定义统一的返回码系统，用于在整个项目中标识操作的执行结果。
 * 通过宏定义机制，实现了错误码的统一管理和扩展。同时提供了辅助函数来简化返回码的判断和处理。
 * 错误码按照功能模块进行了分类，便于管理和维护。
 */

/**
 * @brief 定义所有返回码的宏
 * 
 * 使用宏定义机制可以方便地扩展和维护错误码列表。错误码按照功能模块进行了逻辑分组。
 */
#define DEFINE_RCS                       \
  /* 基础错误码 */                       \
  DEFINE_RC(SUCCESS)                     /* 操作成功 */                           \
  DEFINE_RC(INVALID_ARGUMENT)            /* 参数无效 */                           \
  DEFINE_RC(UNIMPLEMENTED)               /* 功能未实现 */                         \
  DEFINE_RC(SQL_SYNTAX)                  /* SQL语法错误 */                        \
  DEFINE_RC(INTERNAL)                    /* 内部错误 */                           \
  DEFINE_RC(NOMEM)                       /* 内存不足 */                           \
  DEFINE_RC(NOTFOUND)                    /* 未找到 */                             \
  DEFINE_RC(EMPTY)                       /* 为空 */                               \
  DEFINE_RC(FULL)                        /* 已满 */                               \
  DEFINE_RC(EXIST)                       /* 已存在 */                             \
  DEFINE_RC(NOT_EXIST)                   /* 不存在 */                             \
  /* 缓冲区池相关错误 */                 \
  DEFINE_RC(BUFFERPOOL_OPEN)             /* 缓冲区池打开失败 */                   \
  DEFINE_RC(BUFFERPOOL_NOBUF)            /* 缓冲区池中无可用缓冲区 */             \
  DEFINE_RC(BUFFERPOOL_INVALID_PAGE_NUM) /* 无效的页面编号 */                     \
  /* 记录管理相关错误 */                 \
  DEFINE_RC(RECORD_OPENNED)              /* 记录已被打开 */                       \
  DEFINE_RC(RECORD_INVALID_RID)          /* 无效的记录ID */                       \
  DEFINE_RC(RECORD_INVALID_KEY)          /* 无效的键值 */                         \
  DEFINE_RC(RECORD_DUPLICATE_KEY)        /* 键值重复 */                           \
  DEFINE_RC(RECORD_NOMEM)                /* 记录操作内存不足 */                   \
  DEFINE_RC(RECORD_EOF)                  /* 记录遍历到达末尾 */                   \
  DEFINE_RC(RECORD_NOT_EXIST)            /* 记录不存在 */                         \
  DEFINE_RC(RECORD_INVISIBLE)            /* 记录不可见(可能已被删除或未提交) */   \
  /* 模式(schema)相关错误 */             \
  DEFINE_RC(SCHEMA_DB_EXIST)             /* 数据库已存在 */                       \
  DEFINE_RC(SCHEMA_DB_NOT_EXIST)         /* 数据库不存在 */                       \
  DEFINE_RC(SCHEMA_DB_NOT_OPENED)        /* 数据库未打开 */                       \
  DEFINE_RC(SCHEMA_TABLE_NOT_EXIST)      /* 表不存在 */                           \
  DEFINE_RC(SCHEMA_TABLE_EXIST)          /* 表已存在 */                           \
  DEFINE_RC(SCHEMA_FIELD_NOT_EXIST)      /* 字段不存在 */                         \
  DEFINE_RC(SCHEMA_FIELD_MISSING)        /* 缺少字段 */                           \
  DEFINE_RC(SCHEMA_FIELD_TYPE_MISMATCH)  /* 字段类型不匹配 */                     \
  DEFINE_RC(SCHEMA_INDEX_NAME_REPEAT)    /* 索引名重复 */                         \
  /* IO相关错误 */                       \
  DEFINE_RC(IOERR_READ)                  /* 读取错误 */                           \
  DEFINE_RC(IOERR_WRITE)                 /* 写入错误 */                           \
  DEFINE_RC(IOERR_ACCESS)                /* 访问错误 */                           \
  DEFINE_RC(IOERR_OPEN)                  /* 打开错误 */                           \
  DEFINE_RC(IOERR_CLOSE)                 /* 关闭错误 */                           \
  DEFINE_RC(IOERR_SEEK)                  /* 定位错误 */                           \
  DEFINE_RC(IOERR_TOO_LONG)              /* 数据太长 */                           \
  DEFINE_RC(IOERR_SYNC)                  /* 同步错误 */                           \
  /* 锁相关错误 */                       \
  DEFINE_RC(LOCKED_UNLOCK)               /* 解锁错误 */                           \
  DEFINE_RC(LOCKED_NEED_WAIT)            /* 需要等待锁 */                         \
  DEFINE_RC(LOCKED_CONCURRENCY_CONFLICT) /* 并发冲突 */                           \
  /* 文件相关错误 */                     \
  DEFINE_RC(FILE_EXIST)                  /* 文件已存在 */                         \
  DEFINE_RC(FILE_NOT_EXIST)              /* 文件不存在 */                         \
  DEFINE_RC(FILE_NAME)                   /* 文件名无效 */                         \
  DEFINE_RC(FILE_BOUND)                  /* 文件边界错误 */                       \
  DEFINE_RC(FILE_CREATE)                 /* 文件创建失败 */                       \
  DEFINE_RC(FILE_OPEN)                   /* 文件打开失败 */                       \
  DEFINE_RC(FILE_NOT_OPENED)             /* 文件未打开 */                         \
  DEFINE_RC(FILE_CLOSE)                  /* 文件关闭失败 */                       \
  DEFINE_RC(FILE_REMOVE)                 /* 文件删除失败 */                       \
  /* 其他错误 */                         \
  DEFINE_RC(VARIABLE_NOT_EXISTS)         /* 变量不存在 */                         \
  DEFINE_RC(VARIABLE_NOT_VALID)          /* 变量无效 */                           \
  DEFINE_RC(LOGBUF_FULL)                 /* 日志缓冲区已满 */                     \
  DEFINE_RC(LOG_FILE_FULL)               /* 日志文件已满 */                       \
  DEFINE_RC(LOG_ENTRY_INVALID)           /* 日志条目无效 */                       \
  DEFINE_RC(JSON_PARSE_FAILED)           /* JSON解析失败 */                       \
  DEFINE_RC(JSON_MEMBER_MISSING)         /* JSON成员缺失 */                       \
  DEFINE_RC(RANGE_ERROR)                 /* 范围错误 */                           \
  DEFINE_RC(WAL_INVALID_FILENAME)        /* WAL文件名无效 */                      \
  DEFINE_RC(INPUT_EOF)                   /* 输入到达末尾 */                       \
  DEFINE_RC(INVALID_TOKEN)               /* 无效的令牌 */                         \
  DEFINE_RC(UNEXPECTED_END_OF_STRING)    /* 字符串意外结束 */                     \
  DEFINE_RC(SYNTAX_ERROR)                /* 语法错误 */                           \
  DEFINE_RC(UNSUPPORTED)                 /* 不支持的功能 */

/**
 * @brief 返回码枚举类
 * 
 * 使用enum class类型来确保类型安全，防止与其他整数类型混淆。
 * 通过宏展开的方式定义所有枚举值，便于统一管理和扩展。
 */
enum class RC
{
#define DEFINE_RC(name) name,
  DEFINE_RCS
#undef DEFINE_RC
};

/**
 * @brief 将返回码转换为可读的字符串
 * 
 * 该函数返回一个指向描述给定返回码的静态字符串的指针，用于日志记录和错误信息展示。
 * @param rc 需要转换的返回码
 * @return 描述返回码的字符串
 */
extern const char *strrc(RC rc);

/**
 * @brief 检查返回码是否表示成功
 * 
 * 用于简化代码中对操作成功的判断。
 * @param rc 需要检查的返回码
 * @return 如果返回码是SUCCESS，则返回true，否则返回false
 */
extern bool OB_SUCC(RC rc);

/**
 * @brief 检查返回码是否表示失败
 * 
 * 用于简化代码中对操作失败的判断。
 * @param rc 需要检查的返回码
 * @return 如果返回码不是SUCCESS，则返回true，否则返回false
 */
extern bool OB_FAIL(RC rc);
