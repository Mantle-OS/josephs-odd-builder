#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "ibyte_encoder.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT Gpt2ByteEncoder final : public IByteEncoder
{
public:
    using Ptr  = std::shared_ptr<Gpt2ByteEncoder>;
    using WPtr = std::weak_ptr<Gpt2ByteEncoder>;
    using UPtr = std::unique_ptr<Gpt2ByteEncoder>;

    Gpt2ByteEncoder() = default;
    ~Gpt2ByteEncoder() override = default;

    Gpt2ByteEncoder(const Gpt2ByteEncoder &) = default;
    Gpt2ByteEncoder &operator=(const Gpt2ByteEncoder &) = default;
    Gpt2ByteEncoder(Gpt2ByteEncoder &&) = default;
    Gpt2ByteEncoder &operator=(Gpt2ByteEncoder &&) = default;

    [[nodiscard]] static Ptr createShared();
    [[nodiscard]] static UPtr createUniq();

    [[nodiscard]] ByteSymbols encode(std::string_view input) const override final;
    [[nodiscard]] std::string decode(std::span<const std::string> input) const final;

};

} // namespace job::token