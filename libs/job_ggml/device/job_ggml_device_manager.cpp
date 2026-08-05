#include "job_ggml_device_manager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace job::ggml {

void JobGgmlDeviceManager::scan()
{
    // Preserve canonical wrapper identity across repeated scans.
    if (m_state == ManagerState::Ready)
        return;

    setState(ManagerState::Scanning);
    clearError();

    // A previous failed scan may have left a partial object map.
    resetScheduler();
    m_cpuDevice.reset();
    m_gpuDevices.clear();
    m_devices.clear();
    m_backendRegistries.clear();

    try {
        // Load and register all known dynamic GGML backends before querying the process-wide device registry.
        ggml_backend_load_all();

        const std::size_t nativeDeviceCount =
            ggml_backend_dev_count();

        if (nativeDeviceCount == 0) {
            throw std::runtime_error{
                "GGML did not report any registered backend devices"
            };
        }

        m_devices.reserve(nativeDeviceCount);

        for (std::size_t index = 0; index < nativeDeviceCount; ++index) {
            ggml_backend_dev_t nativeDevice = ggml_backend_dev_get(index);

            if (!nativeDevice)
                continue;


            // This normally cannot find anything during a fresh scan, but it protects the canonical map if GGML exposes the same native device more than once.
            if (deviceFromNative(nativeDevice))
                continue;

            JobGgmlBackendReg::Ptr backendReg;

            const ggml_backend_reg_t nativeReg =
                ggml_backend_dev_backend_reg(nativeDevice);

            if (nativeReg) {
                backendReg =
                    registryFromNative(nativeReg);

                if (!backendReg) {
                    backendReg = JobGgmlBackendReg::createShared(nativeReg);
                    m_backendRegistries.push_back(backendReg);
                }
            }

            JobGgmlDevice::Ptr wrappedDevice = JobGgmlDevice::createShared(nativeDevice, std::move(backendReg));

            if (!wrappedDevice || !wrappedDevice->isValid()) {
                throw std::runtime_error{
                    "Failed to construct a valid JobGgmlDevice"
                };
            }

            m_devices.push_back(std::move(wrappedDevice));
        }

        if (m_devices.empty()) {
            throw std::runtime_error{
                "GGML devices were registered, but none could be wrapped"
            };
        }

        rebuildDeviceLists();

        if (!m_cpuDevice) {
            throw std::runtime_error{
                "GGML did not report a usable CPU device"
            };
        }

        setState(ManagerState::Ready);
    } catch (const std::exception &error) {
        resetScheduler();

        m_cpuDevice.reset();
        m_gpuDevices.clear();
        m_devices.clear();
        m_backendRegistries.clear();

        setErrorString(error.what());
        setState(ManagerState::Error);
    } catch (...) {
        resetScheduler();

        m_cpuDevice.reset();
        m_gpuDevices.clear();
        m_devices.clear();
        m_backendRegistries.clear();

        setErrorString(
            "Unknown error while scanning GGML devices"
            );

        setState(ManagerState::Error);
    }
}

JobGgmlDeviceManager::ManagerState JobGgmlDeviceManager::state() const noexcept
{
    return m_state;
}

bool JobGgmlDeviceManager::isReady() const noexcept
{
    return m_state == ManagerState::Ready;
}

bool JobGgmlDeviceManager::isValid() const noexcept
{
    return isReady() &&
           !m_devices.empty() &&
           m_cpuDevice &&
           m_cpuDevice->isValid();
}

const std::string &JobGgmlDeviceManager::errorString() const noexcept
{
    return m_errorString;
}

std::size_t JobGgmlDeviceManager::deviceCount() const noexcept
{
    return m_devices.size();
}

std::size_t JobGgmlDeviceManager::gpuDeviceCount() const noexcept
{
    return m_gpuDevices.size();
}

JobGgmlDevice *JobGgmlDeviceManager::device(std::size_t index) noexcept
{
    if (index >= m_devices.size())
        return nullptr;

    return m_devices[index].get();
}

const JobGgmlDevice *JobGgmlDeviceManager::device(std::size_t index) const noexcept
{
    if (index >= m_devices.size())
        return nullptr;

    return m_devices[index].get();
}

JobGgmlDevice::Ptr JobGgmlDeviceManager::deviceShared(std::size_t index) const noexcept
{
    if (index >= m_devices.size())
        return nullptr;

    return m_devices[index];
}

const std::vector<JobGgmlDevice::Ptr> &JobGgmlDeviceManager::devices() const noexcept
{
    return m_devices;
}

JobGgmlDevice *JobGgmlDeviceManager::cpuDevice() noexcept
{
    return m_cpuDevice.get();
}

const JobGgmlDevice *JobGgmlDeviceManager::cpuDevice() const noexcept
{
    return m_cpuDevice.get();
}

JobGgmlDevice::Ptr JobGgmlDeviceManager::cpuDeviceShared() const noexcept
{
    return m_cpuDevice;
}

std::vector<JobGgmlDevice*> JobGgmlDeviceManager::gpuDevices() noexcept
{
    std::vector<JobGgmlDevice *> devices;
    devices.reserve(m_gpuDevices.size());

    for (const JobGgmlDevice::Ptr &device :
         m_gpuDevices) {
        devices.push_back(device.get());
    }

    return devices;
}

std::vector<const JobGgmlDevice*> JobGgmlDeviceManager::gpuDevices() const noexcept
{
    std::vector<const JobGgmlDevice *> devices;
    devices.reserve(m_gpuDevices.size());
    for (const JobGgmlDevice::Ptr &device : m_gpuDevices)
        devices.push_back(device.get());

    return devices;
}

const std::vector<JobGgmlDevice::Ptr> &JobGgmlDeviceManager::gpuDevicesShared() const noexcept
{
    return m_gpuDevices;
}

bool JobGgmlDeviceManager::hasCpu() const noexcept
{
    return m_cpuDevice &&
           m_cpuDevice->isValid();
}

bool JobGgmlDeviceManager::hasGpu() const noexcept
{
    return !m_gpuDevices.empty();
}

JobGgmlDevice *JobGgmlDeviceManager::deviceByName(
    const std::string &name
    ) noexcept
{
    return const_cast<JobGgmlDevice *>(std::as_const(*this).deviceByName(name));
}

const JobGgmlDevice *JobGgmlDeviceManager::deviceByName(const std::string &name) const noexcept
{
    if (name.empty())
        return nullptr;

    const JobGgmlDevice::Ptr device = deviceByNameShared(name);

    return device.get();
}

JobGgmlDevice::Ptr JobGgmlDeviceManager::deviceByNameShared(const std::string &name) const noexcept
{
    if (name.empty())
        return nullptr;

    for (const JobGgmlDevice::Ptr &device :
         m_devices) {
        if (!device || !device->props())
            continue;

        if (device->props()->name() == name)
            return device;
    }

    return nullptr;
}

JobGgmlDevice *JobGgmlDeviceManager::deviceById(const std::string &deviceId) noexcept
{
    return const_cast<JobGgmlDevice *>(std::as_const(*this).deviceById(deviceId));
}

const JobGgmlDevice *JobGgmlDeviceManager::deviceById(const std::string &deviceId) const noexcept
{
    if (deviceId.empty())
        return nullptr;

    const JobGgmlDevice::Ptr device = deviceByIdShared(deviceId);
    return device.get();
}

JobGgmlDevice::Ptr JobGgmlDeviceManager::deviceByIdShared(const std::string &deviceId) const noexcept
{
    if (deviceId.empty())
        return nullptr;

    for (const JobGgmlDevice::Ptr &device : m_devices) {

        if (!device || !device->props())
            continue;

        if (device->props()->deviceId() == deviceId)
            return device;

    }

    return nullptr;
}

JobGgmlBackendSched::Ptr JobGgmlDeviceManager::buildScheduler(std::size_t graphSize, bool parallel, bool opOffload)
{
    if (!isReady()) {
        throw std::runtime_error{
            "Cannot build a scheduler before the device manager is ready"
        };
    }

    std::vector<JobGgmlDevice *> orderedDevices;
    orderedDevices.reserve(m_gpuDevices.size() + (m_cpuDevice ? 1U : 0U));


    // API(on code) says GGML gives lower-index backends higher priority.
    for (const JobGgmlDevice::Ptr &gpu :
         m_gpuDevices) {
        if (gpu)
            orderedDevices.push_back(gpu.get());
    }

    if (m_cpuDevice)
        orderedDevices.push_back(m_cpuDevice.get());

    return buildScheduler(orderedDevices, graphSize, parallel, opOffload);
}

JobGgmlBackendSched::Ptr JobGgmlDeviceManager::buildScheduler(const std::vector<JobGgmlDevice *> &devices, std::size_t graphSize, bool parallel, bool opOffload)
{
    if (!isReady()) {
        throw std::runtime_error{
            "Cannot build a scheduler before the device manager is ready"
        };
    }

    if (devices.empty()) {
        throw std::invalid_argument{
            "buildScheduler requires at least one device"
        };
    }

    if (graphSize == 0) {
        throw std::invalid_argument{
            "buildScheduler requires a graph size greater than zero"
        };
    }

    std::vector<JobGgmlBackend::Ptr> backends;
    backends.reserve(devices.size());

    for (JobGgmlDevice *device : devices) {
        if (!device || !device->isValid()) {
            throw std::invalid_argument{
                "buildScheduler requires valid JobGgmlDevice objects"
            };
        }

        /*
         * Require canonical devices owned by this manager. This prevents a
         * scheduler created here from silently depending on unrelated device
         * lifetimes.
         */
        const JobGgmlDevice::Ptr canonical = deviceFromNative(device->device());

        if (!canonical || canonical.get() != device) {
            throw std::invalid_argument{
                "buildScheduler requires devices owned by this manager"
            };
        }

        JobGgmlBackend::Ptr backend = device->backend();

        if (!backend || !backend->isValid()) {
            throw std::invalid_argument{
                "buildScheduler requires devices with valid backends"
            };
        }

        const auto duplicate = std::find_if(backends.cbegin(), backends.cend(),
                                            [&backend](const JobGgmlBackend::Ptr &candidate) {
                                                return candidate &&
                                                       candidate->backend() ==
                                                           backend->backend();
                                            });

        if (duplicate == backends.cend())
            backends.push_back(std::move(backend));
    }

    if (backends.empty()) {
        throw std::runtime_error{
            "No usable GGML backends were available for the scheduler"
        };
    }


    // An empty buffer-type vector asks the scheduler wrapper to use the backends' default buffer types.
    std::vector<JobGgmlBackendBufferType::Ptr> bufferTypes;

    m_scheduler =
        JobGgmlBackendSched::createShared(
            std::move(backends),
            std::move(bufferTypes),
            graphSize,
            parallel,
            opOffload
            );

    if (!m_scheduler || !m_scheduler->isValid()) {
        m_scheduler.reset();
        throw std::runtime_error{
            "Failed to create a valid GGML backend scheduler"
        };
    }

    return m_scheduler;
}

JobGgmlBackendSched::Ptr JobGgmlDeviceManager::scheduler() const noexcept
{
    return m_scheduler;
}

bool JobGgmlDeviceManager::hasScheduler() const noexcept
{
    return m_scheduler && m_scheduler->isValid();
}

void JobGgmlDeviceManager::resetScheduler() noexcept
{
    m_scheduler.reset();
}

void JobGgmlDeviceManager::reset() noexcept
{
    resetScheduler();

    m_cpuDevice.reset();
    m_gpuDevices.clear();

    m_devices.clear();
    m_backendRegistries.clear();

    clearError();
    setState(ManagerState::Uninitialized);
}

std::string JobGgmlDeviceManager::debugString() const
{
    std::ostringstream stream;

    stream
        << "JobGgmlDeviceManager{"
        << "state=";

    switch (m_state) {
    case ManagerState::Uninitialized:
        stream << "Uninitialized";
        break;

    case ManagerState::Scanning:
        stream << "Scanning";
        break;

    case ManagerState::Ready:
        stream << "Ready";
        break;

    case ManagerState::Error:
        stream << "Error";
        break;
    }

    stream
        << ", devices=" << m_devices.size()
        << ", gpus=" << m_gpuDevices.size()
        << ", hasCpu=" << (hasCpu() ? "true" : "false")
        << ", hasScheduler="
        << (hasScheduler() ? "true" : "false");

    if (!m_errorString.empty()) {
        stream
            << ", error=\""
            << m_errorString
            << '"';
    }

    stream << '}';

    return stream.str();
}

void JobGgmlDeviceManager::setState(ManagerState state) noexcept
{
    m_state = state;
}

void JobGgmlDeviceManager::setErrorString(const std::string &errorString)
{
    m_errorString = errorString;
}

void JobGgmlDeviceManager::clearError() noexcept
{
    m_errorString.clear();
}

JobGgmlDevice::Ptr JobGgmlDeviceManager::deviceFromNative(ggml_backend_dev_t device) const noexcept
{
    if (!device)
        return nullptr;

    for (const JobGgmlDevice::Ptr &candidate : m_devices)
        if (candidate && candidate->device() == device)
            return candidate;

    return nullptr;
}

JobGgmlBackendReg::Ptr JobGgmlDeviceManager::registryFromNative(ggml_backend_reg_t backendReg) const noexcept
{
    if (!backendReg)
        return nullptr;

    for (const JobGgmlBackendReg::Ptr &candidate : m_backendRegistries)
        if (candidate && candidate->backendReg() == backendReg)
            return candidate;

    return nullptr;
}

void JobGgmlDeviceManager::rebuildDeviceLists()
{
    m_cpuDevice.reset();
    m_gpuDevices.clear();

    m_gpuDevices.reserve(m_devices.size());

    for (const JobGgmlDevice::Ptr &device : m_devices) {

        if (!device || !device->isValid() || !device->props())
            continue;


        switch (device->props()->type()) {
        case GGML_BACKEND_DEVICE_TYPE_CPU:
            // Preserve the first CPU as the manager's canonical CPU device.
            if (!m_cpuDevice)
                m_cpuDevice = device;
            break;

        case GGML_BACKEND_DEVICE_TYPE_GPU:
        case GGML_BACKEND_DEVICE_TYPE_IGPU:
            m_gpuDevices.push_back(device);
            break;

        case GGML_BACKEND_DEVICE_TYPE_ACCEL:
        case GGML_BACKEND_DEVICE_TYPE_META:
            // TODO
            // These remain available through devices(), device(), name and ID lookup, but are not reported as GPUs
            break;
        }
    }
}

} // namespace job::ggml