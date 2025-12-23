#pragma once
#include <algorithm>
#include <simd_provider.h>
namespace job::ai::comp {
using namespace job::simd;
// Maxout



struct FunctorMaxout {
    static inline f32 apply(f32 x, float /*alpha*/)
    {

    }
    static inline float apply_naive(float x, float /*alpha*/)
    {

    }
};


struct FunctorGDN{
    static inline f32 apply(f32 x, float /*alpha*/)
    {

    }
    static inline float apply_naive(float x, float /*alpha*/)
    {

    }
};



}
