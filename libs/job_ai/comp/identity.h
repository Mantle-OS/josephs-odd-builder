#pragma once
#include <simd_provider.h>
namespace job::ai::comp {
using namespace job::simd;
template<bool Smooth>
struct FunctorIdentity {
    static inline f32 apply(f32 x, [[maybe_unused]] float alpha = 1.0f)
    {
        return x;
    }

    static inline float apply_naive(float x, [[maybe_unused]] float alpha = 1.0f)
    {
        return x;
    }
};
}
