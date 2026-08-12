#pragma once

#include <memory>

#include <ggml-backend.h>

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDeviceCaps
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceCaps>;
    using UPtr = std::unique_ptr<JobGgmlDeviceCaps>;

    explicit JobGgmlDeviceCaps(ggml_backend_dev_caps caps);
    ~JobGgmlDeviceCaps() = default;

    [[nodiscard]] static Ptr  createShared(ggml_backend_dev_caps caps)
    {
        return std::make_shared<JobGgmlDeviceCaps>(caps);
    }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_caps caps)
    {
        return std::make_unique<JobGgmlDeviceCaps>(caps);
    }

    JobGgmlDeviceCaps(const JobGgmlDeviceCaps &) = delete;
    JobGgmlDeviceCaps &operator=(const JobGgmlDeviceCaps &) = delete;
    JobGgmlDeviceCaps(JobGgmlDeviceCaps &&) = delete;
    JobGgmlDeviceCaps &operator=(JobGgmlDeviceCaps &&) = delete;

    [[nodiscard]] bool operator==(const JobGgmlDeviceCaps &other) const noexcept;
    [[nodiscard]] bool operator!=(const JobGgmlDeviceCaps &other) const noexcept;

    [[nodiscard]] bool async() const noexcept;
    void setAsync(bool async) noexcept;

    [[nodiscard]] bool hostBuffer() const noexcept;
    void setHostBuffer(bool hostBuffer) noexcept;

    [[nodiscard]] bool bufferFromHostPtr() const noexcept;
    void setBufferFromHostPtr(bool bufferFromHostPtr) noexcept;

    [[nodiscard]] bool events() const noexcept;
    void setEvents(bool events) noexcept;

    void setCaps(ggml_backend_dev_caps other) noexcept;
    [[nodiscard]] ggml_backend_dev_caps caps() noexcept;
    void resetCaps() noexcept;

private:
    [[nodiscard]] static constexpr ggml_backend_dev_caps defaultCaps() noexcept
    {
        return {false, false, false, false};
    }

    ggml_backend_dev_caps m_caps{defaultCaps()};
    bool                  m_async{false};
    bool                  m_hostBuffer{false};
    bool                  m_bufferFromHostPtr{false};
    bool                  m_events{false};
};

} // namespace job::ggml