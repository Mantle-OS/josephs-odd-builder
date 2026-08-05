#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>

#include <gguf.h>

#include "job_gguf_context.h"
#include "job_gguf_init_params.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgufReader
{
public:
    using Ptr  = std::shared_ptr<JobGgufReader>;
    using WPtr = std::weak_ptr<JobGgufReader>;
    using UPtr = std::unique_ptr<JobGgufReader>;

    using ReadCallback = std::function<std::size_t(
        void *output,
        std::uint64_t offset,
        std::size_t length
        )>;

    explicit JobGgufReader(JobGgufContext *context = nullptr);
    ~JobGgufReader() = default;

    [[nodiscard]] static Ptr createShared(JobGgufContext *context)
    {
        return std::make_shared<JobGgufReader>(context);
    }

    [[nodiscard]] static UPtr createUniq(JobGgufContext *context)
    {
        return std::make_unique<JobGgufReader>(context);
    }

    JobGgufReader(const JobGgufReader &) = delete;
    JobGgufReader &operator=(const JobGgufReader &) = delete;
    JobGgufReader(JobGgufReader &&) = delete;
    JobGgufReader &operator=(JobGgufReader &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] bool read(const std::filesystem::path &filePath, const JobGgufInitParams &initParams);
    [[nodiscard]] bool read(std::FILE *file, const JobGgufInitParams &initParams);
    [[nodiscard]] bool read(const void *data, std::size_t size, const JobGgufInitParams &initParams);
    [[nodiscard]] bool read(std::span<const std::byte> data, const JobGgufInitParams &initParams);
    [[nodiscard]] bool read(ReadCallback callback, std::size_t maxChunkRead,std::uint64_t maxExpectedSize, const JobGgufInitParams &initParams);

    [[nodiscard]] const std::string &errorString() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    void clearError() noexcept;

private:
    struct CallbackState
    {
        ReadCallback *callback{nullptr};
        std::string  *errorString{nullptr};
    };

    [[nodiscard]] static std::size_t callbackTrampoline( void *userData, void *output, std::uint64_t offset, std::size_t length ) noexcept;
    [[nodiscard]] bool finishRead(struct gguf_context *nativeGgufContext, struct ggml_context *nativeGgmlContext, const JobGgufInitParams &initParams);

    void setError(const std::string &errorString);
    void setError(std::string &&errorString);

    JobGgufContext  *m_context{nullptr}; // Borrowed from JobGguf.
    std::string     m_errorString;
};

} // namespace job::ggml