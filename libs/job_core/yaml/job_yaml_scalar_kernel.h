#pragma once

#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace job::yaml {

enum class YamlScalarKind : std::uint8_t {
    String,
    Null,
    Boolean,
    Integer,
    FloatingPoint
};

class YamlScalarKernel
{
public:
    YamlScalarKernel() = delete;
    ~YamlScalarKernel() = delete;

    YamlScalarKernel(const YamlScalarKernel &) = delete;
    YamlScalarKernel &operator=(const YamlScalarKernel &) = delete;
    YamlScalarKernel(YamlScalarKernel &&) = delete;
    YamlScalarKernel &operator=(YamlScalarKernel &&) = delete;

    [[nodiscard]] static constexpr YamlScalarKind classify(std::string_view value) noexcept
    {
        if (isNull(value))
            return YamlScalarKind::Null;

        if (isBoolean(value))
            return YamlScalarKind::Boolean;

        if (isInteger(value))
            return YamlScalarKind::Integer;

        if (isFloatingPoint(value))
            return YamlScalarKind::FloatingPoint;

        return YamlScalarKind::String;
    }

    [[nodiscard]] static constexpr bool isNull(std::string_view value) noexcept
    {
        return value == "~" || value == "null" || value == "Null" || value == "NULL";
    }

    [[nodiscard]] static constexpr bool isBoolean(std::string_view value) noexcept
    {
        return value == "true"  || value == "True"  || value == "TRUE" ||
               value == "false" || value == "False" || value == "FALSE";
    }

    [[nodiscard]] static constexpr bool parseBool(std::string_view value, bool &result) noexcept
    {
        if (value == "true" || value == "True" || value == "TRUE") {
            result = true;
            return true;
        }

        if (value == "false" || value == "False" || value == "FALSE") {
            result = false;
            return true;
        }

        return false;
    }

    template <std::integral T>
    [[nodiscard]] static constexpr bool parseInteger(std::string_view value, T &result) noexcept
    {
        if (value.empty())
            return false;

        bool negative = false;
        std::size_t offset = 0;

        if (value[0] == '+' || value[0] == '-') {
            negative = value[0] == '-';
            offset = 1;

            if (offset == value.size())
                return false;
        }

        int base = 10;

        if (value.size() - offset > 2 && value[offset] == '0') {
            if (value[offset + 1] == 'x' || value[offset + 1] == 'X') {
                base = 16;
                offset += 2;
            } else if (value[offset + 1] == 'o' || value[offset + 1] == 'O') {
                base = 8;
                offset += 2;
            }
        }

        if (offset == value.size())
            return false;

        using Unsigned = std::make_unsigned_t<T>;

        Unsigned magnitude{};
        const char *first = value.data() + offset;
        const char *last = value.data() + value.size();

        const auto [ptr, ec] = std::from_chars(first, last, magnitude, base);

        if (ec != std::errc{} || ptr != last)
            return false;

        if constexpr (std::is_signed_v<T>) {
            using Limits = std::numeric_limits<T>;

            if (negative) {
                const Unsigned limit = static_cast<Unsigned>(Limits::max()) + 1U;

                if (magnitude > limit)
                    return false;

                if (magnitude == limit) {
                    result = Limits::min();
                    return true;
                }

                result = static_cast<T>(-static_cast<T>(magnitude));
                return true;
            }

            if (magnitude > static_cast<Unsigned>(Limits::max()))
                return false;

            result = static_cast<T>(magnitude);
            return true;
        } else {
            if (negative)
                return false;

            result = static_cast<T>(magnitude);
            return true;
        }
    }

    template <std::floating_point T>
    [[nodiscard]] static bool parseFloat(std::string_view value, T &result) noexcept
    {
        if (value.empty())
            return false;

        if (isPositiveInfinity(value)) {
            result = std::numeric_limits<T>::infinity();
            return true;
        }

        if (isNegativeInfinity(value)) {
            result = -std::numeric_limits<T>::infinity();
            return true;
        }

        if (isNaN(value)) {
            result = std::numeric_limits<T>::quiet_NaN();
            return true;
        }

        if (!isDecimalNumberSyntax(value))
            return false;

        const char *first = value.data();
        const char *last = first + value.size();

        if (*first == '+')
            ++first;

        T parsed{};
        const auto [ptr, ec] = std::from_chars(first, last, parsed, std::chars_format::general);

        if (ec != std::errc{} || ptr != last)
            return false;

        result = parsed;
        return true;
    }

    template <typename T>
        requires std::is_enum_v<T>
    [[nodiscard]] static constexpr bool parseEnum(std::string_view value, T &result) noexcept
    {
        using Underlying = std::underlying_type_t<T>;

        Underlying parsed{};

        if (!parseInteger(value, parsed))
            return false;

        result = static_cast<T>(parsed);
        return true;
    }

    template <typename T>
    [[nodiscard]] static constexpr bool parse(std::string_view value, T &result) noexcept
    {
        using Value = std::remove_cvref_t<T>;

        if constexpr (std::same_as<Value, bool>) {
            return parseBool(value, result);
        } else if constexpr (std::integral<Value>) {
            return parseInteger(value, result);
        } else if constexpr (std::floating_point<Value>) {
            return parseFloat(value, result);
        } else if constexpr (std::is_enum_v<Value>) {
            return parseEnum(value, result);
        } else {
            return false;
        }
    }

    [[nodiscard]] static constexpr bool isInteger(std::string_view value) noexcept
    {
        std::int64_t parsed{};
        return parseInteger(value, parsed);
    }

    [[nodiscard]] static bool isFloatingPoint(std::string_view value) noexcept
    {
        if (!looksFloatingPoint(value))
            return false;

        double parsed{};
        return parseFloat(value, parsed);
    }

private:
    [[nodiscard]] static constexpr bool isDecimalNumberSyntax(std::string_view value) noexcept
    {
        if (value.empty())
            return false;

        std::size_t i = 0;

        if (value[i] == '+' || value[i] == '-') {
            ++i;

            if (i == value.size())
                return false;
        }

        bool digitsBeforeDot = false;
        bool digitsAfterDot = false;

        while (i < value.size() && value[i] >= '0' && value[i] <= '9') {
            digitsBeforeDot = true;
            ++i;
        }

        if (i < value.size() && value[i] == '.') {
            ++i;

            while (i < value.size() && value[i] >= '0' && value[i] <= '9') {
                digitsAfterDot = true;
                ++i;
            }
        }

        if (!digitsBeforeDot && !digitsAfterDot)
            return false;

        if (i < value.size() && (value[i] == 'e' || value[i] == 'E')) {
            ++i;

            if (i < value.size() && (value[i] == '+' || value[i] == '-'))
                ++i;

            const std::size_t exponentStart = i;

            while (i < value.size() && value[i] >= '0' && value[i] <= '9')
                ++i;

            if (i == exponentStart)
                return false;
        }

        return i == value.size();
    }

    [[nodiscard]] static constexpr bool isPositiveInfinity(std::string_view value) noexcept
    {
        return value == ".inf" ||
               value == ".Inf" ||
               value == ".INF" ||
               value == "+.inf" ||
               value == "+.Inf" ||
               value == "+.INF";
    }

    [[nodiscard]] static constexpr bool isNegativeInfinity(std::string_view value) noexcept
    {
        return value == "-.inf" || value == "-.Inf" || value == "-.INF";
    }

    [[nodiscard]] static constexpr bool isNaN(std::string_view value) noexcept
    {
        return value == ".nan" || value == ".NaN" || value == ".NAN";
    }

    [[nodiscard]] static constexpr bool looksFloatingPoint(std::string_view value) noexcept
    {
        if (isPositiveInfinity(value) || isNegativeInfinity(value) || isNaN(value))
            return true;

        for (const char c : value) {
            if (c == '.' || c == 'e' || c == 'E')
                return true;
        }

        return false;
    }
};

} // namespace job::yaml