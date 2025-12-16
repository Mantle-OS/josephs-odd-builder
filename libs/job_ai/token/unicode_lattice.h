#pragma once
#include "learn_config.h"
#include <algorithm>
namespace job::ai::token {
// the atom
struct alignas(16) UnicodeLattice {
    float x;
    float y;
    float z;
    float mass; // pad

    [[nodiscard]] static constexpr UnicodeLattice encode(char32_t c, float mass = 1.0f) noexcept
    {
        // 21 bits total -> 7 bits per axis
        float x = (float)(c & 0x7F);           // Low 7 bits
        float y = (float)((c >> 7) & 0x7F);    // Mid 7 bits
        float z = (float)((c >> 14) & 0x7F);   // High 7 bits
        // (val / 64.0) - 1.0
        return {
            (x * 0.015625f) - 1.0f,
            (y * 0.015625f) - 1.0f,
            (z * 0.015625f) - 1.0f,
            mass // Default weight/mass
        };
    }

    [[nodiscard]] static constexpr char32_t decode(const UnicodeLattice &p) noexcept
    {
        if (learn::punishLearner(p.x) || learn::punishLearner(p.y) || learn::punishLearner(p.z))
            return U'\uFFFD'; // replacement char, or 0

        int i_x = (int)((p.x + 1.0f) * 64.0f);
        int i_y = (int)((p.y + 1.0f) * 64.0f);
        int i_z = (int)((p.z + 1.0f) * 64.0f);

        i_x = std::clamp(i_x, 0, 127);
        i_y = std::clamp(i_y, 0, 127);
        i_z = std::clamp(i_z, 0, 127);

        return (char32_t)(i_x | (i_y << 7) | (i_z << 14));
    }
};

} // namespace
