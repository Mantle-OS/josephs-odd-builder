#pragma once

#include <cstdint>
#include <span>

#include "itoken.h"
#include "byte_lattice_kernel.h"

namespace job::ai::token {

class AsciiToken final : public IToken {
public:
    static constexpr uint8_t    kAsciiMin   = 32;  // ' '
    static constexpr uint8_t    kAsciiMax   = 126; // '~'
    static constexpr uint8_t    kAsciiVocab = 95;


    std::size_t encode(std::span<const uint8_t> input,
                std::span<ByteLattice> output,
                float mass = 1.0f) override
    {
        const std::size_t n = (input.size() < output.size()) ? input.size() : output.size();

        ByteLatticeKernel::batchEncode(input.first(n),
                                       output.first(n),
                                       mass);

        // ASCII policy: clamp produced atoms to printable range.
        for (size_t i = 0; i < n; ++i) {
            uint8_t b = ByteLattice::decode(output[i]);

            if (b < kAsciiMin)
                b = kAsciiMin;

            if (b > kAsciiMax)
                b = kAsciiMax;

            output[i] = ByteLattice::encode(b, output[i].mass);
        }

        return n;
    }

    std::size_t decode(std::span<const ByteLattice> input,
                std::span<uint8_t> output) override
    {
        const std::size_t n = (input.size() < output.size()) ? input.size() : output.size();

        ByteLatticeKernel::batchDecode(input.first(n),
                                       output.first(n));

        // ASCII policy: clamp produced bytes to printable range.
        for (size_t i = 0; i < n; ++i) {
            uint8_t b = output[i];

            if (b < kAsciiMin)
                b = kAsciiMin;

            if (b > kAsciiMax)
                b = kAsciiMax;

            output[i] = b;
        }

        return n;
    }

    void mutate(uint64_t) override
    {
        // ASCII doesn't mutate.
    }
};

} // namespace job::ai::token
