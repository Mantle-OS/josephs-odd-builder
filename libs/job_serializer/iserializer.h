#pragma once

#include <nlohmann/json.hpp>

#include <yaml-cpp/yaml.h>

#include <job_logger.h>

#include "job_serializer_utils.h"
#include "runtime_object.h"

#include "schema.h"

namespace job::serializer {

class ISerializer {
public:
    ISerializer() = default;
    virtual ~ISerializer() = default;

    ISerializer(const ISerializer &) = delete;
    ISerializer &operator=(const ISerializer &) = delete;

    [[nodiscard]] virtual bool encode(const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer,
                                      SerializeFormat fmt = SerializeFormat::Json) noexcept;

    [[nodiscard]] virtual bool decode( const Schema &schema, RuntimeObject &outObject,
        const std::vector<uint8_t> &inBuffer, SerializeFormat fmt = SerializeFormat::Json) noexcept;

    [[nodiscard]] virtual bool encodeJson( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept;
    [[nodiscard]] virtual bool decodeJson( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept;

    [[nodiscard]] virtual bool encodeYaml( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept;
    [[nodiscard]] virtual bool decodeYaml( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept;

protected:
    [[nodiscard]] virtual bool encodeBinary( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept;
    [[nodiscard]] virtual bool decodeBinary( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept;

    // --- Text (Must be overridden by plugins) ---
    [[nodiscard]] virtual bool encodeText( const Schema &schema, const RuntimeObject &object, std::vector<uint8_t> &outBuffer) noexcept;
    [[nodiscard]] virtual bool decodeText( const Schema &schema, RuntimeObject &outObject, const std::vector<uint8_t> &inBuffer) noexcept;
};
} // namespace job::serializer
// CHECKPOINT: v1
