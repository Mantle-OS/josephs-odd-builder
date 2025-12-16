#pragma once
#include <cstdint>
namespace job::ai::token{
enum class TokenType : uint8_t {
    Ascii   = 0,    // Very very simple ascii 96 char
    Char    = 1,    // Gen 0-50: The "Crystal Lattice" (Raw Physics)
    Motif   = 2,    // Gen 50+: The "Molecule" (Merged Clusters)
    BPE     = 3,    // If we ever want to compare against standard
    None    = 254   // What are you even doing
};
}
