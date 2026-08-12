#pragma once

#include <memory>
#include <utility>

#include <ggml-cpp.h>
#include <ggml-backend.h>

#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgmlBackendEvent
{
public:
    using Ptr  = std::shared_ptr<JobGgmlBackendEvent>;
    using WPtr = std::weak_ptr<JobGgmlBackendEvent>;
    using UPtr = std::unique_ptr<JobGgmlBackendEvent>;


    explicit JobGgmlBackendEvent(ggml_backend_dev_t device);    // BACKPORT add construction with  JobGgmlDevice
    explicit JobGgmlBackendEvent(ggml_backend_event_t event);   // Takes ownership of the supplied native event.
    explicit JobGgmlBackendEvent(ggml_backend_event_ptr event); // Takes ownership by moving the native RAII event.

    ~JobGgmlBackendEvent() = default;
    JobGgmlBackendEvent(const JobGgmlBackendEvent &) = delete;
    JobGgmlBackendEvent &operator=(const JobGgmlBackendEvent &) = delete;
    JobGgmlBackendEvent(JobGgmlBackendEvent &&) = delete;
    JobGgmlBackendEvent &operator=(JobGgmlBackendEvent &&) = delete;

    [[nodiscard]] static Ptr createShared(ggml_backend_dev_t device) { return std::make_shared<JobGgmlBackendEvent>(device); }
    [[nodiscard]] static Ptr createShared(ggml_backend_event_t event) { return std::make_shared<JobGgmlBackendEvent>(event); }
    [[nodiscard]] static Ptr createShared(ggml_backend_event_ptr event) { return std::make_shared<JobGgmlBackendEvent>(std::move(event)); }

    [[nodiscard]] static UPtr createUniq(ggml_backend_dev_t device) { return std::make_unique<JobGgmlBackendEvent>(device); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_event_t event) { return std::make_unique<JobGgmlBackendEvent>(event); }
    [[nodiscard]] static UPtr createUniq(ggml_backend_event_ptr event) { return std::make_unique<JobGgmlBackendEvent>(std::move(event)); }


    [[nodiscard]] ggml_backend_event_t event() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    void synchronize();

private:
    ggml_backend_event_ptr m_event;
};

} // namespace job::ggml