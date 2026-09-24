#include "job_kms_format_negotiator.h"

namespace job::render {

std::optional<JobKmsFormatNegotiator::Result> JobKmsFormatNegotiator::negotiate(
    JobGbmDevice &gbmDevice,
    EGLDisplay eglDisplay,
    const JobDrmPlane &plane,
    Width width,
    Height height,
    Usage usage,
    std::span<const JobDrmFormat> preferredFormats,
    const JobEglConfig::Requirements &eglRequirements)
{
    if (!gbmDevice.valid() ||
        eglDisplay == EGL_NO_DISPLAY ||
        !plane.valid() ||
        width == 0 ||
        height == 0 ||
        usage == 0 ||
        preferredFormats.empty() ||
        !eglRequirements.valid()) {
        return std::nullopt;
    }

    for (const JobDrmFormat format : preferredFormats) {
        if (!format.valid() || !plane.supportsFormat(format))
            continue;

        if (eglRequirements.nativeVisualId &&
            *eglRequirements.nativeVisualId != static_cast<EGLint>(format.value())) {
            continue;
        }

        const JobGbmFormat gbmFormat{format};

        if (!gbmDevice.supportsFormat(gbmFormat, usage))
            continue;

        auto requirements = eglRequirements;
        requirements.nativeVisualId = static_cast<EGLint>(format.value());

        auto eglConfig = JobEglConfig::select(eglDisplay, requirements);

        if (!eglConfig)
            continue;

        auto surface = JobGbmSurface::createShared();

        if (!surface)
            continue;

        std::vector<JobGbmModifier> modifiers;

        if (plane.hasExplicitModifiers()) {
            for (const auto &formatModifier : plane.formatModifiers()) {
                if (formatModifier.format() == format)
                    modifiers.emplace_back(formatModifier.modifier());
            }

            if (modifiers.empty())
                continue;

            if (!surface->create(gbmDevice.device(),
                                 width,
                                 height,
                                 gbmFormat,
                                 modifiers,
                                 usage)) {
                continue;
            }
        } else {
            if (!surface->create(gbmDevice.device(),
                                 width,
                                 height,
                                 gbmFormat,
                                 usage)) {
                continue;
            }
        }

        Result result{
            .format = format,
            .modifiers = std::move(modifiers),
            .surface = std::move(surface),
            .eglConfig = std::move(*eglConfig)
        };

        if (result.valid())
            return result;
    }

    return std::nullopt;
}

} // namespace job::render