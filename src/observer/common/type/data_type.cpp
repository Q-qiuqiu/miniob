/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "common/type/char_type.h"
#include "common/type/float_type.h"
#include "common/type/integer_type.h"
#include "common/type/data_type.h"
#include "common/type/vector_type.h"

/**
 * @brief 初始化 DataType 的静态成员 type_instances_
 * @details 为每种 AttrType 类型创建对应的 DataType 实例
 * 注意：某些类型（如 UNDEFINED、BOOLEANS）使用基类 DataType 的实例，而其他类型则使用特定的子类实例
 */
array<unique_ptr<DataType>, static_cast<int>(AttrType::MAXTYPE)> DataType::type_instances_ = {
    make_unique<DataType>(AttrType::UNDEFINED),  ///< 未定义类型使用基类实例
    make_unique<CharType>(),                     ///< 字符类型使用 CharType 子类实例
    make_unique<IntegerType>(),                  ///< 整数类型使用 IntegerType 子类实例
    make_unique<FloatType>(),                    ///< 浮点数类型使用 FloatType 子类实例
    make_unique<VectorType>(),                   ///< 向量类型使用 VectorType 子类实例
    make_unique<DataType>(AttrType::BOOLEANS)    ///< 布尔类型使用基类实例
};