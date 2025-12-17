#pragma once

#include <cstdint>
#include <memory>
#include <span>

#include "byte_lattice.h"

namespace job::ai::token {

class IToken {
public:
    using Ptr  = std::shared_ptr<IToken>;
    using UPtr = std::unique_ptr<IToken>;

    virtual ~IToken() = default;

    // Bytes -> atoms. Caller owns memory.
    virtual std::size_t encode(std::span<const uint8_t> input,
                        std::span<ByteLattice> output,
                        float mass = 1.0f) = 0;

    // Atoms -> bytes. Caller owns memory.
    virtual std::size_t decode(std::span<const ByteLattice> input,
                        std::span<uint8_t> output) = 0;

    // Evolution knob.
    virtual void mutate(uint64_t seed) = 0;
};

} // namespace job::ai::token
