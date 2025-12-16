#pragma once

#include "unicode_lattice.h"

#include <vector>
#include <span>

namespace job::ai::token {
struct LatticeKernel {
    static void batchEncode(std::span<const char32_t> input, std::vector<UnicodeLattice> &output)
    {
        output.resize(input.size());
        size_t i = 0;
        size_t n = input.size();
        for (; i < n; ++i)
            output[i] = UnicodeLattice::encode(input[i]);
    }

    static void batchDecode(std::span<const UnicodeLattice> input, std::vector<char32_t> &output)
    {
        output.resize(input.size());
        for (size_t i = 0; i < input.size(); ++i)
            output[i] = UnicodeLattice::decode(input[i]);
    }
};

}
