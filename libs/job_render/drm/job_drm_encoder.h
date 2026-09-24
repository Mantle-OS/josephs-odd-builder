#pragma once

#include <cstdint>
#include <memory>

#include <xf86drmMode.h>

namespace job::render {

class JobDrmEncoder
{
public:
    using Ptr  = std::shared_ptr<JobDrmEncoder>;
    using WPtr = std::weak_ptr<JobDrmEncoder>;
    using UPtr = std::unique_ptr<JobDrmEncoder>;

    using Id       = std::uint32_t;
    using CrtcId   = std::uint32_t;
    using CrtcMask = std::uint32_t;

    JobDrmEncoder() = default;

    JobDrmEncoder(Id id,
                  std::uint32_t type,
                  CrtcId crtcId,
                  CrtcMask possibleCrtcs,
                  std::uint32_t possibleClones) :
        m_id(id),
        m_type(type),
        m_crtcId(crtcId),
        m_possibleCrtcs(possibleCrtcs),
        m_possibleClones(possibleClones)
    {
    }

    ~JobDrmEncoder() = default;

    JobDrmEncoder(const JobDrmEncoder &) = default;
    JobDrmEncoder &operator=(const JobDrmEncoder &) = default;

    JobDrmEncoder(JobDrmEncoder &&) noexcept = default;
    JobDrmEncoder &operator=(JobDrmEncoder &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(Id id,
                                          std::uint32_t type,
                                          CrtcId crtcId,
                                          CrtcMask possibleCrtcs,
                                          std::uint32_t possibleClones)
    {
        return std::make_shared<JobDrmEncoder>(
            id,
            type,
            crtcId,
            possibleCrtcs,
            possibleClones);
    }

    [[nodiscard]] static UPtr createUniq(Id id,
                                         std::uint32_t type,
                                         CrtcId crtcId,
                                         CrtcMask possibleCrtcs,
                                         std::uint32_t possibleClones)
    {
        return std::make_unique<JobDrmEncoder>(
            id,
            type,
            crtcId,
            possibleCrtcs,
            possibleClones);
    }

    [[nodiscard]] static JobDrmEncoder fromDrm(const drmModeEncoder &encoder)
    {
        return JobDrmEncoder{
            encoder.encoder_id,
            encoder.encoder_type,
            encoder.crtc_id,
            encoder.possible_crtcs,
            encoder.possible_clones
        };
    }

    [[nodiscard]] constexpr Id id() const noexcept
    {
        return m_id;
    }

    [[nodiscard]] constexpr std::uint32_t type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr CrtcId crtcId() const noexcept
    {
        return m_crtcId;
    }

    [[nodiscard]] constexpr CrtcMask possibleCrtcs() const noexcept
    {
        return m_possibleCrtcs;
    }

    [[nodiscard]] constexpr std::uint32_t possibleClones() const noexcept
    {
        return m_possibleClones;
    }

    [[nodiscard]] constexpr bool supportsCrtc(std::uint32_t crtcIndex) const noexcept
    {
        if (crtcIndex >= 32)
            return false;

        return (m_possibleCrtcs & (std::uint32_t{1} << crtcIndex)) != 0;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_id != 0;
    }

    [[nodiscard]] friend bool operator==(const JobDrmEncoder &, const JobDrmEncoder &) = default;

private:
    Id            m_id{};
    std::uint32_t m_type{};
    CrtcId        m_crtcId{};
    CrtcMask      m_possibleCrtcs{};
    std::uint32_t m_possibleClones{};
};

} // namespace job::render