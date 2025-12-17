#pragma once

#include <cassert>
#include <vector>
#include <span>

#include "itoken.h"

namespace job::ai::token {

inline void safeEncode(IToken &tok,
                       std::span<const uint8_t> input,
                       std::vector<ByteLattice> &output,
                       float mass = 1.0f)
{
    output.resize(input.size());
    assert(output.size() == input.size());

    tok.encode(input, std::span<ByteLattice>(output), mass);
}

inline void safeDecode(IToken &tok,
                       std::span<const ByteLattice> input,
                       std::vector<uint8_t> &output)
{
    output.resize(input.size());
    assert(output.size() == input.size());

    tok.decode(input, std::span<uint8_t>(output));
}

} // namespace job::ai::token
