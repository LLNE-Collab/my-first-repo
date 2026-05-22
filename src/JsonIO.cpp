#include "JsonIO.h"

#include <fstream>
#include <raylib.h>

json LoadJSONWithFallback(const std::string& path, const json& fallback) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            TraceLog(LOG_WARNING, "文件不存在: %s，使用默认配置", path.c_str());
            return fallback;
        }
        json data;
        file >> data;
        return data;
    } catch (const json::parse_error& e) {
        TraceLog(LOG_ERROR, "JSON解析失败 (%s): %s", path.c_str(), e.what());
        return fallback;
    } catch (const std::exception& e) {
        TraceLog(LOG_ERROR, "读取JSON失败 (%s): %s", path.c_str(), e.what());
        return fallback;
    }
}

bool SaveFileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}
