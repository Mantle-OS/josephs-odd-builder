#pragma once

#include <cstdint>
#include <memory>

#include "job_fb_buffer.h"
#include "job_fb_device.h"

namespace job::render {

class JobFbPresenter
{
public:
    using Ptr  = std::shared_ptr<JobFbPresenter>;
    using WPtr = std::weak_ptr<JobFbPresenter>;
    using UPtr = std::unique_ptr<JobFbPresenter>;

    JobFbPresenter() = default;
    explicit JobFbPresenter(JobFbDevice *device) noexcept;

    ~JobFbPresenter() = default;

    JobFbPresenter(const JobFbPresenter &) = delete;
    JobFbPresenter &operator=(const JobFbPresenter &) = delete;

    JobFbPresenter(JobFbPresenter &&) noexcept = delete;
    JobFbPresenter &operator=(JobFbPresenter &&) noexcept = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobFbPresenter>();
    }

    [[nodiscard]] static Ptr createShared(JobFbDevice *device)
    {
        return std::make_shared<JobFbPresenter>(device);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobFbPresenter>();
    }

    [[nodiscard]] static UPtr createUniq(JobFbDevice *device)
    {
        return std::make_unique<JobFbPresenter>(device);
    }

    [[nodiscard]] bool initialize(JobFbDevice *device) noexcept;
    void reset() noexcept;

    [[nodiscard]] JobFbDevice *device() noexcept
    {
        return m_device;
    }

    [[nodiscard]] const JobFbDevice *device() const noexcept
    {
        return m_device;
    }

    [[nodiscard]] JobFbBuffer framebuffer() noexcept;
    [[nodiscard]] bool refresh() noexcept;
    [[nodiscard]] bool valid() const noexcept
    {
        return m_device != nullptr && m_device->valid() && m_device->mapped();
    }

    // ABSTRACT TODO MAYBE need to finish KMS first
    // present()
    // swapBuffers()
    // copy()
    // blit()
    // pan()
    // waitForVSync()
    // damage()



private:
    JobFbDevice *m_device{}; // BORROWED
};

}