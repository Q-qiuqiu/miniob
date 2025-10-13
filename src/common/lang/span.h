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

// 包含 <span> 头文件，该库提供了 std::span 类型。std::span 是 C++20 引入的轻量级视图类型，
// 用于表示连续对象序列的只读或读写视图。它通过存储一个指向数据起始位置的指针和元素数量，
// 可以在不复制数据的情况下安全地引用数组、std::array、std::vector 等容器的一部分。
// 使用 std::span 能避免手动管理指针和长度，减少缓冲区溢出风险，提高代码的通用性和安全性，
// 同时也能让函数接口更加清晰和简洁。
// 举例：以下代码展示了 std::span 的基本用法
// #include <iostream>
// #include <vector>
// #include <span>
// 
// void print_elements(std::span<int> sp) {
//     for (int elem : sp) {
//         std::cout << elem << " ";
//     }
//     std::cout << std::endl;
// }
// 
// int main() {
//     std::vector<int> vec = {1, 2, 3, 4, 5};
//     print_elements(vec); // 直接传入 std::vector
// 
//     int arr[] = {6, 7, 8, 9, 10};
//     print_elements(arr); // 直接传入数组
//     return 0;
// }
#include <span>

using std::span;