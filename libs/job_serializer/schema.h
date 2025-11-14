#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <compare>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include "job_field.h"

namespace job::serializer {

struct Schema final {
    std::string tag;
    int version = 0;
    std::string unit;
    std::string base;
    std::string c_struct;
    std::string include_prefix;
    std::string out_base;
    std::vector<Field> fields;

    std::filesystem::path hdr_name;
    std::filesystem::path src_name;
    std::filesystem::path schema_path;

    // JSON
    [[nodiscard]] static bool parse(const nlohmann::json &root, Schema &out);
    static void to_json(nlohmann::json &j, const Schema &s);
    static void from_json(const nlohmann::json &j, Schema &s);

    // YAML
    [[nodiscard]] static bool parse(const YAML::Node &root, Schema &out) noexcept;
    static void to_yaml(YAML::Emitter &emitter, const Schema &s) noexcept;

    // UTILS
    void dump() noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool operator==(const Schema &other) const noexcept = default;
};

} // namespace job::serializer


namespace nlohmann {
using json = nlohmann::json;
template <>
struct adl_serializer<job::serializer::Schema> {
    static void to_json(json &j, const job::serializer::Schema &s) {
        job::serializer::Schema::to_json(j, s);
    }
    static void from_json(const json &j, job::serializer::Schema &s) {
        job::serializer::Schema::from_json(j, s);
    }
};

} // namespace nlohmann
// CHECKPOINT: v1
