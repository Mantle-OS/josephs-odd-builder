#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <EGL/egl.h>

#include "job_drm_plane.h"
#include "job_egl_config.h"
#include "job_gbm_device.h"
#include "job_gbm_modifier.h"
#include "job_gbm_surface.h"

namespace job::render {

class JobKmsFormatNegotiator
{
public:
    using Width  = JobGbmSurface::Width;
    using Height = JobGbmSurface::Height;
    using Usage  = JobGbmSurface::Usage;

    struct Result
    {
        JobDrmFormat                format;
        std::vector<JobGbmModifier> modifiers;
        JobGbmSurface::Ptr          surface;
        JobEglConfig                eglConfig;

        [[nodiscard]] bool explicitModifiers() const noexcept
        {
            return !modifiers.empty();
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return format.valid() &&
                   surface &&
                   surface->valid() &&
                   eglConfig.valid();
        }
    };

    JobKmsFormatNegotiator() = delete;
    ~JobKmsFormatNegotiator() = delete;

    JobKmsFormatNegotiator(const JobKmsFormatNegotiator &) = delete;
    JobKmsFormatNegotiator &operator=(const JobKmsFormatNegotiator &) = delete;

    JobKmsFormatNegotiator(JobKmsFormatNegotiator &&) = delete;
    JobKmsFormatNegotiator &operator=(JobKmsFormatNegotiator &&) = delete;

    [[nodiscard]] static std::optional<Result> negotiate(JobGbmDevice &gbmDevice,
                                                         EGLDisplay eglDisplay,
                                                         const JobDrmPlane &plane,
                                                         Width width,
                                                         Height height,
                                                         Usage usage,
                                                         std::span<const JobDrmFormat> preferredFormats,
                                                         const JobEglConfig::Requirements &eglRequirements);
};

} // namespace job::render