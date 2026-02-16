#pragma once
#include <cassert>
#include <cmath>
#include <simd_provider.h>

namespace job::ai::comp {
using namespace job::simd;

template<bool T_SMOOTH>
struct FunctorSigmoid {

    static inline f32 apply(f32 x, [[maybe_unused]]float alpha = 1.0f)
    {
        assert(alpha == 1.0f && "Sigmod alpha should always be 1.0f");
        auto one = SIMD::set1(1.0f);
        auto zero = SIMD::zero();
        auto neg_x = SIMD::sub(zero, x);

        [[maybe_unused]] f32 e_neg_x;
        if constexpr (T_SMOOTH)
            e_neg_x = SIMD::exp(neg_x);
        else
            e_neg_x = SIMD::exp_fast(neg_x);

        auto denom = SIMD::add(one, e_neg_x);
        return SIMD::div(one, denom);
    }

    static inline float apply_naive(float x, [[maybe_unused]]float alpha = 1.0f)
    {
        return 1.0f / (1.0f + std::exp(-x));
    }
};
}
