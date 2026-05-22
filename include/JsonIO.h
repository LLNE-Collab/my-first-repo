#pragma once

#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

json LoadJSONWithFallback(const std::string& path, const json& fallback);
bool SaveFileExists(const std::string& path);
