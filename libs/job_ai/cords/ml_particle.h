#pragma once

#include <vector>

#include <vec3f.h> // job::science::data

namespace job::ai::cords {
using Vec3 = science::data::Vec3f;

struct MLVec3f {
    float* data{nullptr};

    float& operator[](size_t i) noexcept { return data[i]; }
    const float& operator[](size_t i) const noexcept { return data[i]; }

    // Assign from science Vec3
    MLVec3f& operator=(const Vec3& v) noexcept
    {
        data[0] = v.x;
        data[1] = v.y;
        data[2] = v.z;
        return *this;
    }

    // Assign from {x,y,z} for convenience (used in computeNbodyForces)
    MLVec3f& operator=(std::initializer_list<float> xyz) noexcept
    {
        // Expect exactly 3 elements; in release we just write what we got.
        auto it = xyz.begin();
        data[0] = (it != xyz.end()) ? *it++ : 0.0f;
        data[1] = (it != xyz.end()) ? *it++ : 0.0f;
        data[2] = (it != xyz.end()) ? *it++ : 0.0f;
        return *this;
    }

    // Value conversion
    operator Vec3() const noexcept
    {
        return Vec3{ data[0], data[1], data[2] };
    }

    // Pure ops return a value Vec3 (not a view)
    [[nodiscard]] Vec3 operator+(const Vec3& rhs) const noexcept
    {
        return Vec3{ data[0] + rhs.x, data[1] + rhs.y, data[2] + rhs.z };
    }

    [[nodiscard]] Vec3 operator-(const Vec3& rhs) const noexcept
    {
        return Vec3{ data[0] - rhs.x, data[1] - rhs.y, data[2] - rhs.z };
    }

    [[nodiscard]] Vec3 operator*(float s) const noexcept
    {
        return Vec3{ data[0] * s, data[1] * s, data[2] * s };
    }

    // In-place ops mutate underlying tensor storage (the important part for VerletVector)
    MLVec3f& operator+=(const Vec3& o) noexcept
    {
        data[0] += o.x;
        data[1] += o.y;
        data[2] += o.z;
        return *this;
    }

    MLVec3f& operator-=(const Vec3& o) noexcept
    {
        data[0] -= o.x;
        data[1] -= o.y;
        data[2] -= o.z;
        return *this;
    }

    MLVec3f& operator*=(float s) noexcept
    {
        data[0] *= s;
        data[1] *= s;
        data[2] *= s;
        return *this;
    }
};

struct MLParticle {
    MLVec3f     pos;
    MLVec3f     vel;
    MLVec3f     acc;
    float       mass;
};


template<typename T_Particle>
inline void computeNbodyForces(std::vector<T_Particle> &particles) {
    const size_t n = particles.size();
    const float eps = 1e-5f;

    for (size_t i = 0; i < n; ++i) {
        // Reset Acceleration
        particles[i].acc = {0.0f, 0.0f, 0.0f}; // Write to side-buffer

        for (size_t j = i + 1; j < n; ++j) {
            auto &p1 = particles[i];
            auto &p2 = particles[j];

            // Read directly from Tensor memory via Proxy
            Vec3 pos1 = p1.pos;
            Vec3 pos2 = p2.pos;

            Vec3 r = {pos2.x - pos1.x, pos2.y - pos1.y, pos2.z - pos1.z};
            float distSq = r.x*r.x + r.y*r.y + r.z*r.z + eps;
            float dist = std::sqrt(distSq);
            float distCb = dist * distSq;

            float f1 = p2.mass / distCb;
            float f2 = p1.mass / distCb;

            // Write to side-buffer
            p1.acc[0] += r.x * f1; // x
            p1.acc[1] += r.y * f1; // y
            p1.acc[2] += r.z * f1; // z

            p2.acc[0] -= r.x * f2; // x
            p2.acc[0] -= r.y * f2; // y
            p2.acc[0] -= r.z * f2; // z
        }
    }
}






}
