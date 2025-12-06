#pragma once
#include <cstdint>
namespace job::ai::evo {
enum class CrossoverType : uint8_t {
    None = 0,
    OnePoint,
    TwoPoint,
    Uniform,
    Arithmetic,
    Neat
};
}
