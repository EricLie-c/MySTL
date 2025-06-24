# 设置CMake最低版本
cmake_minimum_required(VERSION 3.14)

# 定义项目名称和语言
project(MySTL CXX)

# 设置标准为C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定编译器为clang
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

# 添加include目录
target_include_directories(MySTL INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 集成google test
enable_testing()

include(FetchContent)

FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip
)

# 指定 GTest 不要成为 "all" 构建目标的一部分
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# 添加 tests 子目录
add_subdirectory(tests)
