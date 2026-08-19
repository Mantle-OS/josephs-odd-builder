#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <job_ggml_context.h>
#include <job_ggml_backend_buffer.h>
#include <job_ggml_tensor.h>

namespace job::model {

class JobMemBuf
{
public:
    // all three required
    using Ptr  = std::shared_ptr<JobMemBuf>;
    using WPtr = std::weak_ptr<JobMemBuf>;
    using UPtr = std::unique_ptr<JobMemBuf>;

    // fab 5 required
    JobMemBuf() = default;
    ~JobMemBuf() = default;

    JobMemBuf(const JobMemBuf &) = delete;
    JobMemBuf &operator=(const JobMemBuf &) = delete;
    JobMemBuf(JobMemBuf &&) noexcept = default;
    JobMemBuf &operator=(JobMemBuf &&) noexcept = default;

    //  required
    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobMemBuf>();
    }

    //  required
    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobMemBuf>();
    }

    [[nodiscard]] int nTensors() const noexcept
    {
        return m_nTensors;
    }

    [[nodiscard]] std::size_t totalSize() const noexcept
    {
        return m_totalSize;
    }

    [[nodiscard]] ggml::JobGgmlBackendBuffer *buf() noexcept
    {
        return m_buf.get();
    }

    [[nodiscard]] const ggml::JobGgmlBackendBuffer *buf() const noexcept
    {
        return m_buf.get();
    }

    [[nodiscard]] ggml::JobGgmlContext *ctx() noexcept
    {
        return m_ctx.get();
    }

    [[nodiscard]] const ggml::JobGgmlContext *ctx() const noexcept
    {
        return m_ctx.get();
    }

    [[nodiscard]] std::vector<ggml::JobGgmlTensor *> &orignal() noexcept
    {
        return m_orignal;
    }

    [[nodiscard]] const std::vector<ggml::JobGgmlTensor *> &orignal() const noexcept
    {
        return m_orignal;
    }

    [[nodiscard]] std::vector<ggml::JobGgmlTensor *> &copy() noexcept
    {
        return m_copy;
    }

    [[nodiscard]] const std::vector<ggml::JobGgmlTensor *> &copy() const noexcept
    {
        return m_copy;
    }

private:
    int                                 m_nTensors{0};
    std::size_t                         m_totalSize{0};

    ggml::JobGgmlBackendBuffer::UPtr    m_buf;
    ggml::JobGgmlContext::UPtr          m_ctx;

    std::vector<ggml::JobGgmlTensor *>  m_orignal;
    std::vector<ggml::JobGgmlTensor *>  m_copy;
};

} // namespace job::model