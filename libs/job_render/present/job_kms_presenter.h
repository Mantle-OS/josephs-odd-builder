#pragma once

#include <cstdint>
#include <memory>
#include <optional>

#include "job_drm_atomic_request.h"
#include "job_drm_atomic_state.h"
#include "job_drm_connector.h"
#include "job_drm_crtc.h"
#include "job_drm_device.h"
#include "job_drm_mode.h"
#include "job_drm_plane.h"

#include "job_kms_presenter_restore.h"
#include "job_present_entry.h"
#include "job_present_queue.h"

namespace job::render {

class JobKmsPresenter
{
public:
    using Ptr  = std::shared_ptr<JobKmsPresenter>;
    using WPtr = std::weak_ptr<JobKmsPresenter>;
    using UPtr = std::unique_ptr<JobKmsPresenter>;

    using ConnectorId = JobDrmConnector::Id;
    using CrtcId      = JobDrmCrtc::Id;
    using PlaneId     = JobDrmPlane::Id;
    using BlobId      = JobDrmPropertyBlob::Id;
    using PropertyId  = JobDrmProperty::Id;

    struct SourceRect
    {
        std::uint32_t x{};
        std::uint32_t y{};
        std::uint32_t width{};
        std::uint32_t height{};

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return width != 0 && height != 0;
        }

        bool operator==(const SourceRect &) const noexcept = default;
    };

    struct DestinationRect
    {
        std::int32_t x{};
        std::int32_t y{};
        std::uint32_t width{};
        std::uint32_t height{};

        [[nodiscard]] constexpr bool valid() const noexcept
        {
            return width != 0 && height != 0;
        }

        bool operator==(const DestinationRect &) const noexcept = default;
    };

    struct Configuration
    {
        ConnectorId connectorId{};
        CrtcId crtcId{};
        PlaneId planeId{};
        JobDrmMode mode;
        SourceRect source;
        DestinationRect destination;

        [[nodiscard]] bool valid() const noexcept
        {
            return connectorId != 0 &&
                   crtcId != 0 &&
                   planeId != 0 &&
                   mode.valid() &&
                   source.valid() &&
                   destination.valid();
        }
    };

    JobKmsPresenter() = default;
    ~JobKmsPresenter();

    JobKmsPresenter(const JobKmsPresenter &) = delete;
    JobKmsPresenter &operator=(const JobKmsPresenter &) = delete;

    JobKmsPresenter(JobKmsPresenter &&) = delete;
    JobKmsPresenter &operator=(JobKmsPresenter &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobKmsPresenter>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobKmsPresenter>();
    }

    [[nodiscard]] bool initialize(
        JobDrmDevice *device,
        Configuration configuration,
        const JobDrmConnector &connector,
        const JobDrmCrtc &crtc,
        const JobDrmPlane &plane);

    void reset() noexcept;

    [[nodiscard]] bool enqueue(JobPresentEntry entry);
    [[nodiscard]] bool presentNext();

    [[nodiscard]] bool hasCurrentFrame() const noexcept
    {
        return m_current.has_value();
    }

    [[nodiscard]] JobPresentEntry *current() noexcept
    {
        if (!m_current)
            return nullptr;

        return &*m_current;
    }

    [[nodiscard]] const JobPresentEntry *current() const noexcept
    {
        if (!m_current)
            return nullptr;

        return &*m_current;
    }

    [[nodiscard]] JobPresentQueue &queue() noexcept
    {
        return m_queue;
    }

    [[nodiscard]] const JobPresentQueue &queue() const noexcept
    {
        return m_queue;
    }

    [[nodiscard]] const Configuration &configuration() const noexcept
    {
        return m_configuration;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_device != nullptr && m_configuration.valid() && m_properties.valid();
    }

private:
    struct PropertySet
    {
        PropertyId connectorCrtcId{};

        PropertyId crtcActive{};
        PropertyId crtcModeId{};

        PropertyId planeFramebufferId{};
        PropertyId planeCrtcId{};

        PropertyId planeSourceX{};
        PropertyId planeSourceY{};
        PropertyId planeSourceWidth{};
        PropertyId planeSourceHeight{};

        PropertyId planeCrtcX{};
        PropertyId planeCrtcY{};
        PropertyId planeCrtcWidth{};
        PropertyId planeCrtcHeight{};

        PropertyId planeInFenceFd{};
        PropertyId planeDamageClips{};

        [[nodiscard]] bool valid() const noexcept
        {
            return connectorCrtcId != 0 &&
                   crtcActive != 0 &&
                   crtcModeId != 0 &&
                   planeFramebufferId != 0 &&
                   planeCrtcId != 0 &&
                   planeSourceX != 0 &&
                   planeSourceY != 0 &&
                   planeSourceWidth != 0 &&
                   planeSourceHeight != 0 &&
                   planeCrtcX != 0 &&
                   planeCrtcY != 0 &&
                   planeCrtcWidth != 0 &&
                   planeCrtcHeight != 0;
        }
    };

    [[nodiscard]] bool resolveProperties();

    [[nodiscard]] bool captureRestoreState(const JobDrmConnector &connector, const JobDrmCrtc &crtc, const JobDrmPlane &plane);

    [[nodiscard]] bool createModeBlob();

    void destroyModeBlob() noexcept;

    [[nodiscard]] bool buildInitialState(JobDrmAtomicState &state) const;

    [[nodiscard]] bool buildDisabledState(JobDrmAtomicState &state) const;

    [[nodiscard]] bool buildRestoreState(JobDrmAtomicState &state, BlobId restoreModeBlobId) const;

    [[nodiscard]] bool buildFrameState(const JobPresentFrame &frame, JobDrmAtomicState &state, BlobId damageBlobId) const;

    [[nodiscard]] BlobId createDamageBlob(const JobPresentFrame &frame);

    [[nodiscard]] bool entryCompatible(const JobPresentEntry &entry) const noexcept;
    [[nodiscard]] bool waitForAcquireFence(const JobPresentFrame &frame) const noexcept;

    [[nodiscard]] bool restoreScanout() noexcept;
    [[nodiscard]] bool disableScanout() noexcept;

    void destroyBlob(BlobId blobId) noexcept;

    [[nodiscard]] static std::uint64_t sourceCoordinate(std::uint32_t value) noexcept
    {
        return static_cast<std::uint64_t>(value) << 16;
    }

    JobDrmDevice *m_device{}; // BORROWED

    Configuration m_configuration;
    PropertySet m_properties;

    BlobId m_modeBlobId{};

    JobPresentRestore m_restore;

    JobPresentQueue m_queue;
    std::optional<JobPresentEntry> m_current;

    bool m_modesetRequired{true};
};

}