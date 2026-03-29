cmake_minimum_required(VERSION 3.10)
project(BreakoutGame)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找 raylib
find_package(raylib QUIET)

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

# 链接 raylib
target_link_libraries(breakout raylib)
