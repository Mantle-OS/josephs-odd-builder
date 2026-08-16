#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

#include <real_type.h>
#include "job_tokenizer_types.h"
#include "jobtoken_export.h"
namespace job::token {

struct alignas(16) JOBTOKEN_EXPORT ByteLattice {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float mass = 1.0f;

    static constexpr float kGridStep64 = 0.015625f; // 1.0f / 64.0f

    // -------------------------------------------------------------------------
    // Raw 8-Bit Byte Atom Projection (Z pinned to -1.0f)
    // -------------------------------------------------------------------------
    [[nodiscard]] static constexpr ByteLattice encodeByte(uint8_t byteValue, float mass = 1.0f) noexcept
    {
        const float low7  = static_cast<float>(byteValue & 0x7F);        // Low 7 bits (0..127)
        const float high1 = static_cast<float>((byteValue >> 7) & 0x01); // High 1 bit (0..1)
        return {
            (low7  * kGridStep64) - 1.0f,
            (high1 * kGridStep64) - 1.0f,
            -1.0f, // Pinned raw byte lane
            mass
        };
    }

    [[nodiscard]] static inline uint8_t decodeByte(const ByteLattice& p) noexcept
    {
        if (!core::isSafeFinite(p.x) || !core::isSafeFinite(p.y) || !core::isSafeFinite(p.z))
            return 0;

        const int i_x = std::clamp(static_cast<int>(std::lrintf((p.x + 1.0f) * 64.0f)), 0, 127);
        const int i_y = std::clamp(static_cast<int>(std::lrintf((p.y + 1.0f) * 64.0f)), 0, 1);
        return static_cast<uint8_t>(i_x | (i_y << 7));
    }

    // -------------------------------------------------------------------------
    // 21-Bit Coordinate Projection (7 bits X, 7 bits Y, 7 bits Z)
    // -------------------------------------------------------------------------
    [[nodiscard]] static constexpr ByteLattice encode21Bit(uint32_t id, float mass = 1.0f) noexcept
    {
        const float c0 = static_cast<float>(id & 0x7Fu);
        const float c1 = static_cast<float>((id >> 7) & 0x7Fu);
        const float c2 = static_cast<float>((id >> 14) & 0x7Fu);

        return {
            (c0 * kGridStep64) - 1.0f,
            (c1 * kGridStep64) - 1.0f,
            (c2 * kGridStep64) - 1.0f,
            mass
        };
    }

    [[nodiscard]] static inline uint32_t decode21Bit(const ByteLattice& p) noexcept
    {
        if (!core::isSafeFinite(p.x) || !core::isSafeFinite(p.y) || !core::isSafeFinite(p.z))
            return 0;

        const int i0 = std::clamp(static_cast<int>(std::lrintf((p.x + 1.0f) * 64.0f)), 0, 127);
        const int i1 = std::clamp(static_cast<int>(std::lrintf((p.y + 1.0f) * 64.0f)), 0, 127);
        const int i2 = std::clamp(static_cast<int>(std::lrintf((p.z + 1.0f) * 64.0f)), 0, 127);
        return static_cast<uint32_t>(i0) |
               (static_cast<uint32_t>(i1) << 7) |
               (static_cast<uint32_t>(i2) << 14);
    }

    // -------------------------------------------------------------------------
    // Coordinate Inspection
    // -------------------------------------------------------------------------
    [[nodiscard]] constexpr bool isRawByteLane() const noexcept
    {
        return z < -0.5f;
    }
};

} // namespace job::token