#pragma once
#include <memory>

#include <utils/utf8_decoder.h>

#include "itoken.h"
namespace job::ai::token {

class BPEToken final : public IToken {
public:
    [[nodiscard]] std::vector<UnicodeLattice> encode(std::string_view text) override
    {

    }

    [[nodiscard]] std::string decode(const std::vector<UnicodeLattice> &tokens) override
    {
    }

    void mutate(uint64_t /*seed*/) override {}

};

} // namespace
