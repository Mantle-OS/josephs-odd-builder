#include "job_input_linux_reader.h"

#include <cerrno>

namespace job::input {

JobInputLinuxReader::JobInputLinuxReader(JobInputLinuxDevice::Ptr device) noexcept :
    m_device(std::move(device))
{
}

bool JobInputLinuxReader::readEvents()
{
    const auto device = m_device.lock();
    if (!device || !device->isOpen())
        return false;

    const auto bytesRead = device->read(reinterpret_cast<char *>(m_events.data()), sizeof(m_events));
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return true;

        return false;
    }

    if (bytesRead == 0)
        return false;

    if ((bytesRead % sizeof(input_event)) != 0)
        return false;

    const std::size_t eventCount = static_cast<std::size_t>(bytesRead) / sizeof(input_event);
    if (!m_eventCallback)
        return true;

    for (std::size_t i = 0; i < eventCount; ++i) {
        const JobInputEvent event = makeEvent(m_events[i]);
        m_eventCallback(event);
    }

    return true;
}

} // namespace job::input