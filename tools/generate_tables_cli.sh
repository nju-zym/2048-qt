#!/bin/bash

# 创建构建目录
mkdir -p build_cli
cd build_cli

# 配置CMake
cmake -DCMAKE_BUILD_TYPE=Release -S .. -B .

# 构建生成器
cmake --build . --config Release

# 运行生成器
./generate_tables_cli

echo "查找表生成完成"
