#pragma once

#include <vector>
#include <cstdint>
#include <variant>
#include <string>

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include <job_logger.h>

namespace job::serializer {

enum class FieldKind : uint8_t {
    Scalar = 0,
    Bin,
    Struct,
    ListScalar,
    ListBin,
    ListStruct
};

[[nodiscard]] FieldKind deduceFieldKind(const std::string &type) noexcept;

enum class SerializeMode : uint8_t {
    Encode = 0,
    Decode
};

enum class SerializeFormat : uint8_t {
    MsgPack = 0,
    Flatbuffers,
    Yaml,
    Json,
    Binary,
    Text,
    File,
    Unknown = 254
};

// LANG_CPP is only supported but ...."nobody puts Baby in a corner"
enum class SerializeLanguage: uint8_t {
    LANG_CPP = 0,
    LANG_C,
    LANG_JAVA,
    LANG_PYTHON,
    LANG_GO,
    LANG_RUST
};

enum class SerializeLicenseType : uint8_t {
    LIC_MIT =0,
    LIC_APACHE,
    LIC_0BSD,
    LIC_MPL,


    LIC_GPLV2,
    LIC_GPLV2_PLUS,
    LIC_GPLV3,
    LIC_LGPLV3,
    LIC_AGPLV3,


    LIC_CUSTOM = 254
};




struct FieldValue final {
    using Scalar    = std::variant<int32_t, uint32_t, int64_t, uint64_t, std::string>;
    using Binary    = std::vector<uint8_t>;
    using List      = std::vector<FieldValue>;
    using Struct    = std::unordered_map<std::string, FieldValue>;

    std::variant<std::monostate, Scalar, Binary, List, Struct> value;

    [[nodiscard]] bool isNull() const noexcept {
        return std::holds_alternative<std::monostate>(value);
    }

    [[nodiscard]] bool isScalar() const noexcept {
        return std::holds_alternative<Scalar>(value);
    }

    [[nodiscard]] bool isBinary() const noexcept {
        return std::holds_alternative<Binary>(value);
    }

    [[nodiscard]] bool isList() const noexcept {
        return std::holds_alternative<List>(value);
    }

    [[nodiscard]] bool isStruct() const noexcept {
        return std::holds_alternative<Struct>(value);
    }
};

[[nodiscard]] inline nlohmann::json jsonToScalar(const FieldValue::Scalar &scalar) noexcept
{
    auto visitor = [](const auto &value) -> nlohmann::json {
        return value;
    };
    return std::visit(visitor, scalar);
}

} // namespace job::serializer
// CHECKPOINT: v2
