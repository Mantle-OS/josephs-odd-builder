#include "job_drm_capabilities.h"

#include <xf86drm.h>

namespace job::render {

bool JobDrmCapabilities::query(int fd) noexcept
{
    clear();

    auto queryCap = [fd](std::uint64_t capability, Value &value) noexcept {
        value = 0;

        const int result = drmGetCap(fd, capability, &value);
        if (result != 0)
            value = 0;
    };

    queryCap(DRM_CAP_DUMB_BUFFER, m_dumbBuffer);
    queryCap(DRM_CAP_VBLANK_HIGH_CRTC, m_vblankHighCrtc);
    queryCap(DRM_CAP_DUMB_PREFERRED_DEPTH, m_dumbPreferredDepth);
    queryCap(DRM_CAP_DUMB_PREFER_SHADOW, m_dumbPreferShadow);
    queryCap(DRM_CAP_PRIME, m_prime);
    queryCap(DRM_CAP_TIMESTAMP_MONOTONIC, m_timestampMonotonic);
    queryCap(DRM_CAP_ASYNC_PAGE_FLIP, m_asyncPageFlip);
    queryCap(DRM_CAP_CURSOR_WIDTH, m_cursorWidth);
    queryCap(DRM_CAP_CURSOR_HEIGHT, m_cursorHeight);
    queryCap(DRM_CAP_ADDFB2_MODIFIERS, m_addFb2Modifiers);
    queryCap(DRM_CAP_PAGE_FLIP_TARGET, m_pageFlipTarget);
    queryCap(DRM_CAP_CRTC_IN_VBLANK_EVENT, m_crtcInVblankEvent);
    queryCap(DRM_CAP_SYNCOBJ, m_syncobj);
    queryCap(DRM_CAP_SYNCOBJ_TIMELINE, m_syncobjTimeline);
    queryCap(DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP, m_atomicAsyncPageFlip);

    m_valid = true;
    return true;
}

void JobDrmCapabilities::clear() noexcept
{
    m_valid = false;

    m_dumbBuffer = 0;
    m_vblankHighCrtc = 0;
    m_dumbPreferredDepth = 0;
    m_dumbPreferShadow = 0;
    m_prime = 0;
    m_timestampMonotonic = 0;
    m_asyncPageFlip = 0;
    m_cursorWidth = 0;
    m_cursorHeight = 0;
    m_addFb2Modifiers = 0;
    m_pageFlipTarget = 0;
    m_crtcInVblankEvent = 0;
    m_syncobj = 0;
    m_syncobjTimeline = 0;
    m_atomicAsyncPageFlip = 0;
}

}