#pragma once

#include <cstdint>
#include <span>

#include "itoken.h"
#include "byte_lattice_kernel.h"

namespace job::ai::token {

class ByteToken final : public IToken {
public:
    std::size_t encode(std::span<const uint8_t> input,
                std::span<ByteLattice> output,
                float mass = 1.0f) override
    {
        const std::size_t n = (input.size() < output.size()) ? input.size() : output.size();

        ByteLatticeKernel::batchEncode(input.first(n),
                                       output.first(n),
                                       mass);
        return n;
    }

    std::size_t decode(std::span<const ByteLattice> input,
                std::span<uint8_t> output) override
    {
        const std::size_t n = (input.size() < output.size()) ? input.size() : output.size();

        ByteLatticeKernel::batchDecode(input.first(n),
                                       output.first(n));
        return n;
    }

    void mutate(uint64_t) override
    {
        // Bytes don't mutate. They just exist.
    }
};

} // namespace job::ai::token
