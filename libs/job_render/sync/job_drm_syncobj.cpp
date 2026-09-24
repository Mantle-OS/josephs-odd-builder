#include "job_drm_syncobj.h"

#include <utility>

#include <xf86drm.h>

namespace job::render {

JobDrmSyncobj::~JobDrmSyncobj()
{
    reset();
}

JobDrmSyncobj::JobDrmSyncobj(JobDrmSyncobj &&other) noexcept :
    m_drmFd(std::exchange(other.m_drmFd, -1)),
    m_handle(std::exchange(other.m_handle, 0)),
    m_type(other.m_type)
{
    other.m_type = DrmSyncobjType::Binary;
}

JobDrmSyncobj &JobDrmSyncobj::operator=(JobDrmSyncobj &&other) noexcept
{
    if (this == &other)
        return *this;

    reset();

    m_drmFd = std::exchange(other.m_drmFd, -1);
    m_handle = std::exchange(other.m_handle, 0);
    m_type = other.m_type;

    other.m_type = DrmSyncobjType::Binary;

    return *this;
}

bool JobDrmSyncobj::create(int drmFd, DrmSyncobjType type, bool signaled)
{
    reset();

    if (drmFd < 0)
        return false;

    if (type == DrmSyncobjType::Timeline &&
        signaled) {
        return false;
    }

    std::uint32_t flags = 0;

    if (signaled)
        flags |= DRM_SYNCOBJ_CREATE_SIGNALED;

    Handle handle{};

    if (drmSyncobjCreate(drmFd, flags, &handle) != 0)
        return false;

    if (handle == 0)
        return false;

    m_drmFd = drmFd;
    m_handle = handle;
    m_type = type;

    return true;
}

bool JobDrmSyncobj::importSyncobjFd(int drmFd, DrmSyncobjType type, NativeHandle fd)
{
    reset();

    if (drmFd < 0 || fd < 0)
        return false;

    Handle handle{};

    if (drmSyncobjFDToHandle(drmFd, fd, &handle) != 0)
        return false;

    if (handle == 0)
        return false;

    m_drmFd = drmFd;
    m_handle = handle;
    m_type = type;

    return true;
}

void JobDrmSyncobj::reset() noexcept
{
    if (m_drmFd >= 0 && m_handle != 0)
        drmSyncobjDestroy(m_drmFd, m_handle);

    m_drmFd = -1;
    m_handle = 0;

    m_type = DrmSyncobjType::Binary;
}

bool JobDrmSyncobj::signal() noexcept
{
    if (!valid() || !binary())
        return false;

    const Handle handle = m_handle;

    return drmSyncobjSignal(m_drmFd, &handle, 1) == 0;
}

bool JobDrmSyncobj::resetFence() noexcept
{
    if (!valid() || !binary())
        return false;

    const Handle handle = m_handle;

    return drmSyncobjReset(m_drmFd, &handle, 1) == 0;
}

bool JobDrmSyncobj::wait(DeadlineNs deadlineNs,Flags flags) const noexcept
{
    if (!valid() || !binary())
        return false;

    Handle handle = m_handle;

    return drmSyncobjWait(m_drmFd,
                          &handle,
                          1,
                          deadlineNs,
                          flags,
                          nullptr) == 0;
}

bool JobDrmSyncobj::signal(Point point) noexcept
{
    if (!valid() || !timeline())
        return false;

    const Handle handle = m_handle;
    Point signalPoint = point;
    return drmSyncobjTimelineSignal(m_drmFd, &handle, &signalPoint, 1) == 0;
}

bool JobDrmSyncobj::wait(Point point, DeadlineNs deadlineNs, Flags flags) const noexcept
{
    if (!valid() || !timeline())
        return false;

    Handle handle = m_handle;
    Point waitPoint = point;

    return drmSyncobjTimelineWait(m_drmFd,
                                  &handle,
                                  &waitPoint,
                                  1,
                                  deadlineNs,
                                  flags,
                                  nullptr) == 0;
}

bool JobDrmSyncobj::query(Point &point) const noexcept
{
    point = 0;
    if (!valid() || !timeline())
        return false;

    Handle handle = m_handle;
    Point queriedPoint{};

    if (drmSyncobjQuery( m_drmFd, &handle, &queriedPoint, 1) != 0)
        return false;

    point = queriedPoint;

    return true;
}

bool JobDrmSyncobj::transfer(JobDrmSyncobj &destination, Point sourcePoint, Point destinationPoint, Flags flags) const noexcept
{
    if (!valid() || !destination.valid() || m_drmFd != destination.m_drmFd)
        return false;

    return drmSyncobjTransfer(m_drmFd,
                              destination.m_handle,
                              destinationPoint,
                              m_handle,
                              sourcePoint,
                              flags) == 0;
}

bool JobDrmSyncobj::importSyncFile(const JobRenderFence &fence)
{
    if (!valid() || !binary() || !fence.valid())
        return false;

    return drmSyncobjImportSyncFile(m_drmFd, m_handle, fence.fd()) == 0;
}

bool JobDrmSyncobj::importSyncFile(const JobRenderFence &fence, Point point)
{
    if (!valid() || !timeline() || !fence.valid())
        return false;

    JobDrmSyncobj temporary;

    if (!createTemporaryBinary(temporary))
        return false;

    if (!temporary.importSyncFile(fence))
        return false;

    return temporary.transfer(*this, 0, point);
}

JobRenderFence JobDrmSyncobj::exportSyncFile() const
{
    if (!valid() || !binary())
        return {};

    NativeHandle fd{-1};
    if (drmSyncobjExportSyncFile(m_drmFd, m_handle, &fd) != 0)
        return {};

    if (fd < 0)
        return {};

    return JobRenderFence{fd};
}

JobRenderFence JobDrmSyncobj::exportSyncFile(Point point) const
{
    if (!valid() || !timeline())
        return {};

    JobDrmSyncobj temporary;
    if (!createTemporaryBinary(temporary))
        return {};

    if (!transfer(temporary, point, 0))
        return {};

    return temporary.exportSyncFile();
}

JobDrmSyncobj::NativeHandle JobDrmSyncobj::exportSyncobjFd() const noexcept
{
    if (!valid())
        return -1;

    NativeHandle fd{-1};
    if (drmSyncobjHandleToFD(m_drmFd, m_handle, &fd) != 0)
        return -1;

    return fd;
}

bool JobDrmSyncobj::createTemporaryBinary(JobDrmSyncobj &syncobj) const
{
    if (!valid())
        return false;

    return syncobj.create(m_drmFd, DrmSyncobjType::Binary, false);
}

}