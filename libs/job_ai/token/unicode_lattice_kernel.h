#pragma once

#include "unicode_lattice.h"

#include <span>
#include <vector>
#include <cassert>

namespace job::ai::token {

struct UnicodeLatticeKernel {
    // Fast path: caller owns memory.
    static void batchEncode(std::span<const char32_t> input,
                            std::span<UnicodeLattice> output)
    {
        assert(output.size() == input.size());

        for (size_t i = 0; i < input.size(); ++i)
            output[i] = UnicodeLattice::encode(input[i]);
    }

    // Safety dance: resizes output to match input (allocations are explicit by name).
    static void safeBatchEncode(std::span<const char32_t> input,
                                std::vector<UnicodeLattice> &output)
    {
        output.resize(input.size());
        batchEncode(input, std::span<UnicodeLattice>(output));
    }

    // Fast path: caller owns memory.
    static void batchDecode(std::span<const UnicodeLattice> input,
                            std::span<char32_t> output)
    {
        assert(output.size() == input.size());

        for (size_t i = 0; i < input.size(); ++i)
            output[i] = UnicodeLattice::decode(input[i]);
    }

    // Safety dance: resizes output to match input (allocations are explicit by name).
    static void safeBatchDecode(std::span<const UnicodeLattice> input,
                                std::vector<char32_t> &output)
    {
        output.resize(input.size());
        batchDecode(input, std::span<char32_t>(output));
    }
};

} // namespace job::ai::token
