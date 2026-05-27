#pragma once

/**
 * @file JsonIO.h
 * @brief JSON 文件读写与防御性加载。
 *
 * 是什么：对 nlohmann::json 的薄封装，统一处理「文件不存在」与「解析失败」。
 * 为什么：关卡、配置、存档均来自 JSON；课程要求错误时回退默认数据而非崩溃。
 * 怎么用：调用 LoadJSONWithFallback(path, fallback)，失败时返回 fallback；
 *        用 SaveFileExists(path) 判断存档是否存在（如主菜单 CONTINUE）。
 */

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

/// 读取 JSON；打不开或解析失败时 TraceLog 并返回 fallback。
json LoadJSONWithFallback(const std::string& path, const json& fallback);

/// 判断路径对应文件是否可读（用于存档、关卡计数）。
bool SaveFileExists(const std::string& path);
