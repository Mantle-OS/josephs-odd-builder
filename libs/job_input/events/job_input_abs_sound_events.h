#pragma once

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputAbsoluteSoundEvent
{
public:
    constexpr JobInputAbsoluteSoundEvent() noexcept = delete;
    ~JobInputAbsoluteSoundEvent() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputValue value) noexcept
    {
        return silent(value)    ||
               vibrate(value)   ||
               ring(value);
    }

    [[nodiscard]] static constexpr bool silent(JobInputValue value) noexcept
    {
        // #define SND_PROFILE_SILENT 0x00
        return value == SND_PROFILE_SILENT;
    }

    [[nodiscard]] static constexpr bool vibrate(JobInputValue value) noexcept
    {
        // #define SND_PROFILE_VIBRATE 0x01
        return value == SND_PROFILE_VIBRATE;
    }

    [[nodiscard]] static constexpr bool ring(JobInputValue value) noexcept
    {
        // #define SND_PROFILE_RING 0x02
        return value == SND_PROFILE_RING;
    }
};

} // namespace job::input