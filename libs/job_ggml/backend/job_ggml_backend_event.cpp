#include "job_ggml_backend_event.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlBackendEvent::JobGgmlBackendEvent(ggml_backend_dev_t device) :
    m_event{device ? ggml_backend_event_new(device) : nullptr}
{
    if (!device)
        throw std::invalid_argument{
            "JobGgmlBackendEvent requires a valid ggml_backend_dev_t"
        };

    if (!m_event)
        throw std::runtime_error{
            "Failed to create GGML backend event"
        };
}

JobGgmlBackendEvent::JobGgmlBackendEvent(ggml_backend_event_t event) :
    m_event{event}
{
    if (!m_event)
        throw std::invalid_argument{
            "JobGgmlBackendEvent requires a valid ggml_backend_event_t"
        };
}

JobGgmlBackendEvent::JobGgmlBackendEvent(ggml_backend_event_ptr event) :
    m_event{std::move(event)}
{
    if (!m_event)
        throw std::invalid_argument{
            "JobGgmlBackendEvent requires a valid ggml_backend_event_ptr"
        };
}

ggml_backend_event_t JobGgmlBackendEvent::event() const noexcept
{
    return m_event.get();
}

bool JobGgmlBackendEvent::isValid() const noexcept
{
    return m_event != nullptr;
}

void JobGgmlBackendEvent::synchronize()
{
    if (!m_event)
        throw std::runtime_error{
            "Cannot synchronize an invalid GGML backend event"
        };

    ggml_backend_event_synchronize(m_event.get());
}

} // namespace job::ggml