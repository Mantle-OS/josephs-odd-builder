#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

// #include <file_io.h>

#include "schema.h"
#include "runtime_object.h"
#include "iserializer.h"
#include "emitters/emitter.h"

namespace job::serializer {

class Reader {
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

    std::string pathString() const;
    std::filesystem::path path() const;

    [[nodiscard]] std::string readAll() noexcept;
    [[nodiscard]] bool readAll(std::vector<uint8_t>& out_buf) noexcept;
protected:
    [[nodiscard]] virtual bool readYaml(Schema &out_schema) noexcept;
    [[nodiscard]] virtual bool readJson(Schema &out_schema) noexcept;
    [[nodiscard]] virtual std::string readText(Schema &in_schema);
    ssize_t read(char *buffer, size_t size);

private:
    Schema m_lastRead;
    std::filesystem::path m_path;
    std::ifstream m_input;
    std::ofstream m_output;
    mutable std::mutex m_mutex;
    bool m_open = false;
    void closeDevice();
};

} // namespace job::serializer

