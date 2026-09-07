#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>

#include <job_file.h>

#include "emitters/emitter.h"
#include "iserializer.h"
#include "job_serializer_utils.h"
#include "jobserializer_export.h"
#include "runtime_object.h"
#include "schema.h"

namespace job::serializer {

class JOBSERIALIZER_EXPORT Writer
{
public:
    using Ptr  = std::shared_ptr<Writer>;
    using WPtr = std::weak_ptr<Writer>;
    using UPtr = std::unique_ptr<Writer>;

    explicit Writer(const std::filesystem::path &path);
    virtual ~Writer();

    Writer(const Writer &) = delete;
    Writer &operator=(const Writer &) = delete;
    Writer(Writer &&) = delete;
    Writer &operator=(Writer &&) = delete;

    [[nodiscard]] static Ptr createShared(const std::filesystem::path &path)
    {
        return std::make_shared<Writer>(path);
    }

    [[nodiscard]] static UPtr createUniq(const std::filesystem::path &path)
    {
        return std::make_unique<Writer>(path);
    }

    [[nodiscard]] bool writeSchema(const Schema &schema,
                                   SerializeFormat mode = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] virtual bool writeEmitter(Emitter &emitter,
                                            const Schema &schema,
                                            const std::filesystem::path &header_file,
                                            const std::filesystem::path &source_file) noexcept;

    [[nodiscard]] bool writeRuntime(ISerializer &ser,
                                    const Schema &schema,
                                    const RuntimeObject &object,
                                    SerializeFormat fmt = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] std::filesystem::path path() const;
    [[nodiscard]] std::string pathString() const;

    void setPath(const std::filesystem::path &path,
                 io::JobFile::Access access,
                 io::JobFile::OpenMode openMode) noexcept;

    ssize_t write(const char *data, size_t size);
    bool flush();
    void closeDevice();

protected:
    [[nodiscard]] virtual bool writeYaml(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeJson(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeText(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeBinary(const Schema &schema) noexcept;

private:
    std::filesystem::path   m_path;
    io::JobFile::UPtr       m_file;         // OWNED
    mutable std::mutex      m_mutex;
};

} // namespace job::serializer