#pragma once

#include <cstdint>

#include "job_input_traits.h"
#include <linux/input-event-codes.h>


namespace job::input {

class JobInputEventType {
public:
    inline static constexpr JobInputType kEventTypeMax = EV_MAX;    // #define EV_MAX   0x1f
    inline static constexpr std::size_t kEventTypeCount = EV_CNT;   // #define EV_CNT   (EV_MAX+1)

    constexpr JobInputEventType() noexcept = delete;
    ~JobInputEventType() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputType type) noexcept
    {
        if (type > kEventTypeMax)
            return false;

        return sync(type)           ||
               key(type)            ||
               relative(type)       ||
               abs(type)            ||
               misc(type)           ||
               sw(type)             ||
               led(type)            ||
               sound(type)          ||
               repeat(type)         ||
               forceFeedback(type)  ||
               power(type)          ||
               forceFeedbackStatus(type);
    }

    [[nodiscard]] constexpr bool sync(JobInputType type) noexcept
    {
        // #define EV_SYN			0x00
        return type == EV_SYN;
    }

    [[nodiscard]] constexpr bool key(JobInputType type) noexcept
    {
        // #define EV_KEY			0x01
        return type == EV_KEY;
    }

    [[nodiscard]] constexpr bool relative(JobInputType type) noexcept
    {
        // #define EV_REL			0x02
        return type == EV_REL;
    }

    [[nodiscard]] constexpr bool abs(JobInputType type) noexcept
    {
        // #define EV_ABS			0x03
        return type == EV_ABS;
    }

    [[nodiscard]] constexpr bool misc(JobInputType type) noexcept
    {
        // #define EV_MSC			0x04
        return type == EV_MSC;
    }

    [[nodiscard]] constexpr bool sw(JobInputType type) noexcept
    {
        // #define EV_SW			0x05
        return type == EV_SW;
    }

    [[nodiscard]] constexpr bool led(JobInputType type) noexcept
    {
        // #define EV_LED			0x11
        return type == EV_LED;
    }

    [[nodiscard]] static constexpr bool sound(JobInputType type) noexcept
    {
        // #define EV_SND			0x12
        return type == EV_SND;
    }

    [[nodiscard]] static constexpr bool repeat(JobInputType type) noexcept
    {
        // #define EV_REP			0x14
        return type == EV_REP;
    }

    [[nodiscard]] static constexpr bool fastForward(JobInputType type) noexcept
    {
        // #define EV_FF			0x15
        return type == EV_FF;
    }

    [[nodiscard]] static constexpr bool power(JobInputType type) noexcept
    {
        // #define EV_PWR			0x16
        return type == EV_PWR;
    }

    [[nodiscard]] static constexpr bool fastForwardStatus(JobInputType type) noexcept
    {
        // #define EV_FF_STATUS		0x17
        return type == EV_FF_STATUS;
    }
};
} // namespace job::input
