#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <ggml-backend.h>

#include "job_ggml_backend_sched.h"
#include "job_ggml_device.h"
#include "job_ggml_enums.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlDeviceManager
{
public:
    using Ptr  = std::shared_ptr<JobGgmlDeviceManager>;
    using WPtr = std::weak_ptr<JobGgmlDeviceManager>;
    using UPtr = std::unique_ptr<JobGgmlDeviceManager>;

    JobGgmlDeviceManager() = default;
    ~JobGgmlDeviceManager() = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<JobGgmlDeviceManager>(); }

    [[nodiscard]] static UPtr createUniq() { return std::make_unique<JobGgmlDeviceManager>(); }

    JobGgmlDeviceManager( const JobGgmlDeviceManager & ) = delete;
    JobGgmlDeviceManager &operator=( const JobGgmlDeviceManager & ) = delete;
    JobGgmlDeviceManager( JobGgmlDeviceManager && ) = delete;
    JobGgmlDeviceManager &operator=( JobGgmlDeviceManager && ) = delete;


    enum class ManagerState : uint8_t
    {
        Uninitialized,
        Scanning,
        Ready,
        Error
    };
    /*
     * Scans GGML's process-wide backend device registry and rebuilds the
     * manager's canonical device object map.
     *
     * Calling scan() while the manager is already Ready is intentionally
     * idempotent. Existing device wrapper identities remain stable.
     */
    void scan();

    [[nodiscard]] ManagerState state() const noexcept;
    [[nodiscard]] bool isReady() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] const std::string &errorString() const noexcept;

    [[nodiscard]] std::size_t deviceCount() const noexcept;
    [[nodiscard]] std::size_t gpuDeviceCount() const noexcept;

    [[nodiscard]] JobGgmlDevice *device(std::size_t index) noexcept;

    [[nodiscard]] const JobGgmlDevice *device(std::size_t index) const noexcept;

    [[nodiscard]] JobGgmlDevice::Ptr deviceShared(std::size_t index) const noexcept;

    [[nodiscard]] const std::vector<JobGgmlDevice::Ptr> &devices() const noexcept;
    [[nodiscard]] JobGgmlDevice *cpuDevice() noexcept;

    [[nodiscard]] const JobGgmlDevice *cpuDevice() const noexcept;
    [[nodiscard]] JobGgmlDevice::Ptr cpuDeviceShared() const noexcept;

    [[nodiscard]] std::vector<JobGgmlDevice *>gpuDevices() noexcept;

    [[nodiscard]] std::vector<const JobGgmlDevice *>gpuDevices() const noexcept;

    [[nodiscard]] const std::vector<JobGgmlDevice::Ptr> &gpuDevicesShared() const noexcept;

    [[nodiscard]] bool hasCpu() const noexcept;
    [[nodiscard]] bool hasGpu() const noexcept;

    [[nodiscard]] JobGgmlDevice *deviceByName(const std::string &name) noexcept;

    [[nodiscard]] const JobGgmlDevice *deviceByName(const std::string &name) const noexcept;

    [[nodiscard]] JobGgmlDevice::Ptr deviceByNameShared(const std::string &name) const noexcept;

    [[nodiscard]] JobGgmlDevice *deviceById(const std::string &deviceId) noexcept;

    [[nodiscard]] const JobGgmlDevice *deviceById(const std::string &deviceId) const noexcept;

    [[nodiscard]] JobGgmlDevice::Ptr deviceByIdShared(const std::string &deviceId) const noexcept;

    /*
     * Builds a scheduler from the currently discovered devices.
     *
     * GPU backends are placed before the CPU backend so they receive higher
     * scheduler priority. Devices without valid backends are skipped.
     */
    [[nodiscard]] JobGgmlBackendSched::Ptr buildScheduler(std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool parallel = true, bool opOffload = true);

    /*
     * Builds a scheduler from an explicit ordered device list.
     *
     * The supplied order is retained because GGML gives lower-index
     * backends higher priority.
     */
    [[nodiscard]] JobGgmlBackendSched::Ptr buildScheduler(const std::vector<JobGgmlDevice *> &devices, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool parallel = true, bool opOffload = true);

    [[nodiscard]] JobGgmlBackendSched::Ptr
    scheduler() const noexcept;

    [[nodiscard]] bool hasScheduler() const noexcept;

    void resetScheduler() noexcept;
    void reset() noexcept;

    [[nodiscard]] std::string debugString() const;

private:
    void setState(ManagerState state) noexcept;
    void setErrorString(const std::string &errorString);
    void clearError() noexcept;

    [[nodiscard]] JobGgmlDevice::Ptr deviceFromNative(ggml_backend_dev_t device) const noexcept;
    [[nodiscard]] JobGgmlBackendReg::Ptr registryFromNative(ggml_backend_reg_t backendReg) const noexcept;

    void rebuildDeviceLists();

    ManagerState m_state{ManagerState::Uninitialized};

    std::string m_errorString;

    /*
     * Canonical owning device list.
     *
     * Raw pointers returned by this manager remain valid until reset() or
     * destruction of the manager.
     */
    std::vector<JobGgmlDevice::Ptr> m_devices;

    JobGgmlDevice::Ptr              m_cpuDevice;
    std::vector<JobGgmlDevice::Ptr> m_gpuDevices;

    /*
     * Registries are borrowed process-wide GGML objects wrapped in shared
     * JOB objects. Keeping canonical registry wrappers here allows devices
     * from the same registry to share the same JobGgmlBackendReg instance.
     */
    std::vector<JobGgmlBackendReg::Ptr> m_backendRegistries;
    JobGgmlBackendSched::Ptr m_scheduler;
};

} // namespace job::ggml