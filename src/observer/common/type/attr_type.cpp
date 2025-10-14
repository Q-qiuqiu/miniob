/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */


#include "common/lang/string.h"
#include "common/type/attr_type.h"

/**
 * @brief 属性类型名称数组
 * @details 存储了 AttrType 枚举中各类型对应的字符串表示，索引与枚举值一一对应
 */
const char *ATTR_TYPE_NAME[] = {"undefined", "chars", "ints", "floats", "vectors", "booleans"};

/**
 * @brief 将属性类型转换为字符串表示
 * @param type 要转换的属性类型枚举值
 * @return 对应属性类型的字符串表示，如果类型无效则返回"unknown"
 * @note 函数会检查类型是否在有效范围内（UNDEFINED 到 MAXTYPE 之间）
 */
const char *attr_type_to_string(AttrType type)
{
  // 检查类型是否在有效范围内
  if (type >= AttrType::UNDEFINED && type < AttrType::MAXTYPE) {
    // 将枚举值转换为整数索引，获取对应的字符串
    return ATTR_TYPE_NAME[static_cast<int>(type)];
  }
  // 类型无效时返回"unknown"
  return "unknown";
}

/**
 * @brief 从字符串解析属性类型
 * @param s 表示属性类型的字符串
 * @return 对应的属性类型枚举值，如果无法解析则返回 UNDEFINED
 * @note 此函数使用不区分大小写的比较方式查找匹配的类型
 */
AttrType attr_type_from_string(const char *s)
{
  // 遍历所有已知的类型名称
  for (unsigned int i = 0; i < sizeof(ATTR_TYPE_NAME) / sizeof(ATTR_TYPE_NAME[0]); i++) {
    // 不区分大小写比较字符串
    if (0 == strcasecmp(ATTR_TYPE_NAME[i], s)) {
      // 找到匹配项，将索引转换为枚举值返回
      return (AttrType)i;
    }
  }
  // 没有找到匹配项，返回 UNDEFINED
  return AttrType::UNDEFINED;
}
