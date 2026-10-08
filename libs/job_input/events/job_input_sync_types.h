#pragma once

#include <cstddef>
#include <linux/input-event-codes.h>
#include "job_input_traits.h"

namespace job::input {
class JobInputSyncType
{
public:
    inline static constexpr JobInputCode kSyncTypeMax = SYN_MAX;  // #define SYN_MAX 0x0f
    inline static constexpr std::size_t kSyncTypeCount = SYN_CNT; // #define SYN_CNT (SYN_MAX + 1)

    constexpr JobInputSyncType() noexcept = delete;
    ~JobInputSyncType() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kSyncTypeMax)
            return false;

        return report(code)     ||
               config(code)     ||
               mtReport(code)   ||
               dropped(code);
    }

    [[nodiscard]] static constexpr bool report(JobInputCode code) noexcept
    {
        // #define SYN_REPORT 0
        return code == SYN_REPORT;
    }

    [[nodiscard]] static constexpr bool config(JobInputCode code) noexcept
    {
        // #define SYN_CONFIG 1
        return code == SYN_CONFIG;
    }

    [[nodiscard]] static constexpr bool mtReport(JobInputCode code) noexcept
    {
        // #define SYN_MT_REPORT 2
        return code == SYN_MT_REPORT;
    }

    [[nodiscard]] static constexpr bool dropped(JobInputCode code) noexcept
    {
        // #define SYN_DROPPED 3
        return code == SYN_DROPPED;
    }
};

} // namespace job::input