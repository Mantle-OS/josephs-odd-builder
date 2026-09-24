#include "job_kms_simple_presenter.h"

#include <cerrno>
#include <cstring>
#include <utility>

#include <xf86drm.h>

#include <job_logger.h>

namespace job::render {

JobKmsSimplePresenter::JobKmsSimplePresenter(JobDrmSimpleDevice::Ptr device) :
    m_device(std::move(device))
{
}

JobKmsSimplePresenter::~JobKmsSimplePresenter()
{
    stop();
}

JobKmsSimplePresenter::Ptr JobKmsSimplePresenter::createShared(JobDrmSimpleDevice::Ptr device)
{
    return std::make_shared<JobKmsSimplePresenter>(std::move(device));
}

JobKmsSimplePresenter::UPtr JobKmsSimplePresenter::createUniq(JobDrmSimpleDevice::Ptr device)
{
    return std::make_unique<JobKmsSimplePresenter>(std::move(device));
}

bool JobKmsSimplePresenter::start(threads::JobIoAsyncThread::Ptr loop)
{
    if (m_running)
        return true;

    if (!m_device || !m_device->isOpen()) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] DRM device is not valid or open.");
        return false;
    }

    if (!loop || !loop->isRunning()) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] I/O event loop is not running.");
        return false;
    }

    if (!m_device->refreshResources()) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] Failed to refresh DRM resources.");
        return false;
    }

    if (!findDisplay()) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] Failed to find a usable KMS display.");
        return false;
    }

    m_loop = loop;

    if (!loop->registerFD(m_device->fd(), threads::IOEvent::Read, [this](threads::IOEvent events) { onEvents(events); })) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] Failed to register DRM fd with I/O event loop.");
        m_loop.reset();
        return false;
    }

    m_running = true;
    return true;
}

void JobKmsSimplePresenter::stop()
{
    if (auto loop = m_loop.lock())
        loop->unregisterFD(m_device ? m_device->fd() : -1);

    m_loop.reset();

    restore();

    m_pending.reset();
    m_current.reset();

    m_connector.reset();
    m_encoder.reset();
    m_savedCrtc.reset();

    m_mode = {};

    m_connectorId = 0;
    m_encoderId = 0;
    m_crtcId = 0;

    m_modeset = false;
    m_presentPending = false;
    m_running = false;
}

bool JobKmsSimplePresenter::refresh()
{
    if (!m_device || !m_device->isOpen())
        return false;

    if (m_presentPending)
        return false;

    restore();

    m_pending.reset();
    m_current.reset();

    m_connector.reset();
    m_encoder.reset();
    m_savedCrtc.reset();

    m_mode = {};

    m_connectorId = 0;
    m_encoderId = 0;
    m_crtcId = 0;

    m_modeset = false;

    if (!m_device->refreshResources())
        return false;

    return findDisplay();
}

bool JobKmsSimplePresenter::present(const JobKmsSimpleSurface::Ptr &surface)
{
    if (!m_running || !surface || !surface->valid())
        return false;

    if (surface->width() != width() || surface->height() != height()) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] Surface size {}x{} mismatch {}x{}.",
                      surface->width(), surface->height(),
                      width(), height());
        return false;
    }

    if (!m_modeset)
        return modeset(surface);

    if (m_presentPending)
        return false;

    return pageFlip(surface);
}

bool JobKmsSimplePresenter::presentPending() const noexcept
{
    return m_presentPending;
}

bool JobKmsSimplePresenter::running() const noexcept
{
    return m_running;
}

std::uint32_t JobKmsSimplePresenter::connectorId() const noexcept
{
    return m_connectorId;
}

std::uint32_t JobKmsSimplePresenter::encoderId() const noexcept
{
    return m_encoderId;
}

std::uint32_t JobKmsSimplePresenter::crtcId() const noexcept
{
    return m_crtcId;
}

const drmModeModeInfo &JobKmsSimplePresenter::mode() const noexcept
{
    return m_mode;
}

std::uint32_t JobKmsSimplePresenter::width() const noexcept
{
    return m_mode.hdisplay;
}

std::uint32_t JobKmsSimplePresenter::height() const noexcept
{
    return m_mode.vdisplay;
}

std::uint32_t JobKmsSimplePresenter::refreshRate() const noexcept
{
    return m_mode.vrefresh;
}

void JobKmsSimplePresenter::setPresentCallback(PresentCallback callback)
{
    m_presentCallback = std::move(callback);
}

void JobKmsSimplePresenter::DrmModeConnectorDeleter::operator()(drmModeConnector *connector) const noexcept
{
    if (connector)
        drmModeFreeConnector(connector);
}

void JobKmsSimplePresenter::DrmModeEncoderDeleter::operator()(drmModeEncoder *encoder) const noexcept
{
    if (encoder)
        drmModeFreeEncoder(encoder);
}

void JobKmsSimplePresenter::DrmModeCrtcDeleter::operator()(drmModeCrtc *crtc) const noexcept
{
    if (crtc)
        drmModeFreeCrtc(crtc);
}

bool JobKmsSimplePresenter::findDisplay()
{
    if (!findConnector())
        return false;

    if (!findEncoder())
        return false;

    if (!findCrtc())
        return false;

    if (!findMode())
        return false;

    return true;
}

bool JobKmsSimplePresenter::findConnector()
{
    m_connector.reset();
    m_connectorId = 0;

    for (const std::uint32_t connectorId : m_device->connectors()) {
        DrmModeConnectorPtr connector{drmModeGetConnector(m_device->fd(), connectorId)};
        if (!connector)
            continue;

        if (connector->connection != DRM_MODE_CONNECTED)
            continue;

        if (connector->count_modes <= 0)
            continue;

        m_connectorId = connectorId;
        m_connector = std::move(connector);

        return true;
    }

    JOB_LOG_ERROR("[JobKmsSimplePresenter] No connected DRM connector with display modes found.");
    return false;
}

bool JobKmsSimplePresenter::findEncoder()
{
    m_encoder.reset();
    m_encoderId = 0;

    if (!m_connector)
        return false;

    if (m_connector->encoder_id != 0) {
        DrmModeEncoderPtr encoder{
            drmModeGetEncoder(m_device->fd(), m_connector->encoder_id)
        };

        if (encoder) {
            m_encoderId = encoder->encoder_id;
            m_encoder = std::move(encoder);
            return true;
        }
    }

    for (int i = 0; i < m_connector->count_encoders; ++i) {
        const std::uint32_t encoderId = m_connector->encoders[i];

        DrmModeEncoderPtr encoder{
            drmModeGetEncoder(m_device->fd(), encoderId)
        };

        if (!encoder)
            continue;

        m_encoderId = encoderId;
        m_encoder = std::move(encoder);

        return true;
    }

    JOB_LOG_ERROR("[JobKmsSimplePresenter] No usable encoder found for connector {}.", m_connectorId);
    return false;
}

bool JobKmsSimplePresenter::findCrtc()
{
    if (!m_encoder)
        return false;

    std::uint32_t selectedCrtc = 0;

    if (m_encoder->crtc_id != 0)
        selectedCrtc = m_encoder->crtc_id;

    if (selectedCrtc == 0) {
        const auto crtcs = m_device->crtcs();

        for (std::size_t i = 0; i < crtcs.size(); ++i) {
            if ((m_encoder->possible_crtcs & (1u << i)) == 0)
                continue;

            selectedCrtc = crtcs[i];
            break;
        }
    }

    if (selectedCrtc == 0) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] No usable CRTC found for encoder {}.", m_encoderId);
        return false;
    }

    m_crtcId = selectedCrtc;

    if (!m_savedCrtc) {
        m_savedCrtc.reset(drmModeGetCrtc(m_device->fd(), m_crtcId));
        if (!m_savedCrtc) {
            JOB_LOG_ERROR("[JobKmsSimplePresenter] Failed to read CRTC {} state: {}", m_crtcId, std::strerror(errno));
            return false;
        }
    }

    return true;
}

bool JobKmsSimplePresenter::findMode()
{
    if (!m_connector || m_connector->count_modes <= 0)
        return false;

    const drmModeModeInfo *selected = nullptr;

    for (int i = 0; i < m_connector->count_modes; ++i) {
        const drmModeModeInfo &mode = m_connector->modes[i];

        if ((mode.type & DRM_MODE_TYPE_PREFERRED) != 0) {
            selected = &mode;
            break;
        }
    }

    if (!selected)
        selected = &m_connector->modes[0];

    m_mode = *selected;
    return true;
}

bool JobKmsSimplePresenter::modeset(const JobKmsSimpleSurface::Ptr &surface)
{
    if (!surface || !surface->valid())
        return false;

    if (drmModeSetCrtc(m_device->fd(),
                       m_crtcId,
                       surface->framebufferId(),
                       0, 0,
                       &m_connectorId,
                       1,
                       &m_mode) != 0) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] drmModeSetCrtc failed: {}", std::strerror(errno));
        return false;
    }

    m_current = surface;
    m_modeset = true;

    return true;
}

bool JobKmsSimplePresenter::pageFlip(const JobKmsSimpleSurface::Ptr &surface)
{
    if (!surface || !surface->valid() || m_presentPending)
        return false;

    if (drmModePageFlip(m_device->fd(), m_crtcId, surface->framebufferId(), DRM_MODE_PAGE_FLIP_EVENT, this) != 0) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] drmModePageFlip failed: {}", std::strerror(errno));
        return false;
    }

    m_pending = surface;
    m_presentPending = true;

    return true;
}

void JobKmsSimplePresenter::restore()
{
    if (!m_device || !m_device->isOpen() || !m_savedCrtc)
        return;

    if (!m_modeset)
        return;

    drmModeCrtc *saved = m_savedCrtc.get();
    drmModeModeInfo *mode = saved->mode_valid ? &saved->mode : nullptr;

    if (drmModeSetCrtc(m_device->fd(),
                       saved->crtc_id,
                       saved->buffer_id,
                       saved->x, saved->y,
                       &m_connectorId,
                       1,
                       mode) != 0) {
        JOB_LOG_WARN("[JobKmsSimplePresenter] Failed to restore CRTC {}: {}", saved->crtc_id, std::strerror(errno));
    }

    m_modeset = false;
}

void JobKmsSimplePresenter::onEvents(threads::IOEvent events)
{
    if ((events & threads::IOEvent::Error) == threads::IOEvent::Error) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] DRM fd reported an error.");
        return;
    }

    if ((events & threads::IOEvent::HangUp) == threads::IOEvent::HangUp) {
        JOB_LOG_ERROR("[JobKmsSimplePresenter] DRM fd reported a hangup.");
        return;
    }

    if ((events & threads::IOEvent::Read) == threads::IOEvent::Read)
        handleEvents();
}

void JobKmsSimplePresenter::handleEvents()
{
    if (!m_device || !m_device->isOpen())
        return;

    drmEventContext context{};

    context.version = DRM_EVENT_CONTEXT_VERSION;
    context.page_flip_handler = &JobKmsSimplePresenter::pageFlipHandler;

    if (drmHandleEvent(m_device->fd(), &context) != 0)
        JOB_LOG_ERROR("[JobKmsSimplePresenter] drmHandleEvent failed: {}", std::strerror(errno));
}

void JobKmsSimplePresenter::pageFlipHandler(int, unsigned int, unsigned int, unsigned int, void *userData)
{
    auto *presenter = static_cast<JobKmsSimplePresenter *>(userData);
    if (presenter)
        presenter->pageFlipped();
}

void JobKmsSimplePresenter::pageFlipped()
{
    if (!m_presentPending)
        return;

    m_current = std::move(m_pending);
    m_pending.reset();

    m_presentPending = false;

    if (m_presentCallback)
        m_presentCallback();
}

} // namespace job::render