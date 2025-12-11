#pragma once
#include <cstdint>
namespace job::ai::learner {

enum class LearnerType : uint8_t {
    None        = 0,
    XOR         = 1,
    CartPole    = 2,
    Portal      = 3
};

}
