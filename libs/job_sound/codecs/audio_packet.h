#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "jobsound_export.h"

namespace job::sound {

struct JOBSOUND_EXPORT AudioPacket {
    std::vector<std::uint8_t> data;
    std::int64_t pts{-1};          // Presentation timestamp (in sample frames or timebase ticks)
    std::int64_t duration{0};      // Frame duration represented by this packet
    bool isKeyframe{true};

    [[nodiscard]] bool empty() const noexcept
    {
        return data.empty();
    }
    [[nodiscard]] std::size_t size() const noexcept
    {
        return data.size();
    }
    [[nodiscard]] std::span<const std::uint8_t> span() const noexcept
    {
        return data;
    }
    void clear() noexcept
    {
        data.clear();
        pts = -1;
        duration = 0;
        isKeyframe = true;
    }
};

} // namespace job::sound