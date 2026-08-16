#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include "jobtoken_export.h"

namespace job::token::utils {

[[nodiscard]] inline std::string readFileToString(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

[[nodiscard]] inline std::string extractTokenString(const nlohmann::json& val)
{
    if (val.is_string()) {
        return val.get<std::string>();
    }
    if (val.is_object()) {
        auto it = val.find("content");
        if (it != val.end() && it->is_string()) {
            return it->get<std::string>();
        }
    }
    return "";
}

} // namespace job::token::formats::utils