#!/bin/bash

# 编译生成查找表的工具
g++ -O3 -std=c++17 -o generate_tables_simple generate_tables_simple.cpp

# 运行工具生成查找表
./generate_tables_simple

echo "查找表生成完成，已保存到 ../ai/BitBoardTables.cpp"
