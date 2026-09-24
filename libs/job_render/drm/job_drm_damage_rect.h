#pragma once

#include <cstdint>

#include <drm_mode.h>

#include "job_rect.h"

namespace job::render {

class JobDrmDamageRect
{
public:
    JobDrmDamageRect() = default;

    constexpr JobDrmDamageRect(std::int32_t x1, std::int32_t y1, std::int32_t x2, std::int32_t y2) noexcept :
        m_rect{x1, y1, x2, y2}
    {

    }

    explicit constexpr JobDrmDamageRect(JobRect rect) noexcept :
        m_rect(rect)
    {

    }

    ~JobDrmDamageRect() = default;

    JobDrmDamageRect(const JobDrmDamageRect &) = default;
    JobDrmDamageRect &operator=(const JobDrmDamageRect &) = default;

    JobDrmDamageRect(JobDrmDamageRect &&) noexcept = default;
    JobDrmDamageRect &operator=(JobDrmDamageRect &&) noexcept = default;

    [[nodiscard]] constexpr std::int32_t x1() const noexcept
    {
        return m_rect.x1;
    }

    [[nodiscard]] constexpr std::int32_t y1() const noexcept
    {
        return m_rect.y1;
    }

    [[nodiscard]] constexpr std::int32_t x2() const noexcept
    {
        return m_rect.x2;
    }

    [[nodiscard]] constexpr std::int32_t y2() const noexcept
    {
        return m_rect.y2;
    }

    [[nodiscard]] constexpr std::int32_t width() const noexcept
    {
        return m_rect.width();
    }

    [[nodiscard]] constexpr std::int32_t height() const noexcept
    {
        return m_rect.height();
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_rect.valid() && m_rect.x1 >= 0 && m_rect.y1 >= 0;
    }

    [[nodiscard]] constexpr const JobRect &rect() const noexcept
    {
        return m_rect;
    }

    [[nodiscard]] static constexpr JobDrmDamageRect fromDrm(const drm_mode_rect &rect) noexcept
    {
        return JobDrmDamageRect{ rect.x1, rect.y1, rect.x2, rect.y2 };
    }

    [[nodiscard]] constexpr drm_mode_rect toDrm() const noexcept
    {
        return drm_mode_rect{
            .x1 = m_rect.x1,
            .y1 = m_rect.y1,
            .x2 = m_rect.x2,
            .y2 = m_rect.y2
        };
    }

    [[nodiscard]] friend constexpr bool operator==(const JobDrmDamageRect &, const JobDrmDamageRect &) noexcept = default;

private:
    JobRect m_rect;
};
//FB_DAMAGE_CLIPS
}