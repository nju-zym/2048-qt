#!/bin/bash

# 创建构建目录
mkdir -p build
cd build

# 配置CMake
cmake ..

# 构建生成器
cmake --build . --config Release

# 运行生成器
./generate_tables

# 复制生成的文件到项目目录
cmake --build . --target copy_tables

echo "查找表生成完成，已复制到项目目录"
