#pragma once

#include <real_type.h>
#include <cmath>

namespace job::science::data {

#pragma pack(push, 1)
struct Vec3f final {
    core::real_t x{core::real_t(0)};
    core::real_t y{core::real_t(0)};
    core::real_t z{core::real_t(0)};

    [[nodiscard]] constexpr Vec3f operator+(const Vec3f &o) const noexcept
    {
        return {x + o.x, y + o.y, z + o.z};
    }

    [[nodiscard]] constexpr Vec3f operator-(const Vec3f &o) const noexcept
    {
        return {x - o.x, y - o.y, z - o.z};
    }

    [[nodiscard]] constexpr Vec3f operator*(core::real_t s) const noexcept
    {
        return {x * s, y * s, z * s};
    }

    // c++26 . . . . constexpr, noexcept FIXME when the day comes
    [[nodiscard]] core::real_t length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr core::real_t lengthSq() const noexcept
    {
        return x * x + y * y + z * z;
    }

    // c++26 . . . . constexpr, noexcept
    [[nodiscard]] Vec3f normalize() const
    {
        const core::real_t len = length();
        if (len == core::real_t(0))
            return *this;
        return {x / len, y / len, z / len};
    }
};
#pragma pack(pop)

inline constexpr Vec3f kEmptyVec3F{core::real_t(0), core::real_t(0), core::real_t(0)};
}
