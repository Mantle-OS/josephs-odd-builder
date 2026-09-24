#include "job_kms_backend.h"

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <drm.h>
#include <drm_mode.h>
#include <gbm.h>

#include <job_logger.h>

#include "job_kms_format_negotiator.h"

namespace job::render {

const JobDrmMode *JobKmsBackend::selectMode(const JobDrmConnector &connector) noexcept
{
    const auto &modes = connector.modes();
    for (const auto &mode : modes) {
        if (mode.valid() && mode.preferred())
            return &mode;
    }

    for (const auto &mode : modes) {
        if (mode.valid())
            return &mode;
    }

    return nullptr;
}

bool JobKmsBackend::primaryPlane(const JobDrmPlane &plane) noexcept
{
    const auto value = plane.properties().value("type");
    return value && *value == DRM_PLANE_TYPE_PRIMARY;
}

JobKmsBackend::~JobKmsBackend()
{
    stop();
}

bool JobKmsBackend::start(const std::string &device)
{
    if (running())
        return true;

    stop();

    if (device.empty())
        m_drmDevice = JobDrmDevice::createShared();
    else
        m_drmDevice = JobDrmDevice::createShared(device);

    if (!m_drmDevice || !m_drmDevice->openDevice()) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to open DRM device.");
        stop();
        return false;
    }

    if (!m_drmDevice->setClientCapability(DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1) ||
        !m_drmDevice->setClientCapability(DRM_CLIENT_CAP_ATOMIC, 1)) {
        JOB_LOG_ERROR("[JobKmsBackend] DRM device does not support the required atomic KMS client capabilities.");
        stop();
        return false;
    }

    if (!discoverDisplay()) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to discover KMS display topology.");
        stop();
        return false;
    }

    if (!createGbm()) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to initialize GBM.");
        stop();
        return false;
    }

    if (!createEgl()) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to initialize EGL.");
        stop();
        return false;
    }

    if (!createPresenter()) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to initialize KMS presenter.");
        stop();
        return false;
    }

    JOB_LOG_INFO("[JobKmsBackend] KMS {}x{}.", width(), height());

    return true;
}

void JobKmsBackend::stop() noexcept
{
    if (m_presenter) {
        m_presenter->reset();
        m_presenter.reset();
    }

    if (m_eglContext && m_eglContext->valid())
        (void)m_eglContext->releaseCurrent();

    if (m_eglSurface) {
        m_eglSurface->reset();
        m_eglSurface.reset();
    }

    if (m_eglContext) {
        m_eglContext->reset();
        m_eglContext.reset();
    }

    m_eglConfig.reset();

    if (m_eglDisplay) {
        m_eglDisplay->reset();
        m_eglDisplay.reset();
    }

    if (m_gbmSurface) {
        m_gbmSurface->reset();
        m_gbmSurface.reset();
    }

    if (m_gbmDevice) {
        m_gbmDevice->reset();
        m_gbmDevice.reset();
    }

    resetDisplayState();

    m_format = {};
    m_explicitModifiers = false;
    m_nextFrameId = 1;

    if (m_drmDevice) {
        m_drmDevice->closeDevice();
        m_drmDevice.reset();
    }
}

bool JobKmsBackend::refresh()
{
    if (!m_drmDevice || !m_drmDevice->isOpen())
        return false;

    const std::string path = m_drmDevice->path();
    stop();

    return start(path);
}

bool JobKmsBackend::present()
{
    if (!running())
        return false;

    if (!m_eglSurface->swapBuffers()) {
        JOB_LOG_ERROR("[JobKmsBackend] EGL buffer swap failed.");
        return false;
    }

    gbm_bo *buffer = m_gbmSurface->lockFrontBuffer();
    if (!buffer) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to lock GBM front buffer.");
        return false;
    }

    JobGbmBufferLease lease;
    if (!lease.reset(m_gbmSurface->surface(), buffer)) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to acquire GBM buffer metadata.");
        return false;
    }

    if (lease.planeCount() > JobDrmDevice::FramebufferCreateInfo::MaxPlanes) {
        JOB_LOG_ERROR(
            "[JobKmsBackend] GBM buffer exposes too many planes for DRM FB2 registration.");
        return false;
    }

    const JobDrmFormat format = lease.format().drmFormat();
    if (format != m_format) {
        JOB_LOG_ERROR(
            "[JobKmsBackend] GBM front buffer format does not match the negotiated scanout format.");
        return false;
    }

    JobDrmModifier modifier = JobDrmModifier::invalid();
    if (m_explicitModifiers) {
        modifier = lease.modifier().drmModifier();

        if (!modifier.valid() ||
            !m_plane.supportsFormatModifier(format, modifier)) {
            JOB_LOG_ERROR("[JobKmsBackend] GBM front buffer modifier is not supported by the selected KMS plane.");
            return false;
        }
    }

    JobDrmDevice::FramebufferCreateInfo framebufferInfo{
        .width = lease.width(),
        .height = lease.height(),
        .format = format,
        .planeCount = lease.planeCount(),
        .modifier = modifier
    };

    for (std::size_t i = 0; i < lease.planeCount(); ++i) {
        const auto *plane = lease.plane(i);
        if (!plane)
            return false;

        framebufferInfo.handles[i] = plane->handle();
        framebufferInfo.pitches[i] = plane->stride();
        framebufferInfo.offsets[i] = plane->offset();
    }

    auto framebuffer = m_drmDevice->createFramebuffer(framebufferInfo);

    if (!framebuffer) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to register GBM buffer as a DRM framebuffer.");
        return false;
    }

    JobPresentFrame frame{ m_nextFrameId++, framebuffer->id() };
    JobPresentEntry entry{ std::move(frame), std::move(*framebuffer), std::move(lease) };

    if (!m_presenter->enqueue(std::move(entry))) {
        JOB_LOG_ERROR("[JobKmsBackend] Failed to enqueue KMS presentation entry.");
        return false;
    }

    if (!m_presenter->presentNext()) {
        JOB_LOG_ERROR("[JobKmsBackend] KMS presentation failed.");
        return false;
    }

    return true;
}

bool JobKmsBackend::discoverDisplay()
{
    resetDisplayState();

    if (!m_drmDevice || !m_drmDevice->isOpen())
        return false;

    const auto resources = m_drmDevice->resources();
    const auto planeResources = m_drmDevice->planeResources();

    if (!resources || !resources->valid() || !planeResources || !planeResources->valid())
        return false;

    for (const auto connectorId : resources->connectorIds()) {
        auto connector = m_drmDevice->connector(connectorId, true);

        if (!connector || !connector->valid() || !connector->connected()) {
            continue;
        }

        const JobDrmMode *mode = selectMode(*connector);
        if (!mode)
            continue;

        const JobDrmMode selectedMode = *mode;
        std::vector<JobDrmConnector::EncoderId> encoderIds;

        if (connector->encoderId() != 0)
            encoderIds.push_back(connector->encoderId());

        for (const auto encoderId : connector->encoders()) {
            if (encoderId != 0 && std::find( encoderIds.begin(), encoderIds.end(), encoderId) == encoderIds.end()) {
                encoderIds.push_back(encoderId);
            }
        }

        for (const auto encoderId : encoderIds) {
            const auto encoder = m_drmDevice->encoder(encoderId);

            if (!encoder || !encoder->valid())
                continue;

            for (std::size_t crtcIndex = 0;
                 crtcIndex < resources->crtcIds().size();
                 ++crtcIndex) {
                if (!encoder->supportsCrtc(static_cast<std::uint32_t>(crtcIndex))) {
                    continue;
                }

                const auto crtc =
                    m_drmDevice->crtc(resources->crtcIds()[crtcIndex]);

                if (!crtc || !crtc->valid())
                    continue;

                for (const auto planeId : planeResources->planeIds()) {
                    auto plane = m_drmDevice->plane(planeId);
                    if (!plane || !plane->valid() || !plane->supportsCrtc( static_cast<std::uint32_t>(crtcIndex)) || !primaryPlane(*plane)) {
                        continue;
                    }

                    m_connector = std::move(*connector);
                    m_crtc = std::move(*crtc);
                    m_plane = std::move(*plane);
                    m_mode = selectedMode;

                    return true;
                }
            }
        }
    }

    return false;
}

bool JobKmsBackend::createGbm()
{
    if (!m_drmDevice || !m_drmDevice->isOpen() || !m_mode.valid() || !m_plane.valid())
        return false;

    m_gbmDevice = JobGbmDevice::createShared();

    if (!m_gbmDevice || !m_gbmDevice->create(m_drmDevice->fd()))
        return false;

    return true;
}

bool JobKmsBackend::createEgl()
{
    if (!m_gbmDevice || !m_gbmDevice->valid() || !m_plane.valid() || !m_mode.valid())
        return false;

    m_eglDisplay = JobEglDisplay::createShared();

    if (!m_eglDisplay || !m_eglDisplay->initialize(m_gbmDevice->device()))
        return false;

    constexpr JobGbmSurface::Usage usage = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
    const auto negotiation = JobKmsFormatNegotiator::negotiate(*m_gbmDevice,
                                                               m_eglDisplay->display(),
                                                               m_plane,
                                                               width(),
                                                               height(),
                                                               usage,
                                                               m_plane.formats(),
                                                               JobEglConfig::Requirements{});

    if (!negotiation || !negotiation->valid())
        return false;

    m_format = negotiation->format;
    m_explicitModifiers = negotiation->explicitModifiers();

    m_gbmSurface = negotiation->surface;
    m_eglConfig = JobEglConfig::createShared(negotiation->eglConfig);

    if (!m_gbmSurface || !m_gbmSurface->valid() || !m_eglConfig || !m_eglConfig->valid())
        return false;

    m_eglContext = JobEglContext::createShared();
    if (!m_eglContext || !m_eglContext->create(m_eglDisplay->display(), *m_eglConfig, EglClientApi::OpenGl))
        return false;

    m_eglSurface = JobEglSurface::createShared();
    if (!m_eglSurface || !m_eglSurface->create(m_eglDisplay->display(), *m_eglConfig, m_gbmSurface->surface()))
        return false;

    if (!m_eglContext->makeCurrent(
            m_eglSurface->surface(),
            m_eglSurface->surface())) {
        return false;
    }

    return true;
}

bool JobKmsBackend::createPresenter()
{
    if (!m_drmDevice || !m_mode.valid() || m_connector.id() == 0 || m_crtc.id() == 0 || m_plane.id() == 0)
        return false;

    JobKmsPresenter::Configuration configuration{
        .connectorId = m_connector.id(),
        .crtcId = m_crtc.id(),
        .planeId = m_plane.id(),
        .mode = m_mode,
        .source = {
            .x = 0,
            .y = 0,
            .width = width(),
            .height = height()
        },
        .destination = {
            .x = 0,
            .y = 0,
            .width = width(),
            .height = height()
        }
    };

    m_presenter = JobKmsPresenter::createShared();
    if (!m_presenter)
        return false;

    if (!m_presenter->initialize(m_drmDevice.get(), std::move(configuration), m_connector, m_crtc, m_plane)) {
        m_presenter.reset();
        return false;
    }

    return true;
}

void JobKmsBackend::resetDisplayState() noexcept
{
    m_connector = {};
    m_crtc = {};
    m_plane = {};
    m_mode = {};
}

} // namespace job::render