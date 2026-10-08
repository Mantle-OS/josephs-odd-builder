#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputMiscEvent
{
public:

    inline static constexpr JobInputCode kMiscEventMax = MSC_MAX; // #define MSC_MAX 0x07
    inline static constexpr std::size_t kMiscEventCount = MSC_CNT;

    constexpr JobInputMiscEvent() noexcept = delete;
    ~JobInputMiscEvent() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kMiscEventMax)
            return false;

        return serial(code)     ||
               pulseLed(code)   ||
               gesture(code)    ||
               raw(code)        ||
               scan(code)       ||
               timestamp(code);
    }

    [[nodiscard]] static constexpr bool serial(JobInputCode code) noexcept
    {
        // #define MSC_SERIAL 0x00
        return code == MSC_SERIAL;
    }

    [[nodiscard]] static constexpr bool pulseLed(JobInputCode code) noexcept
    {
        // #define MSC_PULSELED 0x01
        return code == MSC_PULSELED;
    }

    [[nodiscard]] static constexpr bool gesture(JobInputCode code) noexcept
    {
        // #define MSC_GESTURE 0x02
        return code == MSC_GESTURE;
    }

    [[nodiscard]] static constexpr bool raw(JobInputCode code) noexcept
    {
        // #define MSC_RAW 0x03
        return code == MSC_RAW;
    }

    [[nodiscard]] static constexpr bool scan(JobInputCode code) noexcept
    {
        // #define MSC_SCAN 0x04
        return code == MSC_SCAN;
    }

    [[nodiscard]] static constexpr bool timestamp(JobInputCode code) noexcept
    {
        // #define MSC_TIMESTAMP 0x05
        return code == MSC_TIMESTAMP;
    }
};

} // namespace job::input