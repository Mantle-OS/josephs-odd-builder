#include "job_ggml_device_manager.h"

#include <job_logger.h>

#include <sstream>

namespace job::ggml {

// I guess applicaiton land should control singletons not the lib.
// JobGgmlDeviceManager &JobGgmlDeviceManager::instance()
// {
//     static JobGgmlDeviceManager instance;

//     return instance;
// }

JobGgmlDeviceManager::~JobGgmlDeviceManager()
{
    if (m_sched) {
        ggml_backend_sched_free(m_sched);
        m_sched = nullptr;
    }

    for (auto &dev : m_devices)
        dev = nullptr;
    m_devices.clear();

}

void JobGgmlDeviceManager::scan()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != ManagerState::Uninitialized) {
        // JOB_LOG_WARN("[JobGgmlDeviceManager] Already scanned, state is {}", static_cast<int>(m_state));
        return;
    }

    m_state = ManagerState::Scanning;

    // Load dynamic backends (CPU, CUDA, Vulkan, etc.) — GGML_BACKEND_DL loads them at runtime
    ggml_backend_load_all();

    size_t totalDevices = 0;
    size_t regCount = ggml_backend_reg_count();

    for (size_t r = 0; r < regCount; ++r) {
        auto *reg = ggml_backend_reg_get(r);
        if (!reg)
            continue;

        const char *regName = ggml_backend_reg_name(reg);
        size_t devCount = ggml_backend_reg_dev_count(reg);
        JOB_LOG_DEBUG("[JobGgmlDeviceManager] Registry '{}' has {} device(s)", regName, devCount);

        for (size_t i = 0; i < devCount; ++i) {
            auto *dev = ggml_backend_reg_dev_get(reg, i);
            if (!dev)
                continue;

            // const char *devName = ggml_backend_dev_name(dev);
            std::string key = std::string(regName) + ":" + std::to_string(i);

            // auto ndev = JobGgmlDevice::mkUniq(dev);
            // std::string key = ndev->name();
            m_devices.push_back(JobGgmlDevice::mkUniq(dev));

            ++totalDevices;
        }
    }

    m_state = ManagerState::Ready;
    JOB_LOG_DEBUG("[JobGgmlDeviceManager] Scan complete: {} device(s) registered", totalDevices);

    if (totalDevices > 0)
        buildScheduler();

    JOB_LOG_DEBUG("ggml backend registry count = {}", ggml_backend_reg_count());
}

ManagerState JobGgmlDeviceManager::state() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

JobGgmlDevice *JobGgmlDeviceManager::cpuDevice() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &dev : m_devices) {
        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_CPU)
            return dev.get();
    }
    return nullptr;
}

JobGgmlDevice *JobGgmlDeviceManager::cpuDevice() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &dev : m_devices) {
        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_CPU)
            return dev.get();
    }
    return nullptr;
}

std::vector<JobGgmlDevice*> JobGgmlDeviceManager::gpuDevices() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JobGgmlDevice*> gpus;
    for (auto &dev : m_devices) {
        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_GPU ||
            dev->type() == GGML_BACKEND_DEVICE_TYPE_IGPU) {
            gpus.push_back(dev.get());
        }
    }
    return gpus;
}


std::vector<JobGgmlDevice *> JobGgmlDeviceManager::gpuDevices() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<JobGgmlDevice *> gpus;
    for (const auto &dev : m_devices) {
        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_GPU ||
            dev->type() == GGML_BACKEND_DEVICE_TYPE_IGPU) {
            gpus.push_back(dev.get());
        }
    }
    return gpus;

}

JobGgmlDevice *JobGgmlDeviceManager::deviceByName(const std::string &name) noexcept
{
    JobGgmlDevice *ret = nullptr;
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &dev : m_devices)
        if(name == dev->name())
            return dev.get();

    JOB_LOG_DEBUG("No Device with that name");
    return ret;
}

bool JobGgmlDeviceManager::hasGpu() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto &dev : m_devices) {
        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_GPU ||
            dev->type() == GGML_BACKEND_DEVICE_TYPE_IGPU) {
            return true;
        }
    }
    return false;
}

ggml_backend_sched_t JobGgmlDeviceManager::scheduler() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sched;
}

void JobGgmlDeviceManager::buildScheduler()
{
    // Must be called with m_mutex held (from scan())
    // std::lock_guard<std::mutex> lock(m_mutex);

    if (m_devices.empty())
        return;


    std::vector<ggml_backend_t> gpuBackends;
    ggml_backend_t cpuBackend = nullptr;

    for (auto &dev : m_devices) {
        auto b = dev->backend();
        if (!b)
            continue;

        if (dev->type() == GGML_BACKEND_DEVICE_TYPE_CPU)
            cpuBackend = b;
        else
            gpuBackends.push_back(b);

    }

    // IMPORTANT: deterministic order
    std::sort(gpuBackends.begin(), gpuBackends.end(),
              [](auto a, auto b) {
                  return ggml_backend_dev_type(ggml_backend_get_device(a))
                  < ggml_backend_dev_type(ggml_backend_get_device(b));
              });

    std::vector<ggml_backend_t> backends;

    // GPUs first
    backends.insert(backends.end(), gpuBackends.begin(), gpuBackends.end());

    // CPU LAST (critical)
    if (cpuBackend)
        backends.push_back(cpuBackend);

    if (backends.empty()) {
        JOB_LOG_WARN("[JobGgmlDeviceManager] No initialized backends to schedule");
        return;
    }

    if (m_sched) {
        ggml_backend_sched_free(m_sched);
        m_sched = nullptr;
    }

    m_sched = ggml_backend_sched_new(
        backends.data(),
        nullptr,
        (int)backends.size(),
        GGML_DEFAULT_GRAPH_SIZE,
        true,
        true
    );

    if (!m_sched) {
        JOB_LOG_ERROR("[JobGgmlDeviceManager] Failed to create backend scheduler");
        return;
    }

    JOB_LOG_DEBUG("[JobGgmlDeviceManager] Scheduler created with {} backend(s)", backends.size());
}

void JobGgmlDeviceManager::resetScheduler()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_sched) {
        ggml_backend_sched_free(m_sched);
        m_sched = nullptr;
    }

    if (m_state == ManagerState::Ready)
        buildScheduler();
}

std::string JobGgmlDeviceManager::debugStr() const
{
    // std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;

    oss << "=== JobGgmlDeviceManager Debug ===\n";
    oss << "State: " << static_cast<int>(m_state) << "\n";
    oss << "Device count: " << m_devices.size() << "\n";

    for (const auto &dev : m_devices)
    {
        if (!dev)
            continue;

        oss << " - " << dev->name()
            << " | type=" << dev->type()
            << " | backend=" << (dev->backend() ? "ok" : "null")
            << "\n";
    }

    oss << "CPU: ";
    auto *cpu = cpuDevice();
    if (cpu)
        oss << cpu->name();
    else
        oss << "null";

    oss << "\n";

    auto gpus = gpuDevices();
    oss << "GPU count: " << gpus.size() << "\n";

    for (size_t i = 0; i < gpus.size(); ++i)
    {
        oss << "   [" << i << "] "
            << gpus[i]->name()
            << "\n";
    }

    oss << "Scheduler: " << (m_sched ? "initialized" : "null") << "\n";
    oss << "==================================\n";

    return oss.str();
}

} // namespace job::ggml