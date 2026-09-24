#include "job_egl_config.h"

#include <tuple>
#include <vector>

namespace job::render {

std::optional<JobEglConfig> JobEglConfig::select(EGLDisplay display, const Requirements &requirements)
{
    if (display == EGL_NO_DISPLAY || !requirements.valid())
        return std::nullopt;

    std::vector<Attribute> attributes{
        EGL_RED_SIZE,           requirements.redSize,
        EGL_GREEN_SIZE,         requirements.greenSize,
        EGL_BLUE_SIZE,          requirements.blueSize,
        EGL_ALPHA_SIZE,         requirements.alphaSize,
        EGL_DEPTH_SIZE,         requirements.depthSize,
        EGL_STENCIL_SIZE,       requirements.stencilSize,
        EGL_SAMPLE_BUFFERS,     requirements.sampleBuffers,
        EGL_SAMPLES,            requirements.samples,
        EGL_SURFACE_TYPE,       requirements.surfaceType,
        EGL_RENDERABLE_TYPE,    requirements.renderableType
    };

    if (requirements.nativeVisualId) {
        attributes.push_back(EGL_NATIVE_VISUAL_ID);
        attributes.push_back(*requirements.nativeVisualId);
    }

    attributes.push_back(EGL_NONE);

    EGLint configCount = 0;
    if (eglChooseConfig(display, attributes.data(), nullptr, 0, &configCount) != EGL_TRUE || configCount <= 0)
        return std::nullopt;

    std::vector<EGLConfig> configs(static_cast<std::size_t>(configCount));
    if (eglChooseConfig(display, attributes.data(), configs.data(), configCount, &configCount) != EGL_TRUE || configCount <= 0)
        return std::nullopt;

    configs.resize(static_cast<std::size_t>(configCount));

    std::optional<JobEglConfig> selected;
    std::optional<std::tuple<bool, Value, Value, Id>> selectedRank;

    for (EGLConfig config : configs) {
        JobEglConfig candidate{config};

        if (!candidate.query(display) || !candidate.matches(requirements))
            continue;

        const Value ancillaryExcess =
            (candidate.depthSize() - requirements.depthSize) +
            (candidate.stencilSize() - requirements.stencilSize) +
            (candidate.sampleBuffers() - requirements.sampleBuffers) +
            (candidate.samples() - requirements.samples);

        const Value colorExcess =
            (candidate.redSize() - requirements.redSize) +
            (candidate.greenSize() - requirements.greenSize) +
            (candidate.blueSize() - requirements.blueSize) +
            (candidate.alphaSize() - requirements.alphaSize);

        const auto rank = std::tuple{
            candidate.configCaveat() != EGL_NONE,
            ancillaryExcess,
            colorExcess,
            candidate.id()
        };

        if (!selectedRank || rank < *selectedRank) {
            selected = candidate;
            selectedRank = rank;
        }
    }

    return selected;
}

bool JobEglConfig::query(EGLDisplay display)
{
    m_queried = false;

    m_id                = 0;
    m_redSize           = 0;
    m_greenSize         = 0;
    m_blueSize          = 0;
    m_alphaSize         = 0;
    m_depthSize         = 0;
    m_stencilSize       = 0;
    m_bufferSize        = 0;
    m_samples           = 0;
    m_sampleBuffers     = 0;
    m_surfaceType       = 0;
    m_renderableType    = 0;
    m_conformant        = 0;
    m_nativeVisualId    = 0;
    m_nativeVisualType  = 0;
    m_configCaveat      = EGL_NONE;

    if (display == EGL_NO_DISPLAY || !m_config)
        return false;

    auto queryAttribute = [this, display](Attribute attribute, Value &value) {
        return eglGetConfigAttrib(display, m_config, attribute, &value) == EGL_TRUE;
    };

    if (!queryAttribute(EGL_CONFIG_ID, m_id))
        return false;

    if (!queryAttribute(EGL_RED_SIZE, m_redSize))
        return false;

    if (!queryAttribute(EGL_GREEN_SIZE, m_greenSize))
        return false;

    if (!queryAttribute(EGL_BLUE_SIZE, m_blueSize))
        return false;

    if (!queryAttribute(EGL_ALPHA_SIZE, m_alphaSize))
        return false;

    if (!queryAttribute(EGL_DEPTH_SIZE, m_depthSize))
        return false;

    if (!queryAttribute(EGL_STENCIL_SIZE, m_stencilSize))
        return false;

    if (!queryAttribute(EGL_BUFFER_SIZE, m_bufferSize))
        return false;

    if (!queryAttribute(EGL_SAMPLES, m_samples))
        return false;

    if (!queryAttribute(EGL_SAMPLE_BUFFERS, m_sampleBuffers))
        return false;

    if (!queryAttribute(EGL_SURFACE_TYPE, m_surfaceType))
        return false;

    if (!queryAttribute(EGL_RENDERABLE_TYPE, m_renderableType))
        return false;

    if (!queryAttribute(EGL_CONFORMANT, m_conformant))
        return false;

    if (!queryAttribute(EGL_NATIVE_VISUAL_ID, m_nativeVisualId))
        return false;

    if (!queryAttribute(EGL_NATIVE_VISUAL_TYPE, m_nativeVisualType))
        return false;

    if (!queryAttribute(EGL_CONFIG_CAVEAT, m_configCaveat))
        return false;

    m_queried = true;

    return true;
}

bool JobEglConfig::matches(const Requirements &requirements) const noexcept
{
    if (!valid() || !requirements.valid())
        return false;

    if (m_redSize < requirements.redSize)
        return false;

    if (m_greenSize < requirements.greenSize)
        return false;

    if (m_blueSize < requirements.blueSize)
        return false;

    if (m_alphaSize < requirements.alphaSize)
        return false;

    if (m_depthSize < requirements.depthSize)
        return false;

    if (m_stencilSize < requirements.stencilSize)
        return false;

    if (m_sampleBuffers < requirements.sampleBuffers)
        return false;

    if (m_samples < requirements.samples)
        return false;

    if ((m_surfaceType & requirements.surfaceType) != requirements.surfaceType)
        return false;

    if ((m_renderableType & requirements.renderableType) != requirements.renderableType)
        return false;

    if (requirements.nativeVisualId && m_nativeVisualId != *requirements.nativeVisualId)
        return false;

    return true;
}

}