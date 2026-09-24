#pragma once

#include <cstdint>
#include <memory>

#include <gbm.h>

#include "job_gbm_format.h"
#include "job_gbm_modifier.h"

namespace job::render {

class JobGbmDevice
{
public:
    using Ptr  = std::shared_ptr<JobGbmDevice>;
    using WPtr = std::weak_ptr<JobGbmDevice>;
    using UPtr = std::unique_ptr<JobGbmDevice>;

    JobGbmDevice() = default;
    ~JobGbmDevice() = default;

    JobGbmDevice(const JobGbmDevice &) = delete;
    JobGbmDevice &operator=(const JobGbmDevice &) = delete;

    JobGbmDevice(JobGbmDevice &&) = delete;
    JobGbmDevice &operator=(JobGbmDevice &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGbmDevice>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGbmDevice>();
    }

    [[nodiscard]] bool create(int drmFd);

    void reset() noexcept
    {
        m_device.reset();
        m_drmFd = -1;
    }

    [[nodiscard]] gbm_device *device() noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] const gbm_device *device() const noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] int drmFd() const noexcept
    {
        return m_drmFd;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_device != nullptr;
    }

    [[nodiscard]] bool supportsFormat(JobGbmFormat format, std::uint32_t usage) const noexcept;

private:
    struct GbmDeviceDeleter
    {
        void operator()(gbm_device *device) const noexcept;
    };

    using GbmDevicePtr = std::unique_ptr<gbm_device, GbmDeviceDeleter>;
    GbmDevicePtr m_device{};
    int m_drmFd{-1};
};

}