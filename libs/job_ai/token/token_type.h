#pragma once
#include <cstdint>
namespace job::ai::token{
enum class TokenType : uint8_t {
    Byte            = 0,    // The lowest place
    Ascii           = 1,    // Very very simple ascii 96 char
    Char            = 2,    // Gen 0-50: The "Crystal Lattice" (Raw Physics)
    Motif           = 3,    // Gen 50+: The "Molecule" (Merged Clusters)
    BPE             = 4,    // If we ever want to compare against standard
    Unigram         = 5,
    None            = 254   // What are you even doing
};



enum class StopMode : uint8_t {
    MaxSteps        = 0,    // Stop only when maxSteps is reached (ignore targetFitness)
    TargetFitness   = 1,    // Stop only when targetFitness is met (ignore maxSteps)
    Either          = 2     // Stop when either maxSteps or targetFitness triggers first
};

}
