#include "job_kms_presenter.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <xf86drmMode.h>

namespace job::render {

JobKmsPresenter::~JobKmsPresenter()
{
    reset();
}

bool JobKmsPresenter::initialize(JobDrmDevice *device, Configuration configuration, const JobDrmConnector &connector, const JobDrmCrtc &crtc, const JobDrmPlane &plane)
{
    reset();

    if (!device || !device->isOpen() || !configuration.valid() || !connector.valid() || !crtc.valid() || !plane.valid())
        return false;

    if (configuration.connectorId != connector.id() ||
        configuration.crtcId != crtc.id() ||
        configuration.planeId != plane.id()) {
        return false;
    }

    m_device = device;
    m_configuration = std::move(configuration);

    if (!resolveProperties()) {
        reset();
        return false;
    }

    if (!captureRestoreState(connector, crtc, plane)) {
        reset();
        return false;
    }

    if (!createModeBlob()) {
        reset();
        return false;
    }

    m_modesetRequired = true;

    return true;
}

// Order matters here
void JobKmsPresenter::reset() noexcept
{
    m_queue.clear();

    if (m_current && !restoreScanout())
        (void)disableScanout();

    m_current.reset();

    destroyModeBlob();

    m_restore = {};
    m_properties = {};
    m_configuration = {};

    m_device = nullptr;
    m_modesetRequired = true;
}

bool JobKmsPresenter::enqueue(JobPresentEntry entry)
{
    if (!valid() || !entryCompatible(entry))
        return false;

    return m_queue.push(std::move(entry));
}

bool JobKmsPresenter::presentNext()
{
    if (!valid() || m_queue.empty())
        return false;

    JobPresentEntry entry = m_queue.pop();

    if (!entryCompatible(entry))
        return false;

    if (!waitForAcquireFence(entry.frame()))
        return false;

    const BlobId damageBlobId = createDamageBlob(entry.frame());

    if (entry.frame().hasDamage() && !entry.frame().damage()->empty() && m_properties.planeDamageClips != 0 && damageBlobId == 0)
        return false;

    JobDrmAtomicState state;

    if (m_modesetRequired && !buildInitialState(state)) {
        destroyBlob(damageBlobId);
        return false;
    }

    if (!buildFrameState(entry.frame(), state, damageBlobId)) {
        destroyBlob(damageBlobId);
        return false;
    }

    JobDrmAtomicRequest request;

    if (!request.build(state)) {
        destroyBlob(damageBlobId);
        return false;
    }

    JobDrmAtomicRequest::Flags flags = 0;

    if (m_modesetRequired)
        flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;

    if (!request.test(m_device->fd(), flags)) {
        destroyBlob(damageBlobId);
        return false;
    }

    if (!request.commit(m_device->fd(), flags)) {
        destroyBlob(damageBlobId);
        return false;
    }

    destroyBlob(damageBlobId);
    entry.frame().setAcquireFence({});

    /*
     *
     * The new framebuffer has replaced the previous scanout before commit()
     * returns. Releasing the previous entry is therefore safe here.
     *
     * JobPresentEntry owns both the DRM framebuffer registration and the GBM
     * buffer lease. Destroying the old entry removes the DRM framebuffer
     * before returning the backing GBM buffer to its surface.
     */
    m_current.reset();
    m_current.emplace(std::move(entry));

    m_modesetRequired = false;

    return true;
}

bool JobKmsPresenter::resolveProperties()
{
    if (!m_device)
        return false;

    const auto connectorProperties  = m_device->objectProperties(m_configuration.connectorId, DRM_MODE_OBJECT_CONNECTOR);
    const auto crtcProperties       = m_device->objectProperties(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC);
    const auto planeProperties      = m_device->objectProperties(m_configuration.planeId, DRM_MODE_OBJECT_PLANE);
    if (!connectorProperties || !crtcProperties || !planeProperties)
        return false;

    const auto propertyId = [](const JobDrmObjectProperties &properties, std::string_view name) -> PropertyId {
        const auto *entry = properties.find(name);
        if (!entry)
            return 0;

        return entry->id();
    };

    PropertySet properties;

    properties.connectorCrtcId      = propertyId(*connectorProperties, "CRTC_ID");

    properties.crtcActive           = propertyId(*crtcProperties,   "ACTIVE");
    properties.crtcModeId           = propertyId(*crtcProperties,   "MODE_ID");
    properties.planeFramebufferId   = propertyId(*planeProperties,  "FB_ID");
    properties.planeCrtcId          = propertyId(*planeProperties,  "CRTC_ID");
    properties.planeSourceX         = propertyId(*planeProperties,  "SRC_X");
    properties.planeSourceY         = propertyId(*planeProperties,  "SRC_Y");
    properties.planeSourceWidth     = propertyId(*planeProperties,  "SRC_W");
    properties.planeSourceHeight    = propertyId(*planeProperties,  "SRC_H");
    properties.planeCrtcX           = propertyId(*planeProperties,  "CRTC_X");
    properties.planeCrtcY           = propertyId(*planeProperties,  "CRTC_Y");
    properties.planeCrtcWidth       = propertyId(*planeProperties,  "CRTC_W");
    properties.planeCrtcHeight      = propertyId(*planeProperties,  "CRTC_H");
    properties.planeInFenceFd       = propertyId(*planeProperties,  "IN_FENCE_FD");
    properties.planeDamageClips     = propertyId(*planeProperties,  "FB_DAMAGE_CLIPS");

    if (!properties.valid())
        return false;

    m_properties = properties;

    return true;
}


bool JobKmsPresenter::captureRestoreState(const JobDrmConnector &connector, const JobDrmCrtc &crtc, const JobDrmPlane &plane)
{
    const auto value = [](const JobDrmObjectProperties &properties, std::string_view name, std::uint64_t &destination) {
        const auto propertyValue = properties.value(name);
        if (!propertyValue)
            return false;

        destination = static_cast<std::uint64_t>(*propertyValue);
        return true;
    };

    JobPresentRestore restore;

    if (!value(connector.properties(), "CRTC_ID", restore.connectorCrtcId))
        return false;

    if (!value(crtc.properties(), "ACTIVE", restore.crtcActive))
        return false;

    if (!value(plane.properties(), "FB_ID", restore.planeFramebufferId))
        return false;

    if (!value(plane.properties(), "CRTC_ID", restore.planeCrtcId))
        return false;

    if (!value(plane.properties(), "SRC_X", restore.planeSourceX))
        return false;

    if (!value(plane.properties(), "SRC_Y", restore.planeSourceY))
        return false;

    if (!value(plane.properties(), "SRC_W", restore.planeSourceWidth))
        return false;

    if (!value(plane.properties(), "SRC_H", restore.planeSourceHeight))
        return false;

    if (!value(plane.properties(), "CRTC_X", restore.planeCrtcX))
        return false;

    if (!value(plane.properties(), "CRTC_Y", restore.planeCrtcY))
        return false;

    if (!value(plane.properties(), "CRTC_W", restore.planeCrtcWidth))
        return false;

    if (!value(plane.properties(), "CRTC_H", restore.planeCrtcHeight))
        return false;

    restore.modeValid = crtc.modeValid();

    if (restore.modeValid)
        restore.mode = crtc.mode();

    if (restore.crtcActive != 0 && !restore.modeValid)
        return false;

    if (restore.connectorCrtcId != 0 && restore.connectorCrtcId != crtc.id())
        return false;

    if (restore.planeCrtcId != 0 && restore.planeCrtcId != crtc.id())
        return false;

    m_restore = std::move(restore);

    return true;
}

bool JobKmsPresenter::createModeBlob()
{
    destroyModeBlob();

    if (!m_device)
        return false;

    const drmModeModeInfo mode = m_configuration.mode.toDrm();
    const auto *data = reinterpret_cast<const std::byte *>(&mode);

    const std::span<const std::byte> bytes{
        data,
        sizeof(mode)
    };

    const auto blob = m_device->createPropertyBlob(bytes);

    if (!blob)
        return false;

    m_modeBlobId = blob->id();

    return m_modeBlobId != 0;
}

void JobKmsPresenter::destroyModeBlob() noexcept
{
    if (!m_device || m_modeBlobId == 0) {
        m_modeBlobId = 0;
        return;
    }

    (void)m_device->destroyPropertyBlob(m_modeBlobId);

    m_modeBlobId = 0;
}

bool JobKmsPresenter::buildInitialState(JobDrmAtomicState &state) const
{
    if (!valid() || m_modeBlobId == 0)
        return false;

    if (!state.set(m_configuration.connectorId, DRM_MODE_OBJECT_CONNECTOR, m_properties.connectorCrtcId, m_configuration.crtcId))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcActive, 1))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcModeId, m_modeBlobId))
        return false;

    return true;
}

bool JobKmsPresenter::buildDisabledState(JobDrmAtomicState &state) const
{
    if (!valid())
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeFramebufferId, 0))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcId, 0))
        return false;

    if (!state.set(m_configuration.connectorId, DRM_MODE_OBJECT_CONNECTOR, m_properties.connectorCrtcId, 0))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcActive, 0))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcModeId, 0))
        return false;

    return true;
}

bool JobKmsPresenter::buildRestoreState(JobDrmAtomicState &state, BlobId restoreModeBlobId) const
{
    if (!valid())
        return false;

    if (m_restore.modeValid && restoreModeBlobId == 0)
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeFramebufferId, m_restore.planeFramebufferId))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcId, m_restore.planeCrtcId))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceX, m_restore.planeSourceX))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceY, m_restore.planeSourceY))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceWidth, m_restore.planeSourceWidth))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceHeight, m_restore.planeSourceHeight))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcX, m_restore.planeCrtcX))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcY, m_restore.planeCrtcY))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcWidth, m_restore.planeCrtcWidth))
        return false;

    if (!state.set( m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcHeight, m_restore.planeCrtcHeight))
        return false;

    if (!state.set(m_configuration.connectorId, DRM_MODE_OBJECT_CONNECTOR, m_properties.connectorCrtcId, m_restore.connectorCrtcId))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcActive, m_restore.crtcActive))
        return false;

    if (!state.set(m_configuration.crtcId, DRM_MODE_OBJECT_CRTC, m_properties.crtcModeId, m_restore.modeValid ? restoreModeBlobId : 0))
        return false;

    return true;
}


bool JobKmsPresenter::buildFrameState(const JobPresentFrame &frame, JobDrmAtomicState &state, BlobId damageBlobId) const
{
    if (!valid() || !frame.valid())
        return false;

    const auto &source = m_configuration.source;
    const auto &destination = m_configuration.destination;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeFramebufferId, frame.framebufferId()))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcId, m_configuration.crtcId))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceX, sourceCoordinate(source.x)))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceY, sourceCoordinate(source.y)))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceWidth, sourceCoordinate(source.width)))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeSourceHeight, sourceCoordinate(source.height)))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcX, static_cast<std::uint64_t>( static_cast<std::int64_t>( destination.x))))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcY, static_cast<std::uint64_t>( static_cast<std::int64_t>( destination.y))))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcWidth, destination.width))
        return false;

    if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeCrtcHeight, destination.height))
        return false;


    if (m_properties.planeInFenceFd != 0) {
        const int fenceFd = frame.acquireFence().valid() ? frame.acquireFence().fd() : -1;
        if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeInFenceFd, static_cast<std::uint64_t>( static_cast<std::int64_t>( fenceFd))))
            return false;
    }

    if (m_properties.planeDamageClips != 0) {
        if (!state.set(m_configuration.planeId, DRM_MODE_OBJECT_PLANE, m_properties.planeDamageClips, damageBlobId))
            return false;
    }

    return true;
}

JobKmsPresenter::BlobId JobKmsPresenter::createDamageBlob(const JobPresentFrame &frame)
{
    if (!m_device || m_properties.planeDamageClips == 0 || !frame.hasDamage())
        return 0;

    const auto &damage = *frame.damage();

    if (damage.empty())
        return 0;

    std::vector<drm_mode_rect> rectangles;
    rectangles.reserve(damage.count());

    for (const auto &rect : damage.rects()) {
        if (!rect.valid())
            return 0;

        rectangles.push_back(rect.toDrm());
    }

    const auto *data = reinterpret_cast<const std::byte *>(rectangles.data());
    const std::span<const std::byte> bytes{ data, rectangles.size() * sizeof(drm_mode_rect) };
    const auto blob = m_device->createPropertyBlob(bytes);

    if (!blob)
        return 0;

    return blob->id();
}

bool JobKmsPresenter::entryCompatible(const JobPresentEntry &entry) const noexcept
{
    if (!entry.valid())
        return false;

    const auto &source = m_configuration.source;
    const auto &lease = entry.lease();

    if (source.x > lease.width() || source.y > lease.height())
        return false;

    if (source.width > lease.width() - source.x)
        return false;

    if (source.height > lease.height() - source.y)
        return false;

    return true;
}

bool JobKmsPresenter::waitForAcquireFence(const JobPresentFrame &frame) const noexcept
{
    if (!frame.acquireFence().valid() || m_properties.planeInFenceFd != 0)
        return true;

    return frame.acquireFence().wait();
}


bool JobKmsPresenter::restoreScanout() noexcept
{
    if (!valid())
        return false;

    BlobId restoreModeBlobId = 0;
    if (m_restore.modeValid) {
        const drmModeModeInfo mode = m_restore.mode.toDrm();
        const auto *data = reinterpret_cast<const std::byte *>(&mode);
        const std::span<const std::byte> bytes{data, sizeof(mode)};

        const auto blob = m_device->createPropertyBlob(bytes);
        if (!blob)
            return false;

        restoreModeBlobId = blob->id();

        if (restoreModeBlobId == 0)
            return false;
    }

    JobDrmAtomicState state;
    if (!buildRestoreState(state, restoreModeBlobId)) {
        (void)destroyBlob(restoreModeBlobId);
        return false;
    }

    JobDrmAtomicRequest request;
    if (!request.build(state)) {
        (void)destroyBlob(restoreModeBlobId);
        return false;
    }

    constexpr JobDrmAtomicRequest::Flags flags = DRM_MODE_ATOMIC_ALLOW_MODESET;
    if (!request.test(m_device->fd(), flags)) {
        (void)destroyBlob(restoreModeBlobId);
        return false;
    }

    if (!request.commit(m_device->fd(), flags)) {
        (void)destroyBlob(restoreModeBlobId);
        return false;
    }

    (void)destroyBlob(restoreModeBlobId);
    m_modesetRequired = true;

    return true;
}


bool JobKmsPresenter::disableScanout() noexcept
{
    if (!valid())
        return false;

    JobDrmAtomicState state;
    if (!buildDisabledState(state))
        return false;

    JobDrmAtomicRequest request;
    if (!request.build(state))
        return false;

    if (!request.commit(m_device->fd(), DRM_MODE_ATOMIC_ALLOW_MODESET))
        return false;

    m_modesetRequired = true;
    return true;
}

void JobKmsPresenter::destroyBlob(BlobId blobId) noexcept
{
    if (!m_device || blobId == 0)
        return;

    (void)m_device->destroyPropertyBlob(blobId);
}

}