/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "storage/common/codec.h"

/**
 * @brief 字符串终止标记的定义
 * @details 用于标记字符串在编码字节序列中的结束位置
 */
const byte_t OrderedCode::term[] = {0x00, 0x01};

/**
 * @brief 0x00字符的转义标记定义
 * @details 用于在字符串中表示实际的0x00字符，避免与控制字符混淆
 */
const byte_t OrderedCode::lit00[] = {0x00, 0xff};

/**
 * @brief 0xff字符的转义标记定义
 * @details 用于在字符串中表示实际的0xff字符，避免与控制字符混淆
 */
const byte_t OrderedCode::litff[] = {0xff, 0x00};

/**
 * @brief 无穷大标记的定义
 * @details 用于在编码序列中表示无穷大值
 */
const byte_t OrderedCode::inf[] = {0xff, 0xff};

/**
 * @brief 最高有效位掩码数组的定义
 * @details 用于编码整数时确定字节数和符号位
 * 数组元素: 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe
 */
const byte_t OrderedCode::msb[] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};