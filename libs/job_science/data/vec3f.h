#pragma once

#include <cassert>
#include <real_type.h>
#include <cmath>

namespace job::science::data {

#pragma pack(push, 1)
struct Vec3f final {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    // Read/Write access (Reference)
    [[nodiscard]] float &operator[](size_t i) noexcept
    {
        assert(i < 3 && "Vec3f index out of bounds");
        if(i == 0)
            return x;
        if(i == 1)
            return y;
        return z;
    }

    // Read-only access (Value is fine for scalars, or const ref)
    [[nodiscard]] float operator[](size_t i) const noexcept
    {
        assert(i < 3 && "Vec3f index out of bounds");
        if(i == 0)
            return x;
        if(i == 1)
            return y;
        return z;
    }

    [[nodiscard]] constexpr Vec3f operator+(const Vec3f &o) const noexcept
    {
        return {x + o.x, y + o.y, z + o.z};
    }
    constexpr Vec3f& operator+=(const Vec3f& o) noexcept
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }


    [[nodiscard]] constexpr Vec3f operator-(const Vec3f &o) const noexcept
    {
        return {x - o.x, y - o.y, z - o.z};
    }
    constexpr Vec3f& operator-=(const Vec3f& o) noexcept
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }

    [[nodiscard]] constexpr Vec3f operator*(float s) const noexcept
    {
        return {x * s, y * s, z * s};
    }
    constexpr Vec3f& operator*=(float s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }

    // c++26 . . . . constexpr, noexcept FIXME when the day comes
    [[nodiscard]] float length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    [[nodiscard]] constexpr float lengthSq() const noexcept
    {
        return x * x + y * y + z * z;
    }

    // c++26 . . . . constexpr, noexcept
    [[nodiscard]] Vec3f normalize() const
    {
        const float len = length();
        if (len == 0.0f)
            return *this;
        return {x / len, y / len, z / len};
    }
};
#pragma pack(pop)

inline constexpr Vec3f kEmptyVec3F{0.0f, 0.0f, 0.0f};
}
