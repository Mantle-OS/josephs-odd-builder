#pragma once

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include <job_logger.h>
#include <file_io.h>

#include "schema.h"
#include "runtime_object.h"
#include "iserializer.h"
#include "emitters/emitter.h"

namespace job::serializer {

class Reader : public io::FileIO {
public:
    explicit Reader(const std::filesystem::path &path);
    virtual ~Reader();

    [[nodiscard]] bool readSchema(Schema &out_schema,
                                   SerializeFormat mode = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] virtual bool readEmitter(const Emitter &in_emitter,
                                           Schema &out_schema) noexcept;

    [[nodiscard]] bool readRuntime(ISerializer &ser,
                                   const Schema &out_schema,
                                   RuntimeObject &object, SerializeFormat fmt) noexcept;
protected:
    [[nodiscard]] virtual bool readYaml(Schema &out_schema) noexcept;
    [[nodiscard]] virtual bool readJson(Schema &out_schema) noexcept;
    [[nodiscard]] virtual std::string readText(Schema &in_schema);

    // [[nodiscard]] virtual std::vector<unit8_t> readBinary(const Schema &in_schema) noexcept;

private:
    Schema m_lastRead;
};

} // namespace job::serializer

