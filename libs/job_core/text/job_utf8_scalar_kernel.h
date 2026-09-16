#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "job_lattice_transform_result.h"

#include "job_unicode_scalar.h"
#include "job_utf8_status.h"

namespace job::core::text {

class Utf8ScalarKernel
{
public:
    using Result = lattice::LatticeTransformResult<Utf8Status>;

    Utf8ScalarKernel() = delete;
    ~Utf8ScalarKernel() = delete;

    Utf8ScalarKernel(const Utf8ScalarKernel &) = delete;
    Utf8ScalarKernel &operator=(const Utf8ScalarKernel &) = delete;
    Utf8ScalarKernel(Utf8ScalarKernel &&) = delete;
    Utf8ScalarKernel &operator=(Utf8ScalarKernel &&) = delete;

    [[nodiscard]] static constexpr Result decode(std::span<const std::uint8_t> input, UnicodeScalar &output) noexcept
    {
        if (input.empty())
            return {.status = Utf8Status::Incomplete};

        const std::uint8_t b0 = input[0];
        if (b0 <= 0x7F) {
            output = *UnicodeScalar::from(static_cast<char32_t>(b0));
            return {
                .inputConsumed = 1,
                .outputProduced = 1,
                .status = Utf8Status::Complete
            };
        }

        std::size_t length{};
        char32_t value{};

        if (b0 >= 0xC2 && b0 <= 0xDF) {
            length = 2;
            value = static_cast<char32_t>(b0 & 0x1F);
        } else if (b0 >= 0xE0 && b0 <= 0xEF) {
            length = 3;
            value = static_cast<char32_t>(b0 & 0x0F);
        } else if (b0 >= 0xF0 && b0 <= 0xF4) {
            length = 4;
            value = static_cast<char32_t>(b0 & 0x07);
        } else {
            return {.status = Utf8Status::Invalid};
        }

        if (input.size() < length)
            return {.status = Utf8Status::Incomplete};

        for (std::size_t i = 1; i < length; ++i) {
            const std::uint8_t byte = input[i];
            if (!isContinuation(byte))
                return {.status = Utf8Status::Invalid};

            value = static_cast<char32_t>((value << 6) | (byte & 0x3F));
        }

        if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) || (length == 4 && value < 0x10000))
            return {.status = Utf8Status::Invalid};

        const auto scalar = UnicodeScalar::from(value);
        if (!scalar)
            return {.status = Utf8Status::Invalid};

        output = *scalar;

        return {
            .inputConsumed = length,
            .outputProduced = 1,
            .status = Utf8Status::Complete
        };
    }

    [[nodiscard]] static constexpr Result encode(UnicodeScalar input, std::span<std::uint8_t> output) noexcept
    {
        const char32_t value = input.value();
        std::size_t required{};
        if (value <= 0x7F)
            required = 1;
        else if (value <= 0x7FF)
            required = 2;
        else if (value <= 0xFFFF)
            required = 3;
        else
            required = 4;

        if (output.size() < required)
            return {.status = Utf8Status::OutputFull};

        switch (required) {
        case 1:
            output[0] = static_cast<std::uint8_t>(value);
            break;

        case 2:
            output[0] = static_cast<std::uint8_t>(0xC0 | (value >> 6));
            output[1] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
            break;

        case 3:
            output[0] = static_cast<std::uint8_t>(0xE0 | (value >> 12));
            output[1] = static_cast<std::uint8_t>(0x80 | ((value >> 6) & 0x3F));
            output[2] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
            break;

        case 4:
            output[0] = static_cast<std::uint8_t>(0xF0 | (value >> 18));
            output[1] = static_cast<std::uint8_t>(0x80 | ((value >> 12) & 0x3F));
            output[2] = static_cast<std::uint8_t>(0x80 | ((value >> 6) & 0x3F));
            output[3] = static_cast<std::uint8_t>(0x80 | (value & 0x3F));
            break;
        }

        return {
            .inputConsumed = 1,
            .outputProduced = required,
            .status = Utf8Status::Complete
        };
    }

    [[nodiscard]] static constexpr Result validate(std::span<const std::uint8_t> input) noexcept
    {
        UnicodeScalar scalar{};
        return decode(input, scalar);
    }

private:
    [[nodiscard]] static constexpr bool isContinuation(std::uint8_t value) noexcept
    {
        return (value & 0xC0) == 0x80;
    }
};

} // namespace job::core::text