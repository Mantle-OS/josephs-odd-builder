#pragma once

#include "itoken.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace job::ai::token {

class AsciiToken final : public IToken {
public:
    // Printable ASCII is 32 (' ') to 126 ('~').
    // That gives us 95 characters.
    static constexpr int kVocabSize   = 95;
    static constexpr int kAsciiOffset = 32;

    [[nodiscard]] std::vector<UnicodeLattice> encode(std::string_view text, float mass = 1.0f) override
    {
        std::vector<UnicodeLattice> ret;
        ret.reserve(text.size());

        for (char c : text) {
            int val = (unsigned char)c;

            if (val < kAsciiOffset)
                val = kAsciiOffset;

            if (val > 126)
                val = 126;

            float normalized = ((float)(val - kAsciiOffset) / (float)(kVocabSize - 1)) * 2.0f - 1.0f;

            ret.push_back({ normalized, 0.0f, 0.0f, mass });
        }
        return ret;
    }

    [[nodiscard]] std::string decode(const std::vector<UnicodeLattice> &tokens) override
    {
        std::string ret;
        ret.reserve(tokens.size());

        for (const auto &p : tokens) {
            float t = (p.x + 1.0f) * 0.5f;
            int idx = (int)std::round(t * (float)(kVocabSize - 1));

            if (idx < 0)
                idx = 0;

            if (idx >= kVocabSize)
                idx = kVocabSize - 1;

            char c = (char)(idx + kAsciiOffset);
            ret.push_back(c);
        }
        return ret;
    }

    void mutate(uint64_t /*seed*/) override {}
};

} // namespace job::ai::token
