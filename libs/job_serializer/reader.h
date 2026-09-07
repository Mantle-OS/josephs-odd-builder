#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <job_file.h>

#include "emitters/emitter.h"
#include "iserializer.h"
#include "jobserializer_export.h"
#include "runtime_object.h"
#include "schema.h"

namespace job::serializer {

class JOBSERIALIZER_EXPORT Reader
{
public:
    using Ptr  = std::shared_ptr<Reader>;
    using WPtr = std::weak_ptr<Reader>;
    using UPtr = std::unique_ptr<Reader>;

    explicit Reader(const std::filesystem::path &path);
    virtual ~Reader();

    Reader(const Reader &) = delete;
    Reader &operator=(const Reader &) = delete;
    Reader(Reader &&) = delete;
    Reader &operator=(Reader &&) = delete;

    [[nodiscard]] static Ptr createShared(const std::filesystem::path &path)
    {
        return std::make_shared<Reader>(path);
    }

    [[nodiscard]] static UPtr createUniq(const std::filesystem::path &path)
    {
        return std::make_unique<Reader>(path);
    }

    [[nodiscard]] bool readSchema(Schema &out_schema,
                                  SerializeFormat mode = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] virtual bool readEmitter(const Emitter &in_emitter,
                                           Schema &out_schema) noexcept;

    [[nodiscard]] bool readRuntime(ISerializer &ser,
                                   const Schema &out_schema,
                                   RuntimeObject &object,
                                   SerializeFormat fmt) noexcept;

    [[nodiscard]] std::string pathString() const;
    [[nodiscard]] std::filesystem::path path() const;

    [[nodiscard]] std::string readAll() noexcept;
    [[nodiscard]] bool readAll(std::vector<uint8_t> &out_buf) noexcept;

protected:
    [[nodiscard]] virtual bool readYaml(Schema &out_schema) noexcept;
    [[nodiscard]] virtual bool readJson(Schema &out_schema) noexcept;
    [[nodiscard]] virtual std::string readText(Schema &in_schema);

    ssize_t read(char *buffer, size_t size);

private:
    Schema                  m_lastRead;
    std::filesystem::path   m_path;
    io::JobFile::UPtr       m_file; // OWNED
    mutable std::mutex      m_mutex;
};

} // namespace job::serializer