#pragma once

#include <cstdint>
#include <optional>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include <job_logger.h>

#include "job_serializer_utils.h"

namespace job::serializer {

struct Field final {
    uint32_t key = 0;
    std::string name;
    std::string type;
    FieldKind kind = FieldKind::Scalar;

    std::optional<int> size;
    std::optional<std::string> ctype;
    std::optional<std::string> ref_include;
    std::optional<std::string> ref_sym;

    bool required = false;
    std::optional<std::string> comment;

    static std::optional<int> parseBinSize(const std::string &type) noexcept;
    static std::optional<int> parseListBinSize(const std::string &type) noexcept;

    static bool from_yaml(const YAML::Node &n, Field &out) noexcept;
    static bool from_yaml(const YAML::Node &node, std::vector<Field> &fields) noexcept;
    static void to_yaml(YAML::Emitter &emitter, const Field &f) noexcept;

    static void to_json(nlohmann::json &j, const Field &f);
    static void from_json(const nlohmann::json &j, Field &f);

    void dump() const noexcept;
    [[nodiscard]] static bool isValid(std::vector<Field> fields) noexcept;
    [[nodiscard]] bool operator==(const Field &b) const noexcept = default;


};

} // job::serializer

namespace nlohmann {
using json = nlohmann::json;
template <>
struct adl_serializer<job::serializer::Field> {
    static void to_json(json &j, const job::serializer::Field &f) {
        job::serializer::Field::to_json(j, f);
    }
    static void from_json(const json &j, job::serializer::Field &f) {
        job::serializer::Field::from_json(j, f);
    }
};
};
