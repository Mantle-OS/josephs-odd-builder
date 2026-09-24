#include "job_drm_device.h"

#include <array>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <utility>
#include <vector>

#include <drm.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace job::render {

JobDrmDevice::~JobDrmDevice()
{
    closeDevice();
}

bool JobDrmDevice::openDevice()
{
    if (isOpen())
        return true;

    if (m_path.empty())
        return false;

    m_fd = ::open(m_path.c_str(), O_RDWR | O_CLOEXEC);

    if (m_fd < 0)
        return false;

    if (!refreshCapabilities()) {
        closeDevice();
        return false;
    }

    return true;
}

void JobDrmDevice::closeDevice()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_capabilities.clear();
}

ssize_t JobDrmDevice::read(char *buffer, std::size_t maxlen)
{
    if (!isOpen() || !buffer || maxlen == 0)
        return -1;

    return ::read(m_fd, buffer, maxlen);
}

ssize_t JobDrmDevice::write(const char *data, std::size_t len)
{
    if (!isOpen() || !data || len == 0)
        return -1;

    return ::write(m_fd, data, len);
}

bool JobDrmDevice::isOpen() const
{
    return m_fd >= 0;
}

int JobDrmDevice::fd() const
{
    return m_fd;
}

void JobDrmDevice::setNonBlocking(bool enabled)
{
    if (!isOpen())
        return;

    const int flags = ::fcntl(m_fd, F_GETFL);

    if (flags < 0)
        return;

    int newFlags = flags;

    if (enabled)
        newFlags |= O_NONBLOCK;
    else
        newFlags &= ~O_NONBLOCK;

    if (newFlags != flags)
        ::fcntl(m_fd, F_SETFL, newFlags);
}

void JobDrmDevice::setReadCallback(ReadCallback callback)
{
    m_readCallback = std::move(callback);
}

bool JobDrmDevice::refreshCapabilities() noexcept
{
    if (!isOpen()) {
        m_capabilities.clear();
        return false;
    }

    return m_capabilities.query(m_fd);
}

std::optional<JobDrmResources> JobDrmDevice::resources() const
{
    if (!isOpen())
        return std::nullopt;

    drmModeResPtr resources = drmModeGetResources(m_fd);
    if (!resources)
        return std::nullopt;

    JobDrmResources result = JobDrmResources::fromDrm(*resources);

    drmModeFreeResources(resources);

    return result;
}

std::optional<JobDrmPlaneResources> JobDrmDevice::planeResources() const
{
    if (!isOpen())
        return std::nullopt;

    drmModePlaneResPtr resources = drmModeGetPlaneResources(m_fd);
    if (!resources)
        return std::nullopt;

    JobDrmPlaneResources result = JobDrmPlaneResources::fromDrm(*resources);

    drmModeFreePlaneResources(resources);

    return result;
}

std::optional<JobDrmConnector> JobDrmDevice::connector(ConnectorId connectorId, bool forceProbe) const
{
    if (!isOpen() || connectorId == 0)
        return std::nullopt;

    drm_mode_get_connector connector{};
    connector.connector_id = connectorId;

    drm_mode_modeinfo temporaryMode{};

    if (forceProbe) {
        connector.count_modes = 0;
        connector.modes_ptr = 0;
    } else {
        connector.count_modes = 1;
        connector.modes_ptr = reinterpret_cast<std::uint64_t>(&temporaryMode);
    }

    connector.count_props = 0;
    connector.count_encoders = 0;

    if (drmIoctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) != 0)
        return std::nullopt;

    for (;;) {
        const std::uint32_t modeCount = connector.count_modes;
        const std::uint32_t propertyCount = connector.count_props;
        const std::uint32_t encoderCount = connector.count_encoders;

        std::vector<drm_mode_modeinfo> modes(modeCount);
        std::vector<std::uint32_t> propertyIds(propertyCount);
        std::vector<std::uint64_t> propertyValues(propertyCount);
        std::vector<std::uint32_t> encoderIds(encoderCount);

        connector.count_modes = modeCount;
        connector.count_props = propertyCount;
        connector.count_encoders = encoderCount;

        connector.modes_ptr = modes.empty() ? 0 : reinterpret_cast<std::uint64_t>(modes.data());
        connector.props_ptr = propertyIds.empty() ? 0 : reinterpret_cast<std::uint64_t>(propertyIds.data());
        connector.prop_values_ptr = propertyValues.empty() ? 0 : reinterpret_cast<std::uint64_t>(propertyValues.data());
        connector.encoders_ptr = encoderIds.empty() ? 0 : reinterpret_cast<std::uint64_t>(encoderIds.data());

        if (drmIoctl(m_fd, DRM_IOCTL_MODE_GETCONNECTOR, &connector) != 0)
            return std::nullopt;

        if (connector.count_modes != modeCount ||
            connector.count_props != propertyCount ||
            connector.count_encoders != encoderCount) {
            continue;
        }

        JobDrmConnector::Modes jobModes;
        jobModes.reserve(modes.size());

        for (const auto &mode : modes) {
            const std::size_t nameLength = ::strnlen(mode.name, DRM_DISPLAY_MODE_LEN);

            jobModes.emplace_back(
                mode.clock,
                mode.hdisplay,
                mode.hsync_start,
                mode.hsync_end,
                mode.htotal,
                mode.hskew,
                mode.vdisplay,
                mode.vsync_start,
                mode.vsync_end,
                mode.vtotal,
                mode.vscan,
                mode.vrefresh,
                mode.flags,
                mode.type,
                std::string{mode.name, nameLength});
        }

        JobDrmObjectProperties::Entries entries;
        entries.reserve(propertyIds.size());

        for (std::size_t i = 0; i < propertyIds.size(); ++i) {
            drmModePropertyPtr property = drmModeGetProperty(m_fd, propertyIds[i]);

            if (!property)
                continue;

            entries.push_back({
                .property = JobDrmProperty::fromDrm(*property),
                .value = propertyValues[i]
            });

            drmModeFreeProperty(property);
        }

        JobDrmObjectProperties properties{
            connector.connector_id,
            DRM_MODE_OBJECT_CONNECTOR,
            std::move(entries)
        };

        JobDrmConnector::EncoderIds jobEncoders{
            encoderIds.begin(),
            encoderIds.end()
        };

        return JobDrmConnector{
            connector.connector_id,
            connector.encoder_id,
            connector.connector_type,
            connector.connector_type_id,
            JobDrmConnector::statusFromDrm(connector.connection),
            connector.mm_width,
            connector.mm_height,
            JobDrmConnector::subpixelOrderFromDrm(connector.subpixel),
            std::move(jobEncoders),
            std::move(jobModes),
            std::move(properties)
        };
    }
}

std::optional<JobDrmEncoder> JobDrmDevice::encoder(EncoderId encoderId) const
{
    if (!isOpen() || encoderId == 0)
        return std::nullopt;

    drmModeEncoderPtr encoder = drmModeGetEncoder(m_fd, encoderId);
    if (!encoder)
        return std::nullopt;

    JobDrmEncoder result = JobDrmEncoder::fromDrm(*encoder);

    drmModeFreeEncoder(encoder);

    return result;
}

std::optional<JobDrmCrtc> JobDrmDevice::crtc(CrtcId crtcId) const
{
    if (!isOpen() || crtcId == 0)
        return std::nullopt;

    drmModeCrtcPtr crtc = drmModeGetCrtc(m_fd, crtcId);
    if (!crtc)
        return std::nullopt;

    auto properties = objectProperties(crtcId, DRM_MODE_OBJECT_CRTC);
    if (!properties) {
        drmModeFreeCrtc(crtc);
        return std::nullopt;
    }

    JobDrmCrtc result = JobDrmCrtc::fromDrm(*crtc, std::move(*properties));

    drmModeFreeCrtc(crtc);

    return result;
}

std::optional<JobDrmPlane> JobDrmDevice::plane(PlaneId planeId) const
{
    if (!isOpen() || planeId == 0)
        return std::nullopt;

    drmModePlanePtr plane = drmModeGetPlane(m_fd, planeId);
    if (!plane)
        return std::nullopt;

    auto properties = objectProperties(planeId, DRM_MODE_OBJECT_PLANE);
    if (!properties) {
        drmModeFreePlane(plane);
        return std::nullopt;
    }

    JobDrmPlane::FormatModifiers formatModifiers;
    bool explicitModifiers = false;

    if (const auto *entry = properties->find("IN_FORMATS"); entry && entry->value != 0) {
        auto blob = propertyBlob(static_cast<BlobId>(entry->value));

        if (!blob) {
            drmModeFreePlane(plane);
            return std::nullopt;
        }

        auto parsed = parseFormatModifiers(*blob);

        if (!parsed) {
            drmModeFreePlane(plane);
            return std::nullopt;
        }

        formatModifiers = std::move(*parsed);
        explicitModifiers = true;
    }

    JobDrmPlane result = JobDrmPlane::fromDrm(
        *plane,
        std::move(formatModifiers),
        explicitModifiers,
        std::move(*properties));

    drmModeFreePlane(plane);

    return result;
}

std::optional<JobDrmPlane::FormatModifiers> JobDrmDevice::parseFormatModifiers(
    const JobDrmPropertyBlob &blob)
{
    const auto data = blob.data();

    if (data.size() < sizeof(drm_format_modifier_blob))
        return std::nullopt;

    drm_format_modifier_blob header{};
    std::memcpy(&header, data.data(), sizeof(header));

    if (header.version != FORMAT_BLOB_CURRENT)
        return std::nullopt;

    const std::size_t formatsOffset = header.formats_offset;
    const std::size_t modifiersOffset = header.modifiers_offset;
    const std::size_t formatCount = header.count_formats;
    const std::size_t modifierCount = header.count_modifiers;

    if (formatsOffset > data.size() ||
        formatCount > (data.size() - formatsOffset) / sizeof(std::uint32_t)) {
        return std::nullopt;
    }

    if (modifiersOffset > data.size() ||
        modifierCount > (data.size() - modifiersOffset) / sizeof(drm_format_modifier)) {
        return std::nullopt;
    }

    std::vector<std::uint32_t> formats(formatCount);

    if (!formats.empty()) {
        std::memcpy(
            formats.data(),
            data.data() + formatsOffset,
            formats.size() * sizeof(std::uint32_t));
    }

    JobDrmPlane::FormatModifiers result;

    for (std::size_t modifierIndex = 0; modifierIndex < modifierCount; ++modifierIndex) {
        drm_format_modifier modifier{};

        const std::size_t offset =
            modifiersOffset + modifierIndex * sizeof(drm_format_modifier);

        std::memcpy(
            &modifier,
            data.data() + offset,
            sizeof(modifier));

        for (std::uint32_t bit = 0; bit < 64; ++bit) {
            if ((modifier.formats & (std::uint64_t{1} << bit)) == 0)
                continue;

            const std::size_t formatIndex =
                static_cast<std::size_t>(modifier.offset) + bit;

            if (formatIndex >= formats.size())
                return std::nullopt;

            const JobDrmFormatModifier candidate{
                JobDrmFormat{formats[formatIndex]},
                JobDrmModifier{modifier.modifier}
            };

            if (!candidate.valid())
                return std::nullopt;

            bool duplicate = false;

            for (const auto &existing : result) {
                if (existing == candidate) {
                    duplicate = true;
                    break;
                }
            }

            if (!duplicate)
                result.push_back(candidate);
        }
    }

    return result;
}

std::optional<JobDrmFramebuffer> JobDrmDevice::framebuffer(
    FramebufferId framebufferId) const
{
    if (!isOpen() || framebufferId == 0)
        return std::nullopt;

    drmModeFB2Ptr framebuffer = drmModeGetFB2(m_fd, framebufferId);

    if (!framebuffer)
        return std::nullopt;

    JobDrmFramebuffer result = JobDrmFramebuffer::fromDrm(*framebuffer);

    drmModeFreeFB2(framebuffer);

    return result;
}

std::optional<JobDrmFramebufferRegistration> JobDrmDevice::createFramebuffer(
    const FramebufferCreateInfo &info)
{
    if (!isOpen() || !info.valid())
        return std::nullopt;

    FramebufferId framebufferId{};

    if (info.hasModifier()) {
        std::array<std::uint64_t, FramebufferCreateInfo::MaxPlanes> modifiers{};

        for (std::size_t i = 0; i < info.planeCount; ++i)
            modifiers[i] = info.modifier.value();

        if (drmModeAddFB2WithModifiers(
                m_fd,
                info.width,
                info.height,
                info.format.value(),
                info.handles.data(),
                info.pitches.data(),
                info.offsets.data(),
                modifiers.data(),
                &framebufferId,
                DRM_MODE_FB_MODIFIERS) != 0) {
            return std::nullopt;
        }
    } else {
        if (drmModeAddFB2(
                m_fd,
                info.width,
                info.height,
                info.format.value(),
                info.handles.data(),
                info.pitches.data(),
                info.offsets.data(),
                &framebufferId,
                0) != 0) {
            return std::nullopt;
        }
    }

    if (framebufferId == 0)
        return std::nullopt;

    return JobDrmFramebufferRegistration{
        m_fd,
        framebufferId
    };
}

std::optional<JobDrmObjectProperties> JobDrmDevice::objectProperties(
    ObjectId objectId,
    ObjectType objectType) const
{
    if (!isOpen() ||
        objectId == 0 ||
        objectType == 0) {
        return std::nullopt;
    }

    drmModeObjectPropertiesPtr properties = drmModeObjectGetProperties(
        m_fd,
        objectId,
        objectType);

    if (!properties)
        return std::nullopt;

    JobDrmObjectProperties::Entries entries;
    entries.reserve(properties->count_props);

    for (std::uint32_t i = 0; i < properties->count_props; ++i) {
        drmModePropertyPtr property = drmModeGetProperty(
            m_fd,
            properties->props[i]);

        if (!property)
            continue;

        entries.push_back({
            .property = JobDrmProperty::fromDrm(*property),
            .value = properties->prop_values[i]
        });

        drmModeFreeProperty(property);
    }

    drmModeFreeObjectProperties(properties);

    return JobDrmObjectProperties{
        objectId,
        objectType,
        std::move(entries)
    };
}

std::optional<JobDrmPropertyBlob> JobDrmDevice::propertyBlob(
    BlobId blobId) const
{
    if (!isOpen() || blobId == 0)
        return std::nullopt;

    drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blobId);

    if (!blob)
        return std::nullopt;

    JobDrmPropertyBlob::Data data;
    data.resize(blob->length);

    if (blob->length != 0 && blob->data)
        std::memcpy(data.data(), blob->data, blob->length);

    JobDrmPropertyBlob result{
        blob->id,
        std::move(data)
    };

    drmModeFreePropertyBlob(blob);

    return result;
}

std::optional<JobDrmPropertyBlob> JobDrmDevice::createPropertyBlob(
    std::span<const std::byte> data)
{
    if (!isOpen() || data.empty())
        return std::nullopt;

    BlobId blobId{};

    if (drmModeCreatePropertyBlob(
            m_fd,
            data.data(),
            data.size(),
            &blobId) != 0) {
        return std::nullopt;
    }

    JobDrmPropertyBlob::Data blobData{
        data.begin(),
        data.end()
    };

    return JobDrmPropertyBlob{
        blobId,
        std::move(blobData)
    };
}

bool JobDrmDevice::destroyPropertyBlob(BlobId blobId) noexcept
{
    if (!isOpen() || blobId == 0)
        return false;

    return drmModeDestroyPropertyBlob(
               m_fd,
               blobId) == 0;
}

bool JobDrmDevice::setClientCapability(
    std::uint64_t capability,
    std::uint64_t value) noexcept
{
    if (!isOpen())
        return false;

    return drmSetClientCap(
               m_fd,
               capability,
               value) == 0;
}

} // namespace job::render