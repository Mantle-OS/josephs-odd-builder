#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <span>

#include "job_token_types.h"
#include "jobtoken_export.h"

namespace job::token {
class JOBTOKEN_EXPORT IByteEncoder
{
public:
    using Ptr  = std::shared_ptr<IByteEncoder>;
    using WPtr = std::weak_ptr<IByteEncoder>;
    using UPtr = std::unique_ptr<IByteEncoder>;

    IByteEncoder() = default;
    virtual ~IByteEncoder() = default;

    IByteEncoder(const IByteEncoder &) = default;
    IByteEncoder &operator=(const IByteEncoder &) = default;
    IByteEncoder(IByteEncoder &&) = default;
    IByteEncoder &operator=(IByteEncoder &&) = default;

    [[nodiscard]] virtual ByteSymbols encode(std::string_view input) const = 0;
    [[nodiscard]] virtual std::string decode(std::span<const std::string> input) const = 0;

};

} // namespace job::token