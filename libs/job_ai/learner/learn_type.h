#pragma once
#include <cstdint>
namespace job::ai::learn {

enum class LearnType : uint8_t {
    None        = 0,
    XOR         = 1,
    CartPole    = 2,
    Portal      = 3,
    Bard        = 4
};

}
