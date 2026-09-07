#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include "job_serializer_utils.h"
#include "jobserializer_export.h"
#include "runtime_object.h"
#include "schema.h"

namespace job::serializer {

class JOBSERIALIZER_EXPORT ISerializer
{
public:
    using Ptr  = std::shared_ptr<ISerializer>;
    using WPtr = std::weak_ptr<ISerializer>;
    using UPtr = std::unique_ptr<ISerializer>;

    ISerializer() = default;
    virtual ~ISerializer() = default;

    ISerializer(const ISerializer &) = delete;
    ISerializer &operator=(const ISerializer &) = delete;
    ISerializer(ISerializer &&) noexcept = default;
    ISerializer &operator=(ISerializer &&) noexcept = default;

    [[nodiscard]] virtual bool encode(const Schema &schema,
                                      const RuntimeObject &object,
                                      std::vector<uint8_t> &outBuffer,
                                      SerializeFormat fmt = SerializeFormat::Json) noexcept;

    [[nodiscard]] virtual bool decode(const Schema &schema,
                                      RuntimeObject &outObject,
                                      const std::vector<uint8_t> &inBuffer,
                                      SerializeFormat fmt = SerializeFormat::Json) noexcept;

    [[nodiscard]] virtual bool encodeJson(const Schema &schema,
                                          const RuntimeObject &object,
                                          std::vector<uint8_t> &outBuffer) noexcept;

    [[nodiscard]] virtual bool decodeJson(const Schema &schema,
                                          RuntimeObject &outObject,
                                          const std::vector<uint8_t> &inBuffer) noexcept;

    [[nodiscard]] virtual bool encodeYaml(const Schema &schema,
                                          const RuntimeObject &object,
                                          std::vector<uint8_t> &outBuffer) noexcept;

    [[nodiscard]] virtual bool decodeYaml(const Schema &schema,
                                          RuntimeObject &outObject,
                                          const std::vector<uint8_t> &inBuffer) noexcept;

protected:
    [[nodiscard]] virtual bool encodeBinary(const Schema &schema,
                                            const RuntimeObject &object,
                                            std::vector<uint8_t> &outBuffer) noexcept;

    [[nodiscard]] virtual bool decodeBinary(const Schema &schema,
                                            RuntimeObject &outObject,
                                            const std::vector<uint8_t> &inBuffer) noexcept;

    [[nodiscard]] virtual bool encodeText(const Schema &schema,
                                          const RuntimeObject &object,
                                          std::vector<uint8_t> &outBuffer) noexcept;

    [[nodiscard]] virtual bool decodeText(const Schema &schema,
                                          RuntimeObject &outObject,
                                          const std::vector<uint8_t> &inBuffer) noexcept;
};

} // namespace job::serializer