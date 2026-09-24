#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include <io_base.h>

#include "job_drm_capabilities.h"
#include "job_drm_connector.h"
#include "job_drm_crtc.h"
#include "job_drm_encoder.h"
#include "job_drm_framebuffer.h"
#include "job_drm_framebuffer_registration.h"
#include "job_drm_object_properties.h"
#include "job_drm_plane.h"
#include "job_drm_plane_resources.h"
#include "job_drm_property_blob.h"
#include "job_drm_resources.h"

namespace job::render {

class JobDrmDevice final : public io::IODevice
{
public:
    using Ptr  = std::shared_ptr<JobDrmDevice>;
    using WPtr = std::weak_ptr<JobDrmDevice>;
    using UPtr = std::unique_ptr<JobDrmDevice>;

    using ObjectId      = JobDrmObjectProperties::ObjectId;
    using ObjectType    = JobDrmObjectProperties::ObjectType;
    using BlobId        = JobDrmPropertyBlob::Id;
    using ConnectorId   = JobDrmConnector::Id;
    using CrtcId        = JobDrmCrtc::Id;
    using EncoderId     = JobDrmEncoder::Id;
    using PlaneId       = JobDrmPlane::Id;
    using FramebufferId = JobDrmFramebuffer::Id;

    struct FramebufferCreateInfo
    {
        static constexpr std::size_t MaxPlanes = 4;

        std::uint32_t width{};
        std::uint32_t height{};

        JobDrmFormat format;

        std::array<std::uint32_t, MaxPlanes> handles{};
        std::array<std::uint32_t, MaxPlanes> pitches{};
        std::array<std::uint32_t, MaxPlanes> offsets{};

        std::size_t planeCount{};
        JobDrmModifier modifier = JobDrmModifier::invalid();

        [[nodiscard]] bool valid() const noexcept
        {
            if (width == 0 ||
                height == 0 ||
                !format.valid() ||
                planeCount == 0 ||
                planeCount > MaxPlanes) {
                return false;
            }

            for (std::size_t i = 0; i < planeCount; ++i) {
                if (handles[i] == 0 || pitches[i] == 0)
                    return false;
            }

            for (std::size_t i = planeCount; i < MaxPlanes; ++i) {
                if (handles[i] != 0 || pitches[i] != 0 || offsets[i] != 0)
                    return false;
            }

            return true;
        }

        [[nodiscard]] bool hasModifier() const noexcept
        {
            return modifier.valid();
        }
    };

    JobDrmDevice() = default;

    explicit JobDrmDevice(std::filesystem::path path) :
        m_path(std::move(path))
    {
    }

    ~JobDrmDevice() override;

    JobDrmDevice(const JobDrmDevice &) = delete;
    JobDrmDevice &operator=(const JobDrmDevice &) = delete;

    JobDrmDevice(JobDrmDevice &&) = delete;
    JobDrmDevice &operator=(JobDrmDevice &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmDevice>();
    }

    [[nodiscard]] static Ptr createShared(std::filesystem::path path)
    {
        return std::make_shared<JobDrmDevice>(std::move(path));
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmDevice>();
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path path)
    {
        return std::make_unique<JobDrmDevice>(std::move(path));
    }

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback callback) override;

    void setPath(std::filesystem::path path)
    {
        m_path = std::move(path);
    }

    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] const JobDrmCapabilities &capabilities() const noexcept
    {
        return m_capabilities;
    }

    [[nodiscard]] bool refreshCapabilities() noexcept;

    [[nodiscard]] std::optional<JobDrmResources> resources() const;
    [[nodiscard]] std::optional<JobDrmPlaneResources> planeResources() const;

    [[nodiscard]] std::optional<JobDrmConnector> connector(ConnectorId connectorId, bool forceProbe = false) const;

    [[nodiscard]] std::optional<JobDrmEncoder> encoder(EncoderId encoderId) const;

    [[nodiscard]] std::optional<JobDrmCrtc> crtc(CrtcId crtcId) const;

    [[nodiscard]] std::optional<JobDrmPlane> plane(PlaneId planeId) const;

    [[nodiscard]] std::optional<JobDrmFramebuffer> framebuffer(FramebufferId framebufferId) const;

    [[nodiscard]] std::optional<JobDrmObjectProperties> objectProperties(ObjectId objectId, ObjectType objectType) const;

    [[nodiscard]] std::optional<JobDrmPropertyBlob> propertyBlob(BlobId blobId) const;

    [[nodiscard]] std::optional<JobDrmPropertyBlob> createPropertyBlob(std::span<const std::byte> data);

    [[nodiscard]] std::optional<JobDrmFramebufferRegistration> createFramebuffer(const FramebufferCreateInfo &info);

    [[nodiscard]] bool destroyPropertyBlob(BlobId blobId) noexcept;

    [[nodiscard]] bool setClientCapability(std::uint64_t capability, std::uint64_t value) noexcept;

private:
    [[nodiscard]] static std::optional<JobDrmPlane::FormatModifiers>
    parseFormatModifiers(const JobDrmPropertyBlob &blob);

    std::filesystem::path m_path{"/dev/dri/card0"};
    int m_fd{-1};

    ReadCallback m_readCallback;

    JobDrmCapabilities m_capabilities;
};

} // namespace job::render