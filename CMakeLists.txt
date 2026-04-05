cmake_minimum_required(VERSION 3.14)  # 升级到3.14以支持FetchContent
project(BreakoutGame)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 raylib
find_package(raylib REQUIRED)  # 移除QUIET，添加REQUIRED

if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(raylib)
endif()

# 添加可执行文件
add_executable(breakout main.c)

# 链接 raylib（使用正确的目标名）
target_link_libraries(breakout PRIVATE Raylib::raylib)  # 使用Raylib::raylib
