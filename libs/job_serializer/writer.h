#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>

#include "job_serializer_utils.h"
#include "schema.h"
#include "emitters/emitter.h"
#include "runtime_object.h"
#include "iserializer.h"

namespace job::serializer {

class Writer {
public:
    explicit Writer(const std::filesystem::path &path);
    ~Writer()
    {
        if(m_open)
            closeDevice();
    }

    enum class WriteType{
        Truncate,
        Append,
        ReadOnly
    };

    [[nodiscard]] bool writeSchema(const Schema &schema,
                                   SerializeFormat mode = SerializeFormat::Unknown) noexcept;

    [[nodiscard]] virtual bool writeEmitter(Emitter &emitter,
                                    const Schema &schema,
                                    const std::filesystem::path &header_file,
                                    const std::filesystem::path &source_file) noexcept;

    [[nodiscard]] bool writeRuntime(ISerializer &ser, const Schema &schema,
                                    const RuntimeObject &object,
                                    SerializeFormat fmt = SerializeFormat::Unknown) noexcept;

    std::filesystem::path path();
    void setPath(const std::filesystem::path &path, WriteType openType) noexcept;
    std::string pathString() const;

    ssize_t write(const char *data, size_t size);
    bool flush();
    void closeDevice();

protected:
    [[nodiscard]] virtual bool writeYaml(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeJson(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeText(const Schema &schema) noexcept;
    [[nodiscard]] virtual bool writeBinary(const Schema &schema) noexcept;

private:
    std::filesystem::path m_path;
    std::ifstream m_input;
    std::ofstream m_output;
    mutable std::mutex m_mutex;

    bool m_open = false;
    bool m_writeMode = false;

};

} // namespace job::serializer

