#pragma once

#include <memory>

#include <gguf.h>

#include "job_ggml_context.h"
#include "jobggml_export.h"

namespace job::ggml {

class JobGgufReader;

class JOBGGML_EXPORT JobGgufInitParams
{
public:
    using Ptr  = std::shared_ptr<JobGgufInitParams>;
    using WPtr = std::weak_ptr<JobGgufInitParams>;
    using UPtr = std::unique_ptr<JobGgufInitParams>;

    explicit JobGgufInitParams(bool noAlloc = true, bool createContext = false) noexcept;
    JobGgufInitParams(bool noAlloc, JobGgmlContext::UPtr *contextOutput) noexcept;
    explicit JobGgufInitParams(const struct gguf_init_params &initParams) noexcept;
    ~JobGgufInitParams() = default;

    [[nodiscard]] static Ptr createShared(bool noAlloc = true, bool createContext = false)
    {
        return std::make_shared<JobGgufInitParams>(noAlloc,createContext);
    }

    [[nodiscard]] static Ptr createShared(bool noAlloc, JobGgmlContext::UPtr *contextOutput)
    {
        return std::make_shared<JobGgufInitParams>(noAlloc, contextOutput);
    }

    [[nodiscard]] static UPtr createUniq(bool noAlloc = true, bool createContext = false)
    {
        return std::make_unique<JobGgufInitParams>(noAlloc, createContext);
    }

    [[nodiscard]] static UPtr createUniq( bool noAlloc, JobGgmlContext::UPtr *contextOutput)
    {
        return std::make_unique<JobGgufInitParams>(noAlloc, contextOutput);
    }

    JobGgufInitParams(const JobGgufInitParams &) = delete;
    JobGgufInitParams &operator=(const JobGgufInitParams &) = delete;
    JobGgufInitParams(JobGgufInitParams &&) = delete;
    JobGgufInitParams &operator=(JobGgufInitParams &&) = delete;

    [[nodiscard]] bool noAlloc() const noexcept;
    void setNoAlloc(bool noAlloc) noexcept;

    [[nodiscard]] bool createContext() const noexcept;
    void setCreateContext(bool createContext) noexcept;

    [[nodiscard]] JobGgmlContext::UPtr *contextOutput() const noexcept;

    void setContextOutput(JobGgmlContext::UPtr *contextOutput) noexcept;

    void setParams(const struct gguf_init_params &initParams) noexcept;
    void resetParams() noexcept;

    [[nodiscard]] static constexpr struct gguf_init_params defaultParams() noexcept
    {
        return { true, nullptr };
    }

    // BACKPORT friend class JobGgufReader;
    [[nodiscard]] struct gguf_init_params params(struct ggml_context **contextOutput) const noexcept;
private:

    struct gguf_init_params     m_params{defaultParams()};
    bool                        m_noAlloc{true};
    bool                        m_createContext{false};
    // Non-owning destination supplied by the caller. The reader transfers a newly created JobGgmlContext::UPtr into this location after parsing.
    JobGgmlContext::UPtr        *m_contextOutput{nullptr};
};

} // namespace job::ggml