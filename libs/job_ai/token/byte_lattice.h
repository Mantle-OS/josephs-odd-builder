#pragma once
#include <cstdint>
#include <bit>
#include <cmath>
#include <algorithm>

#include "learn_config.h"

namespace job::ai::token {

// the atom (byte)
struct alignas(16) ByteLattice {
    float x;
    float y;
    float z;
    float mass; // pad

    [[nodiscard]] static constexpr ByteLattice encode(uint8_t b, float mass = 1.0f) noexcept
    {
        // 8 bits total -> 7 bits in x, 1 bit in y
        float x = (float)(b & 0x7F);        // Low 7 bits  (0..127)
        float y = (float)((b >> 7) & 0x01); // High 1 bit  (0..1)

        return {
            (x * 0.015625f) - 1.0f, // (val / 64.0) - 1.0 ; grid step 1/64
            (y * 0.015625f) - 1.0f, // same grid, tiny range
            -1.0f,                  // reserved lane (keep pinned)
            mass
        };
    }

    [[nodiscard]] static inline uint8_t decode(const ByteLattice &p) noexcept
    {
        // Nan/Inf guard
        if (learn::punishLearner(p.x) || learn::punishLearner(p.y) || learn::punishLearner(p.z))
            return 0;

        // Snap-to-grid using round-to-nearest.
        // This makes untouched lattice points decode exactly, and reduces drift sensitivity.
        int i_x = (int)std::lrintf((p.x + 1.0f) * 64.0f);
        int i_y = (int)std::lrintf((p.y + 1.0f) * 64.0f);

        i_x = std::clamp(i_x, 0, 127);
        i_y = std::clamp(i_y, 0, 1);

        return (uint8_t)(i_x | (i_y << 7));
    }
};

} // namespace job::ai::token
