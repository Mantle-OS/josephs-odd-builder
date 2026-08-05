#pragma once

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "job_gguf_context.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgufWriter
{
public:
    using Ptr  = std::shared_ptr<JobGgufWriter>;
    using WPtr = std::weak_ptr<JobGgufWriter>;
    using UPtr = std::unique_ptr<JobGgufWriter>;

    explicit JobGgufWriter(JobGgufContext *context);

    ~JobGgufWriter() = default;

    [[nodiscard]] static Ptr createShared(JobGgufContext *context)
    {
        return std::make_shared<JobGgufWriter>(context);
    }

    [[nodiscard]] static UPtr createUniq(JobGgufContext *context)
    {
        return std::make_unique<JobGgufWriter>(context);
    }

    JobGgufWriter(const JobGgufWriter &) = delete;
    JobGgufWriter &operator=(const JobGgufWriter &) = delete;
    JobGgufWriter(JobGgufWriter &&) = delete;
    JobGgufWriter &operator=(JobGgufWriter &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] bool write(const std::filesystem::path &filePath, bool metadataOnly = false);
    [[nodiscard]] bool write(std::FILE *file, bool metadataOnly = false);

    [[nodiscard]] std::size_t metadataSize() const noexcept;
    [[nodiscard]] std::vector<std::byte> metadata() const;
    [[nodiscard]] bool writeMetadata(void *destination, std::size_t destinationSize);
    [[nodiscard]] bool writeMetadata(std::span<std::byte> destination);

    [[nodiscard]] const std::string &errorString() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;

    void clearError() noexcept;

private:
    void setError(const std::string &errorString);

    void setError(std::string &&errorString);

    JobGgufContext *m_context{nullptr}; // Borrowed from JobGguf.

    std::string m_errorString;
};

} // namespace job::ggml

