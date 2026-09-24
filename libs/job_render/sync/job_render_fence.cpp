#include "job_render_fence.h"

#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace job::render {

JobRenderFence::~JobRenderFence()
{
    reset();
}

JobRenderFence::JobRenderFence(JobRenderFence &&other) noexcept :
    m_fd(other.release())
{
}

JobRenderFence &JobRenderFence::operator=(JobRenderFence &&other) noexcept
{
    if (this != &other)
        reset(other.release());

    return *this;
}

void JobRenderFence::reset(NativeHandle fd) noexcept
{
    if (m_fd >= 0)
        ::close(m_fd);

    m_fd = fd;
}

JobRenderFence::NativeHandle JobRenderFence::release() noexcept
{
    const NativeHandle fd = m_fd;
    m_fd = -1;

    return fd;
}

bool JobRenderFence::signaled() const noexcept
{
    if (!valid())
        return false;

    pollfd descriptor{};
    descriptor.fd = m_fd;
    descriptor.events = POLLIN;

    for (;;) {
        const int result = ::poll(&descriptor, 1, 0);
        if (result > 0)
            return (descriptor.revents & POLLIN) != 0;

        if (result == 0)
            return false;

        if (errno != EINTR)
            return false;
    }
}

bool JobRenderFence::wait() const noexcept
{
    if (!valid())
        return false;

    pollfd descriptor{};
    descriptor.fd = m_fd;
    descriptor.events = POLLIN;

    for (;;) {
        const int result = ::poll(&descriptor, 1, -1);

        if (result > 0)
            return (descriptor.revents & POLLIN) != 0;

        if (result == 0)
            continue;

        if (errno != EINTR)
            return false;
    }
}

bool JobRenderFence::wait(Timeout timeout) const noexcept
{
    if (!valid())
        return false;

    const auto count = timeout.count();

    if (count < 0)
        return wait();

    const int timeoutMs = count > static_cast<decltype(count)>(INT_MAX) ? INT_MAX : static_cast<int>(count);

    pollfd descriptor{};
    descriptor.fd = m_fd;
    descriptor.events = POLLIN;

    for (;;) {
        const int result = ::poll(
            &descriptor,
            1,
            timeoutMs);

        if (result > 0)
            return (descriptor.revents & POLLIN) != 0;

        if (result == 0)
            return false;

        if (errno != EINTR)
            return false;
    }
}

JobRenderFence JobRenderFence::duplicate() const noexcept
{
    if (!valid())
        return {};

    const int fd = ::fcntl( m_fd, F_DUPFD_CLOEXEC, 0);
    if (fd < 0)
        return {};

    return JobRenderFence{fd};
}

}