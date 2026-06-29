#pragma once
#include <memory>
#include <string>
#include <vector>

#include <ggml-backend.h>

#include "job_ggml_device.h"

namespace job::ggml {

enum class ManagerState : uint8_t {
    Uninitialized   = 0,
    Scanning        = 1,
    Ready           = 2,
    Error           = 255
};

class JobGgmlDeviceManager {
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceManager>;
    using UPtr = std::unique_ptr<JobGgmlDeviceManager>;
    explicit JobGgmlDeviceManager(bool autoscan = false) {
        if(autoscan)
            scan();
    }

    ~JobGgmlDeviceManager();
    JobGgmlDeviceManager(JobGgmlDeviceManager&&) = delete;
    JobGgmlDeviceManager& operator=(JobGgmlDeviceManager&&) = delete;

    void scan();
    ManagerState state() const noexcept;

    JobGgmlDevice *cpuDevice() noexcept;
    JobGgmlDevice *cpuDevice() const noexcept;

    std::vector<JobGgmlDevice *> gpuDevices() noexcept;
    std::vector<JobGgmlDevice *> gpuDevices() const noexcept;


    JobGgmlDevice *deviceByName(const std::string &name) noexcept;
    bool hasGpu() const noexcept;

    ggml_backend_sched_t scheduler() noexcept;
    void buildScheduler();
    void resetScheduler();


    JobGgmlDeviceManager(const JobGgmlDeviceManager &) = delete;
    JobGgmlDeviceManager &operator=(const JobGgmlDeviceManager &) = delete;

    std::string debugStr() const;
private:

    ManagerState                                    m_state{ManagerState::Uninitialized};
    std::vector<JobGgmlDevice::UPtr>                m_devices;
    ggml_backend_sched_t                            m_sched{nullptr};
    mutable std::mutex                              m_mutex;
};
}