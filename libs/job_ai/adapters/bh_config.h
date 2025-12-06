#pragma once
#include <vec3f.h>
namespace job::ai::adapters {

struct BhTraits {
    using Vec3 = job::science::data::Vec3f;
    using Real = float;
    struct Body {
        Vec3 position;
        Vec3 acceleration;
        Real mass;
    };
};

struct BhConfig {
    float   theta           = 0.5f;         // Accuracy knob (0.0 = Exact O(N^2), 1.0 = Rough Approx)
    float   gravity         = 1.0f;         // Interaction strength (G)
    float   epsilon         = 1e-5f;        // Softening factor to prevent singularity at r=0
    int     dim_mapping[3]  = {0, 1, 2};    // Which embedding dims map to X, Y, Z
};
} // namespace job::ai::adapters
