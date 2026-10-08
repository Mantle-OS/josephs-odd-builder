#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputSoundEvent
{
public:
    inline static constexpr JobInputCode kSoundEventMax = SND_MAX; // #define SND_MAX 0x07
    inline static constexpr std::size_t kSoundEventCount = SND_CNT;

    constexpr JobInputSoundEvent() noexcept = delete;
    ~JobInputSoundEvent() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kSoundEventMax)
            return false;

        return click(code)  ||
               bell(code)   ||
               tone(code);
    }

    [[nodiscard]] static constexpr bool click(JobInputCode code) noexcept
    {
        // #define SND_CLICK 0x00
        return code == SND_CLICK;
    }

    [[nodiscard]] static constexpr bool bell(JobInputCode code) noexcept
    {
        // #define SND_BELL 0x01
        return code == SND_BELL;
    }

    [[nodiscard]] static constexpr bool tone(JobInputCode code) noexcept
    {
        // #define SND_TONE 0x02
        return code == SND_TONE;
    }
};

} // namespace job::input