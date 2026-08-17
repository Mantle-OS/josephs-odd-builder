#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "ibyte_encoder.h"
#include "jobtoken_export.h"

namespace job::token {

class JOBTOKEN_EXPORT RawByteEncoder final : public IByteEncoder
{
public:
    using Ptr  = std::shared_ptr<RawByteEncoder>;
    using WPtr = std::weak_ptr<RawByteEncoder>;
    using UPtr = std::unique_ptr<RawByteEncoder>;

    RawByteEncoder() = default;
    ~RawByteEncoder() override = default;

    RawByteEncoder(const RawByteEncoder &) = default;
    RawByteEncoder &operator=(const RawByteEncoder &) = default;
    RawByteEncoder(RawByteEncoder &&) = default;
    RawByteEncoder &operator=(RawByteEncoder &&) = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<RawByteEncoder>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<RawByteEncoder>();
    }

    [[nodiscard]] ByteSymbols encode(std::string_view input) const override
    {
        ByteSymbols output;
        output.reserve(input.size());

        for (const char character : input) {
            output.emplace_back(
                1,
                character);
        }

        return output;
    }

    // [[nodiscard]] std::string decode(std::span<const std::string> input) const override
    // {
    //     std::string output;
    //     output.reserve(input.size());

    //     for (const std::string &symbol : input) {
    //         if (symbol.size() != 1)
    //             return {};

    //         output.push_back(
    //             symbol.front());
    //     }

    //     return output;
    // }
    [[nodiscard]] std::string decode(std::span<const std::string> input) const override
    {
        std::size_t size = 0;

        for (const std::string &symbol : input)
            size += symbol.size();

        std::string output;
        output.reserve(size);

        for (const std::string &symbol : input)
            output += symbol;

        return output;
    }
};

} // namespace job::token