#include "job_drm_atomic_request.h"

namespace job::render {

void JobDrmAtomicRequest::DrmModeAtomicReqDeleter::operator()(drmModeAtomicReq *request) const noexcept
{
    if (request)
        drmModeAtomicFree(request);
}

bool JobDrmAtomicRequest::build(const JobDrmAtomicState &state)
{
    clear();

    m_request.reset(drmModeAtomicAlloc());
    if (!m_request)
        return false;

    for (const auto &object : state.objects()) {
        for (const auto &property : object.properties) {
            if (!add(object.id, property.id, property.value)) {
                clear();
                return false;
            }
        }
    }

    return true;
}

bool JobDrmAtomicRequest::add(JobDrmAtomicState::ObjectId objectId,
                              JobDrmAtomicState::PropertyId propertyId,
                              JobDrmAtomicState::Value value)
{
    if (!m_request || objectId == 0 || propertyId == 0)
        return false;

    const int result = drmModeAtomicAddProperty(m_request.get(), objectId, propertyId, value);
    if (result < 0)
        return false;

    ++m_propertyCount;

    return true;
}

void JobDrmAtomicRequest::clear() noexcept
{
    m_request.reset();
    m_propertyCount = 0;
}

bool JobDrmAtomicRequest::commit(int fd, Flags flags, UserData userData) noexcept
{
    if (fd < 0 || !m_request)
        return false;

    return drmModeAtomicCommit(fd, m_request.get(), flags, userData) == 0;
}

bool JobDrmAtomicRequest::test(int fd, Flags flags) noexcept
{
    if (fd < 0 || !m_request)
        return false;

    return drmModeAtomicCommit(fd, m_request.get(), flags | DRM_MODE_ATOMIC_TEST_ONLY, nullptr) == 0;
}

}