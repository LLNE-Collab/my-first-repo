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

# 运行目录需要 config / levels（从 build 启动时自动复制）
add_custom_command(TARGET breakout POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/levels ${CMAKE_BINARY_DIR}/levels
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/config.json
    COMMENT "Copy game data files to build directory"
)
if (EXISTS ${CMAKE_SOURCE_DIR}/assets)
    add_custom_command(TARGET breakout POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
        COMMENT "Copy assets to build directory"
    )
endif()

# 测试配置 (用于单元测试)
enable_testing()
add_subdirectory(tests)
