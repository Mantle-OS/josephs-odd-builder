#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputRelativeAxis
{
public:
    inline static constexpr JobInputCode kRelativeAxisMax = REL_MAX;  // #define REL_MAX 0x0f
    inline static constexpr std::size_t kRelativeAxisCount = REL_CNT; // #define REL_CNT (REL_MAX + 1)

    constexpr JobInputRelativeAxis() noexcept = delete;
    ~JobInputRelativeAxis() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kRelativeAxisMax)
            return false;

        return x(code)              ||
               y(code)              ||
               z(code)              ||
               rx(code)             ||
               ry(code)             ||
               rz(code)             ||
               hWheel(code)         ||
               dial(code)           ||
               wheel(code)          ||
               misc(code)           ||
               reserved(code)       ||
               wheelHiRes(code)     ||
               hWheelHiRes(code);
    }

    [[nodiscard]] static constexpr bool x(JobInputCode code) noexcept
    {
        // #define REL_X 0x00
        return code == REL_X;
    }

    [[nodiscard]] static constexpr bool y(JobInputCode code) noexcept
    {
        // #define REL_Y 0x01
        return code == REL_Y;
    }

    [[nodiscard]] static constexpr bool z(JobInputCode code) noexcept
    {
        // #define REL_Z 0x02
        return code == REL_Z;
    }

    [[nodiscard]] static constexpr bool rx(JobInputCode code) noexcept
    {
        // #define REL_RX 0x03
        return code == REL_RX;
    }

    [[nodiscard]] static constexpr bool ry(JobInputCode code) noexcept
    {
        // #define REL_RY 0x04
        return code == REL_RY;
    }

    [[nodiscard]] static constexpr bool rz(JobInputCode code) noexcept
    {
        // #define REL_RZ 0x05
        return code == REL_RZ;
    }

    [[nodiscard]] static constexpr bool hWheel(JobInputCode code) noexcept
    {
        // #define REL_HWHEEL 0x06
        return code == REL_HWHEEL;
    }

    [[nodiscard]] static constexpr bool dial(JobInputCode code) noexcept
    {
        // #define REL_DIAL 0x07
        return code == REL_DIAL;
    }

    [[nodiscard]] static constexpr bool wheel(JobInputCode code) noexcept
    {
        // #define REL_WHEEL 0x08
        return code == REL_WHEEL;
    }

    [[nodiscard]] static constexpr bool misc(JobInputCode code) noexcept
    {
        // #define REL_MISC 0x09
        return code == REL_MISC;
    }

    [[nodiscard]] static constexpr bool reserved(JobInputCode code) noexcept
    {
        // #define REL_RESERVED 0x0a
        return code == REL_RESERVED;
    }

    [[nodiscard]] static constexpr bool wheelHiRes(JobInputCode code) noexcept
    {
        // #define REL_WHEEL_HI_RES 0x0b
        return code == REL_WHEEL_HI_RES;
    }

    [[nodiscard]] static constexpr bool hWheelHiRes(JobInputCode code) noexcept
    {
        // #define REL_HWHEEL_HI_RES 0x0c
        return code == REL_HWHEEL_HI_RES;
    }
};

} // namespace job::input