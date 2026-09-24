#include "job_drm_resources.h"

namespace job::render {

JobDrmResources JobDrmResources::fromDrm(const drmModeRes &resources)
{
    JobDrmResources result;

    if (resources.count_fbs > 0 && resources.fbs)
        result.m_framebufferIds.assign(resources.fbs, resources.fbs + resources.count_fbs);

    if (resources.count_crtcs > 0 && resources.crtcs)
        result.m_crtcIds.assign( resources.crtcs, resources.crtcs + resources.count_crtcs);

    if (resources.count_connectors > 0 && resources.connectors)
        result.m_connectorIds.assign(resources.connectors, resources.connectors + resources.count_connectors);

    if (resources.count_encoders > 0 && resources.encoders)
        result.m_encoderIds.assign(resources.encoders, resources.encoders + resources.count_encoders);

    result.m_minWidth = static_cast<std::uint32_t>(resources.min_width);
    result.m_maxWidth = static_cast<std::uint32_t>(resources.max_width);
    result.m_minHeight = static_cast<std::uint32_t>(resources.min_height);
    result.m_maxHeight = static_cast<std::uint32_t>(resources.max_height);
    return result;
}

}