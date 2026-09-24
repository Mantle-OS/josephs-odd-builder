#pragma once

#include <memory>
#include <string>

#include "fb/job_fb_buffer.h"
#include "fb/job_fb_device.h"
#include "fb/job_fb_presenter.h"

namespace job::render {

class JobFbBackend
{
public:
    using Ptr  = std::shared_ptr<JobFbBackend>;
    using WPtr = std::weak_ptr<JobFbBackend>;
    using UPtr = std::unique_ptr<JobFbBackend>;

    JobFbBackend() = default;
    ~JobFbBackend();

    JobFbBackend(const JobFbBackend &) = delete;
    JobFbBackend &operator=(const JobFbBackend &) = delete;

    JobFbBackend(JobFbBackend &&) noexcept = delete;
    JobFbBackend &operator=(JobFbBackend &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFbBackend>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFbBackend>();
    }

    [[nodiscard]] bool start(const std::string &device = {});
    void stop() noexcept;

    [[nodiscard]] bool refresh();

    [[nodiscard]] JobFbDevice *device() noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] const JobFbDevice *device() const noexcept
    {
        return m_device.get();
    }

    [[nodiscard]] JobFbPresenter *presenter() noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] const JobFbPresenter *presenter() const noexcept
    {
        return m_presenter.get();
    }

    [[nodiscard]] JobFbBuffer framebuffer() noexcept;

    [[nodiscard]] bool running() const noexcept
    {
        return m_device && m_presenter && m_device->valid() && m_presenter->valid();
    }

private:
    JobFbDevice::Ptr        m_device;
    JobFbPresenter::Ptr     m_presenter;
};

}