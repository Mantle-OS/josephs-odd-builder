#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "job_ggml_context.h"

#include "job_gguf_context.h"
#include "job_gguf_init_params.h"
#include "job_gguf_reader.h"
#include "job_gguf_writer.h"

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGguf
{
public:
    using Ptr  = std::shared_ptr<JobGguf>;
    using WPtr = std::weak_ptr<JobGguf>;
    using UPtr = std::unique_ptr<JobGguf>;

    explicit JobGguf(JobGgmlContext::UPtr *contextOutput = nullptr);

    ~JobGguf() = default;

    [[nodiscard]] static Ptr createShared(JobGgmlContext::UPtr *contextOutput = nullptr)
    {
        return std::make_shared<JobGguf>(contextOutput);
    }

    [[nodiscard]] static UPtr createUniq(JobGgmlContext::UPtr *contextOutput = nullptr)
    {
        return std::make_unique<JobGguf>(contextOutput);
    }

    JobGguf(const JobGguf &) = delete;
    JobGguf &operator=(const JobGguf &) = delete;
    JobGguf(JobGguf &&) = delete;
    JobGguf &operator=(JobGguf &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool hasContent() const noexcept;

    // Owned subsystem objects
    [[nodiscard]] JobGgufInitParams *initParams() noexcept;
    [[nodiscard]] const JobGgufInitParams *initParams() const noexcept;

    [[nodiscard]] JobGgufContext *context() noexcept;
    [[nodiscard]] const JobGgufContext *context() const noexcept;

    [[nodiscard]] JobGgufReader *reader() noexcept;
    [[nodiscard]] const JobGgufReader *reader() const noexcept;

    [[nodiscard]] JobGgufWriter *writer() noexcept;
    [[nodiscard]] const JobGgufWriter *writer() const noexcept;

    // * No ownership is transferred by this call.
    void setContextOutput(JobGgmlContext::UPtr *contextOutput) noexcept;

    [[nodiscard]] JobGgmlContext::UPtr *contextOutput() const noexcept;

    // Reading
    [[nodiscard]] bool open(const std::filesystem::path &filePath);
    [[nodiscard]] bool open(std::FILE *file);
    [[nodiscard]] bool open(const void *data, std::size_t size);
    [[nodiscard]] bool open(std::span<const std::byte> data);
    [[nodiscard]] bool open(JobGgufReader::ReadCallback callback, std::size_t maxChunkRead, std::uint64_t maxExpectedSize);

    // Writing
    [[nodiscard]] bool save(const std::filesystem::path &filePath, bool metadataOnly = false);
    [[nodiscard]] bool save(std::FILE *file,bool metadataOnly = false);

    [[nodiscard]] std::size_t metadataSize() const noexcept;
    [[nodiscard]] std::vector<std::byte> metadata() const;
    [[nodiscard]] bool writeMetadata(void *destination, std::size_t destinationSize);
    [[nodiscard]] bool writeMetadata(std::span<std::byte> destination);


    [[nodiscard]] std::int64_t readInt(const std::string &key, std::int64_t def = -1) const noexcept
    {
        auto kv = keyValue(key);
        if (!kv)
            return -1;

        return kv->readInt(def);
    }

    [[nodiscard]] std::string readString(const std::string &key, const std::string &def = {}) const
    {
        auto kv = keyValue(key);
        if (!kv)
            return {};

        return kv->readString(def);
    }

    [[nodiscard]] bool readBool(const std::string &key, bool def = false) const noexcept
    {
        auto kv = keyValue(key);
        if (!kv)
            return false;

        return kv->readBool(def);
    }

    [[nodiscard]] float readFloat(const std::string &key, float def = core::safeInfinity()) const noexcept
    {
        auto kv = keyValue(key);
        if (!kv)
            return false;

        return kv->readFloat(def);
    }


    // Common context inspection
    [[nodiscard]] std::uint32_t version() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;
    [[nodiscard]] std::size_t dataOffset() const noexcept;

    [[nodiscard]] std::int64_t keyValueCount() const noexcept;
    [[nodiscard]] std::int64_t tensorCount() const noexcept;

    [[nodiscard]] bool hasKey(const std::string &key) const noexcept;

    [[nodiscard]] bool hasTensor(const std::string &name) const noexcept;
    [[nodiscard]] JobGgufKv::UPtr keyValue(const std::string &key) const;
    [[nodiscard]] JobGgufKv::UPtr keyValue(std::int64_t index) const;
    [[nodiscard]] std::vector<JobGgufKv::UPtr> keyValues() const;


    // Common context mutation
    void setKeyValue(const JobGgufKv &keyValue);
    void setKeyValues(const JobGgufContext &source);
    std::int64_t removeKey(const std::string &key);

    void addTensor(const JobGgmlTensor &tensor);

    void setTensorType(const std::string &name, JobGgmlType type);

    void setTensorData(const std::string &name, const void *data);
    void setTensorData(const std::string &name, const std::vector<std::byte> &data);


    // State
    void reset();

    [[nodiscard]] const std::string &
    errorString() const noexcept;

    [[nodiscard]] bool hasError() const noexcept;

    void clearError() noexcept;

private:
    void adoptReaderError();
    void adoptWriterError();

    void setError(const std::string &errorString);
    void setError(std::string &&errorString);

    JobGgufInitParams::UPtr m_initParams;
    JobGgufContext::UPtr    m_context;

    JobGgufReader::UPtr     m_reader;
    JobGgufWriter::UPtr     m_writer;

    std::string m_errorString;
};

} // namespace job::ggml