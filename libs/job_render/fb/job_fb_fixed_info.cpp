#include "job_fb_fixed_info.h"

#include <algorithm>
#include <cstring>

namespace job::render {

JobFbFixedInfo JobFbFixedInfo::fromFb(const fb_fix_screeninfo &info)
{
    JobFbFixedInfo result;

    const auto *idBegin = reinterpret_cast<const char *>(info.id);
    const auto *idEnd = std::find(idBegin, idBegin + sizeof(info.id), '\0');
    result.m_id.assign(idBegin, idEnd);

    result.m_memoryStart    = static_cast<MemoryAddress>( info.smem_start);
    result.m_memoryLength   = info.smem_len;
    result.m_type           = JobFbType{ static_cast<JobFbType::Value>( info.type) };
    result.m_typeAux        = info.type_aux;
    result.m_visual         = JobFbVisual{ static_cast<JobFbVisual::Value>( info.visual) };
    result.m_xPanStep       = info.xpanstep;
    result.m_yPanStep       = info.ypanstep;
    result.m_yWrapStep      = info.ywrapstep;
    result.m_lineLength     = info.line_length;
    result.m_mmioStart      = static_cast<MemoryAddress>(info.mmio_start);
    result.m_mmioLength     = info.mmio_len;
    result.m_acceleration   = info.accel;
    result.m_capabilities   = JobFbCapabilities{ static_cast<JobFbCapabilities::Value>(info.capabilities) };
    result.m_reserved       = { info.reserved[0], info.reserved[1] };

    return result;
}

fb_fix_screeninfo JobFbFixedInfo::toFb() const noexcept
{
    fb_fix_screeninfo info{};
    const std::size_t idLength = std::min(m_id.size(), sizeof(info.id));
    if (idLength > 0)
        std::memcpy(info.id, m_id.data(), idLength);

    info.smem_start     = static_cast<decltype(info.smem_start)>( m_memoryStart);
    info.smem_len       = m_memoryLength;
    info.type           = static_cast<decltype(info.type)>( m_type.value());
    info.type_aux       = m_typeAux;
    info.visual         = static_cast<decltype(info.visual)>(m_visual.value());
    info.xpanstep       = m_xPanStep;
    info.ypanstep       = m_yPanStep;
    info.ywrapstep      = m_yWrapStep;
    info.line_length    = m_lineLength;
    info.mmio_start     = static_cast<decltype(info.mmio_start)>(m_mmioStart);
    info.mmio_len       = m_mmioLength;
    info.accel          = m_acceleration;
    info.capabilities   = static_cast<decltype(info.capabilities)>(m_capabilities.value());
    info.reserved[0]    = m_reserved[0];
    info.reserved[1]    = m_reserved[1];

    return info;
}

}