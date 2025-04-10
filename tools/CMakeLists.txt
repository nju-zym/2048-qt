cmake_minimum_required(VERSION 3.16)

project(BitBoardTableGenerator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    add_compile_definitions(USE_OPENMP)
endif()

# 启用优化
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native -mtune=native")
elseif(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /arch:AVX2")
endif()

# 添加可执行文件
add_executable(generate_tables generate_tables.cpp)

# 链接OpenMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(generate_tables PRIVATE OpenMP::OpenMP_CXX)
endif()

# 添加自定义命令，在构建后运行生成器
add_custom_command(
    TARGET generate_tables
    POST_BUILD
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/generate_tables
    COMMENT "Generating BitBoard lookup tables..."
)

# 添加自定义目标，将生成的文件复制到项目目录
add_custom_target(
    copy_tables
    DEPENDS generate_tables
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/BitBoardTables.cpp ${CMAKE_SOURCE_DIR}/../ai/
    COMMENT "Copying BitBoardTables.cpp to project directory..."
)
