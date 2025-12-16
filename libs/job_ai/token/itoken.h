#pragma once

#include <memory>
#include <vector>
#include <string>
#include <string_view>

#include "unicode_lattice.h"

namespace job::ai::token {

class IToken {
public:
    using Ptr  = std::shared_ptr<IToken>;
    using UPtr = std::unique_ptr<IToken>;

    virtual ~IToken() = default;
    virtual std::vector<UnicodeLattice> encode(std::string_view text, float mass = 1.0f) = 0;
    [[nodiscard]] virtual std::string decode(const std::vector<UnicodeLattice> &tokens) = 0;
    virtual void mutate(uint64_t seed) = 0;
};

} // namespace
