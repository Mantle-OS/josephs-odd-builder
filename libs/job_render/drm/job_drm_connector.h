#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include <drm_mode.h>
#include <xf86drmMode.h>

#include "job_drm_mode.h"
#include "job_drm_object_properties.h"

namespace job::render {

enum class DrmConnectorStatus : std::uint8_t
{
    Unknown,
    Connected,
    Disconnected
};

enum class DrmSubpixelOrder : std::uint8_t
{
    Unknown,
    HorizontalRgb,
    HorizontalBgr,
    VerticalRgb,
    VerticalBgr,
    None
};

class JobDrmConnector
{
public:
    using Ptr  = std::shared_ptr<JobDrmConnector>;
    using WPtr = std::weak_ptr<JobDrmConnector>;
    using UPtr = std::unique_ptr<JobDrmConnector>;

    using Id            = std::uint32_t;
    using EncoderId     = std::uint32_t;
    using Type          = std::uint32_t;
    using TypeId        = std::uint32_t;
    using EncoderIds    = std::vector<EncoderId>;
    using Modes         = std::vector<JobDrmMode>;

    JobDrmConnector() = default;

    JobDrmConnector(Id id, EncoderId encoderId, Type type, TypeId typeId, DrmConnectorStatus status,
                    std::uint32_t widthMillimeters, std::uint32_t heightMillimeters,
                    DrmSubpixelOrder subpixelOrder, EncoderIds encoders, Modes modes, JobDrmObjectProperties properties) :
        m_id(id),
        m_encoderId(encoderId),
        m_type(type),
        m_typeId(typeId),
        m_status(status),
        m_widthMillimeters(widthMillimeters),
        m_heightMillimeters(heightMillimeters),
        m_subpixelOrder(subpixelOrder),
        m_encoders(std::move(encoders)),
        m_modes(std::move(modes)),
        m_properties(std::move(properties))
    {
    }

    ~JobDrmConnector() = default;

    JobDrmConnector(const JobDrmConnector &) = default;
    JobDrmConnector &operator=(const JobDrmConnector &) = default;

    JobDrmConnector(JobDrmConnector &&) noexcept = default;
    JobDrmConnector &operator=(JobDrmConnector &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          EncoderId encoderId,
                                          Type type,
                                          TypeId typeId,
                                          DrmConnectorStatus status,
                                          std::uint32_t widthMillimeters,
                                          std::uint32_t heightMillimeters,
                                          DrmSubpixelOrder subpixelOrder,
                                          EncoderIds encoders,
                                          Modes modes,
                                          JobDrmObjectProperties properties)
    {
        return std::make_shared<JobDrmConnector>(id,
                                                 encoderId,
                                                 type,
                                                 typeId,
                                                 status,
                                                 widthMillimeters,
                                                 heightMillimeters,
                                                 subpixelOrder,
                                                 std::move(encoders),
                                                 std::move(modes),
                                                 std::move(properties));
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         EncoderId encoderId,
                                         Type type,
                                         TypeId typeId,
                                         DrmConnectorStatus status,
                                         std::uint32_t widthMillimeters,
                                         std::uint32_t heightMillimeters,
                                         DrmSubpixelOrder subpixelOrder,
                                         EncoderIds encoders,
                                         Modes modes,
                                         JobDrmObjectProperties properties)
    {
        return std::make_unique<JobDrmConnector>(id,
                                                 encoderId,
                                                 type,
                                                 typeId,
                                                 status,
                                                 widthMillimeters,
                                                 heightMillimeters,
                                                 subpixelOrder,
                                                 std::move(encoders),
                                                 std::move(modes),
                                                 std::move(properties));
    }

    [[nodiscard]] static JobDrmConnector fromDrm(const drmModeConnector &connector, JobDrmObjectProperties properties)
    {
        EncoderIds encoders;
        encoders.reserve(connector.count_encoders);

        for (int i = 0; i < connector.count_encoders; ++i)
            encoders.push_back(connector.encoders[i]);

        Modes modes;
        modes.reserve(connector.count_modes);

        for (int i = 0; i < connector.count_modes; ++i)
            modes.push_back(JobDrmMode::fromDrm(connector.modes[i]));

        return JobDrmConnector{
            connector.connector_id,
            connector.encoder_id,
            connector.connector_type,
            connector.connector_type_id,
            statusFromDrm(connector.connection),
            connector.mmWidth,
            connector.mmHeight,
            subpixelOrderFromDrm(connector.subpixel),
            std::move(encoders),
            std::move(modes),
            std::move(properties)
        };
    }


    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr EncoderId encoderId() const noexcept
    {
        return m_encoderId;
    }

    [[nodiscard]] constexpr Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr TypeId typeId() const noexcept
    {
        return m_typeId;
    }

    [[nodiscard]] constexpr DrmConnectorStatus status() const noexcept
    {
        return m_status;
    }

    [[nodiscard]] constexpr bool connected() const noexcept
    {
        return m_status == DrmConnectorStatus::Connected;
    }

    [[nodiscard]] constexpr std::uint32_t widthMillimeters() const noexcept
    {
        return m_widthMillimeters;
    }

    [[nodiscard]] constexpr std::uint32_t heightMillimeters() const noexcept
    {
        return m_heightMillimeters;
    }

    [[nodiscard]] constexpr DrmSubpixelOrder subpixelOrder() const noexcept
    {
        return m_subpixelOrder;
    }

    [[nodiscard]] std::span<const EncoderId> encoders() const noexcept
    {
        return m_encoders;
    }

    [[nodiscard]] std::span<const JobDrmMode> modes() const noexcept
    {
        return m_modes;
    }

    [[nodiscard]] const JobDrmObjectProperties &properties() const noexcept
    {
        return m_properties;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] friend bool operator==(const JobDrmConnector &, const JobDrmConnector &) = default;

    [[nodiscard]] static constexpr DrmConnectorStatus statusFromDrm(std::uint32_t status) noexcept
    {
        switch (status) {
        case DRM_MODE_CONNECTED:
            return DrmConnectorStatus::Connected;

        case DRM_MODE_DISCONNECTED:
            return DrmConnectorStatus::Disconnected;

        case DRM_MODE_UNKNOWNCONNECTION:
        default:
            return DrmConnectorStatus::Unknown;
        }
    }

    [[nodiscard]] static constexpr DrmSubpixelOrder subpixelOrderFromDrm(std::uint32_t subpixel) noexcept
    {
        switch (subpixel) {
        case DRM_MODE_SUBPIXEL_HORIZONTAL_RGB:
            return DrmSubpixelOrder::HorizontalRgb;

        case DRM_MODE_SUBPIXEL_HORIZONTAL_BGR:
            return DrmSubpixelOrder::HorizontalBgr;

        case DRM_MODE_SUBPIXEL_VERTICAL_RGB:
            return DrmSubpixelOrder::VerticalRgb;

        case DRM_MODE_SUBPIXEL_VERTICAL_BGR:
            return DrmSubpixelOrder::VerticalBgr;

        case DRM_MODE_SUBPIXEL_NONE:
            return DrmSubpixelOrder::None;

        case DRM_MODE_SUBPIXEL_UNKNOWN:
        default:
            return DrmSubpixelOrder::Unknown;
        }
    }

private:
    Id                      m_id{};
    EncoderId               m_encoderId{};
    Type                    m_type{};
    TypeId                  m_typeId{};
    DrmConnectorStatus      m_status{DrmConnectorStatus::Unknown};
    std::uint32_t           m_widthMillimeters{};
    std::uint32_t           m_heightMillimeters{};
    DrmSubpixelOrder        m_subpixelOrder{DrmSubpixelOrder::Unknown};
    EncoderIds              m_encoders;
    Modes                   m_modes;
    JobDrmObjectProperties  m_properties;
};

}