#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <xf86drmMode.h>

#include "job_drm_atomic_state.h"

namespace job::render {

class JobDrmAtomicRequest
{
public:
    using Ptr  = std::shared_ptr<JobDrmAtomicRequest>;
    using WPtr = std::weak_ptr<JobDrmAtomicRequest>;
    using UPtr = std::unique_ptr<JobDrmAtomicRequest>;

    using Flags    = std::uint32_t;
    using UserData = void *;

    JobDrmAtomicRequest() = default;
    ~JobDrmAtomicRequest() = default;

    JobDrmAtomicRequest(const JobDrmAtomicRequest &) = delete;
    JobDrmAtomicRequest &operator=(const JobDrmAtomicRequest &) = delete;

    JobDrmAtomicRequest(JobDrmAtomicRequest &&) = delete;
    JobDrmAtomicRequest &operator=(JobDrmAtomicRequest &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmAtomicRequest>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmAtomicRequest>();
    }

    [[nodiscard]] bool build(const JobDrmAtomicState &state);

    [[nodiscard]] bool add(JobDrmAtomicState::ObjectId objectId,
                           JobDrmAtomicState::PropertyId propertyId,
                           JobDrmAtomicState::Value value);

    void clear() noexcept;
    [[nodiscard]] bool commit(int fd, Flags flags = 0, UserData userData = nullptr) noexcept;
    [[nodiscard]] bool test(int fd, Flags flags = 0) noexcept;

    [[nodiscard]] drmModeAtomicReq *request() noexcept
    {
        return m_request.get();
    }

    [[nodiscard]] const drmModeAtomicReq *request() const noexcept
    {
        return m_request.get();
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_request != nullptr;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_propertyCount == 0;
    }

    [[nodiscard]] std::size_t propertyCount() const noexcept
    {
        return m_propertyCount;
    }

private:
    struct DrmModeAtomicReqDeleter
    {
        void operator()(drmModeAtomicReq *request) const noexcept;
    };
    using DrmModeAtomicReqPtr = std::unique_ptr<drmModeAtomicReq, DrmModeAtomicReqDeleter>;

    DrmModeAtomicReqPtr m_request{};
    std::size_t         m_propertyCount{};
};

}