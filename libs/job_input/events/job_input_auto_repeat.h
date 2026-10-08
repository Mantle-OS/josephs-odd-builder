#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputAutoRepeat
{
public:
    inline static constexpr JobInputCode kAutoRepeatMax = REP_MAX; // #define REP_MAX 0x01
    inline static constexpr std::size_t kAutoRepeatCount = REP_CNT;

    constexpr JobInputAutoRepeat() noexcept = delete;
    ~JobInputAutoRepeat() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        return delay(code) ||
               period(code);
    }

    [[nodiscard]] static constexpr bool delay(JobInputCode code) noexcept
    {
        // #define REP_DELAY 0x00
        return code == REP_DELAY;
    }

    [[nodiscard]] static constexpr bool period(JobInputCode code) noexcept
    {
        // #define REP_PERIOD 0x01
        return code == REP_PERIOD;
    }
};

} // namespace job::input