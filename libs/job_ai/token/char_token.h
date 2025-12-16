#pragma once

#include <utils/utf8_decoder.h>

#include "itoken.h"
#include "lattice_kernel.h"


namespace job::ai::token {

class CharToken final : public IToken {
public:
    [[nodiscard]] std::vector<UnicodeLattice> encode(std::string_view text, [[maybe_unused]]float mass = 1.0f) override
    {
        std::vector<char32_t> atoms = ansi::utils::Utf8Decoder::decodeUtf8Stream(text);
        std::vector<UnicodeLattice> ret;
        LatticeKernel::batchEncode(atoms, ret);
        return ret;
    }

    [[nodiscard]] std::string decode(const std::vector<UnicodeLattice> &tokens) override
    {
        std::string ret;
        std::vector<char32_t> atoms;
        LatticeKernel::batchDecode(tokens, atoms);
        ret.reserve(atoms.size()); // Approx
        for(char32_t c : atoms)
            ansi::utils::Utf8Decoder::encodeTo(c, ret);
        return ret;
    }

    void mutate(uint64_t /*seed*/) override {}

};

} // namespace
