cmake_minimum_required(VERSION 3.14)
project(BreakoutGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 引入 Raylib
find_package(raylib QUIET)
if (NOT raylib_FOUND)
    include(FetchContent)
    FetchContent_Declare(raylib GIT_REPOSITORY https://github.com/raysan5/raylib.git GIT_TAG master)
    FetchContent_MakeAvailable(raylib)
endif()

# 引入 nlohmann/json 用于配置文件
include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

# 游戏源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(breakout ${SOURCES})

target_include_directories(breakout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(breakout PRIVATE raylib nlohmann_json::nlohmann_json)

# 测试配置 (用于单元测试)
enable_testing()
add_subdirectory(tests)
