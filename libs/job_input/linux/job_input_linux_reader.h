#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory>

#include <linux/input.h>

#include "job_input_event.h"
#include "job_input_linux_device.h"

namespace job::input {

class JobInputLinuxReader
{
public:
    using Ptr  = std::shared_ptr<JobInputLinuxReader>;
    using WPtr = std::weak_ptr<JobInputLinuxReader>;
    using UPtr = std::unique_ptr<JobInputLinuxReader>;

    using EventCallback = std::function<void(const JobInputEvent &)>;

    inline static constexpr std::size_t kEventBufferSize = 64;

    explicit JobInputLinuxReader(JobInputLinuxDevice::Ptr device) noexcept;

    ~JobInputLinuxReader() = default;

    JobInputLinuxReader(const JobInputLinuxReader &) = delete;
    JobInputLinuxReader &operator=(const JobInputLinuxReader &) = delete;

    JobInputLinuxReader(JobInputLinuxReader &&) noexcept = default;
    JobInputLinuxReader &operator=(JobInputLinuxReader &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(JobInputLinuxDevice::Ptr device)
    {
        return std::make_shared<JobInputLinuxReader>(std::move(device));
    }

    [[nodiscard]] static UPtr createUniq(JobInputLinuxDevice::Ptr device)
    {
        return std::make_unique<JobInputLinuxReader>(std::move(device));
    }

    [[nodiscard]] bool readEvents();

    void setEventCallback(EventCallback callback)
    {
        m_eventCallback = std::move(callback);
    }

    [[nodiscard]] JobInputLinuxDevice::Ptr device() const noexcept
    {
        return m_device.lock();
    }

private:
    [[nodiscard]] static constexpr JobInputEvent makeEvent(const input_event &event) noexcept
    {
        return JobInputEvent(JobTimeValue(event), static_cast<JobInputType>(event.type), static_cast<JobInputCode>(event.code), static_cast<JobInputValue>(event.value));
    }

    JobInputLinuxDevice::WPtr   m_device;
    EventCallback               m_eventCallback;
    std::array<input_event,     kEventBufferSize> m_events{};
};

} // namespace job::input