#pragma once

#include <cstdint>
#include <concepts>
#include <span>

#include "byte_lattice.h"

namespace job::ai::token {

template <typename T>
concept TokenConcept = requires(T t,
                                std::span<const uint8_t>        inBytes,
                                std::span<ByteLattice>          outAtoms,
                                std::span<const ByteLattice>    inAtoms,
                                std::span<uint8_t>              outBytes,
                                float                           mass,
                                uint64_t                        seed)
{
    { t.encode(inBytes, outAtoms, mass) } -> std::same_as<std::size_t>;
    { t.decode(inAtoms, outBytes) } -> std::same_as<std::size_t>;
    { t.mutate(seed) } -> std::same_as<void>;
};

} // namespace job::ai::token
