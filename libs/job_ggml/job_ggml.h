#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <ggml.h>

#include "job_ggml_device_manager.h"
#include "job_gguf.h"
#include "jobggml_export.h"

namespace job::ggml {

class JobGgmlContext;

/*
 * Top-level JOB GGML facade.
 *
 * Owns process-lifetime GGML subsystem services.
 *
 * Workload-scoped objects such as contexts, tensors, operations, graphs,
 * schedulers, optimization runs, and thread pools remain explicitly owned
 * by their callers.
 */
class JOBGGML_EXPORT JobGgml
{
public:
    using Ptr  = std::shared_ptr<JobGgml>;
    using WPtr = std::weak_ptr<JobGgml>;
    using UPtr = std::unique_ptr<JobGgml>;

    using LogCallback = std::function<void(ggml_log_level level, const char *text, void *userData)>;

    explicit JobGgml(bool scanDevices = true);
    ~JobGgml();

    [[nodiscard]] static Ptr createShared(bool scanDevices = true)
    {
        return std::make_shared<JobGgml>(scanDevices);
    }

    [[nodiscard]] static UPtr createUniq(bool scanDevices = true)
    {
        return std::make_unique<JobGgml>(scanDevices);
    }

    JobGgml(const JobGgml &) = delete;
    JobGgml &operator=(const JobGgml &) = delete;
    JobGgml(JobGgml &&) = delete;
    JobGgml &operator=(JobGgml &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] const std::string &version() const noexcept;
    [[nodiscard]] const std::string &commit() const noexcept;

    [[nodiscard]] JobGgmlDeviceManager *deviceManager() noexcept;
    [[nodiscard]] const JobGgmlDeviceManager *deviceManager() const noexcept;

    [[nodiscard]] JobGguf *gguf() noexcept;
    [[nodiscard]] const JobGguf *gguf() const noexcept;

    void setLogCallback(LogCallback callback, void *userData = nullptr);
    void clearLogCallback() noexcept;

    static void timeInit() noexcept;

    [[nodiscard]] static std::int64_t timeMs() noexcept;
    [[nodiscard]] static std::int64_t timeUs() noexcept;
    [[nodiscard]] static std::int64_t cycles() noexcept;
    [[nodiscard]] static std::int64_t cyclesPerMs() noexcept;

    static void printObjects(const JobGgmlContext &context);

private:
    static void callLogBouncer(ggml_log_level level, const char *text, void *userData);

    JobGgmlDeviceManager::UPtr m_deviceManager; // Owned.
    JobGguf::UPtr               m_gguf;          // Owned.

    std::string                 m_version;
    std::string                 m_commit;

    LogCallback                 m_logCallback;
    void                       *m_logUserData{nullptr}; // Borrowed.
};

} // namespace job::ggml