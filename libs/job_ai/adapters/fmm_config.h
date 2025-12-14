#pragma once
#include <cstdint>
#include <vec3f.h>
namespace job::ai::adapters {

struct FmmTraits {
    using Vec3 = job::science::data::Vec3f;
    using Real = float;
    struct Body {
        Vec3        position{0,0,0};        // From Embedding [d0, d1, d2]
        Vec3        acceleration{0,0,0};    // The Resulting "Force" (Context Vector)
        Real        mass{0.0f};             // The "Attention Weight" / Charge
        uint32_t    id{0};                  // Original Token Index (to map back to Tensor)
    };

    static constexpr int P = 3; // not really text book P=3

    static Vec3 &position(Body &b)
    {
        return b.position;
    }

    static const Vec3 &position(const Body &b)
    {
        return b.position;
    }

    static Real &mass(Body &b)
    {
        return b.mass;
    }
    static const Real &mass(const Body &b)
    {
        return b.mass;
    }

    // Accumulate force into the body
    static void applyForce(Body &b, const Vec3 &f)
    {
        b.acceleration = b.acceleration + f;
    }
};

struct FmmConfig {
    float       theta{0.5f};                    // Accuracy knob (0.5 is standard)
    int         maxLeaf{64};                    // Max particles per octree leaf
    int         maxDepth{16};                   // MaxDepth to the tree
    float       gravity{1.0f};                  // Interaction strength
    int         dim_mapping[3] = {0, 1, 2};     // Which embedding dims map to X,Y,Z
};

}
