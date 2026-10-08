#pragma once

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <cstddef>

#include "job_lattice.h"
#include "job_text.h"

namespace job::core {

template <typename T>
concept JobStringType = std::same_as<std::remove_cvref_t<T>, std::string> || std::same_as<std::remove_cvref_t<T>, std::string_view>;

template <typename T>
concept JobIntegerType = std::integral<std::remove_cvref_t<T>> && !std::same_as<std::remove_cvref_t<T>, bool>;


enum class JobStringCleanMode : std::uint8_t {
    Space,
    Esc,
    SpaceAndEsc,
    Tabs,
    All
};

class JobStringUtils
{
public:
    using UnicodeScalars = std::vector<text::UnicodeScalar>;
    static inline constexpr char kHexChars[] = "0123456789ABCDEF";

    JobStringUtils() = delete;
    ~JobStringUtils() = delete;

    JobStringUtils(const JobStringUtils &) = delete;
    JobStringUtils &operator=(const JobStringUtils &) = delete;

    JobStringUtils(JobStringUtils &&) noexcept = delete;
    JobStringUtils &operator=(JobStringUtils &&) noexcept = delete;

    template <JobStringType S>
    [[nodiscard]] static std::uint64_t toUInt64(const S &val, int base = 10) noexcept
    {
        std::uint64_t outVal = 0;
        const auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), outVal, base);
        if (ec != std::errc{} || ptr != val.data() + val.size())
            return 0;

        return outVal;
    }

    template <JobIntegerType I, JobStringType S>
    [[nodiscard]] static I toInteger(const S &val, int base = 10) noexcept
    {
        I outVal = 0;
        const auto [ptr, ec] = std::from_chars(val.data(), val.data() + val.size(), outVal, base);
        if (ec != std::errc{} || ptr != val.data() + val.size())
            return 0;

        return outVal;
    }


    [[nodiscard]] static std::string toLower(std::string_view sv)
    {
        std::string result(sv);
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return result;
    }

    [[nodiscard]] static std::string toUpper(std::string_view sv)
    {
        std::string result(sv);
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

        return result;
    }

    template <JobStringType S>
    [[nodiscard]] static constexpr const char *data(const S &val) noexcept
    {
        return val.data();
    }

    template <JobStringType S>
    [[nodiscard]] static constexpr std::size_t size(const S &val) noexcept
    {
        return val.size();
    }


    template <JobStringType S>
    [[nodiscard]] static std::string toHex(const S &val, char sep = '\0')
    {
        std::string out;

        if (sep == '\0') {
            out.reserve(2 + val.size() * 2);
            out += "0x";
        } else if (!val.empty()) {
            out.reserve(val.size() * 3 - 1);
        }

        for (std::size_t i = 0; i < val.size(); ++i) {
            const auto byte = static_cast<unsigned char>(val[i]);
            out += kHexChars[(byte >> 4) & 0x0F];
            out += kHexChars[byte & 0x0F];
            if (sep != '\0' && i + 1 < val.size())
                out += sep;
        }

        return out;
    }

    template <JobStringType S>
    [[nodiscard]] static constexpr std::string fromHex(const S &val, char sep = '\0')
    {
        std::string_view hex(val);

        if (sep == '\0' && hex.starts_with("0x"))
            hex.remove_prefix(2);

        std::string out;
        out.reserve(hex.size() / 2);

        std::size_t pos = 0;

        while (pos < hex.size()) {
            if (pos + 2 > hex.size())
                return {};

            std::uint8_t byte = 0;
            const auto [ptr, ec] = std::from_chars(hex.data() + pos, hex.data() + pos + 2, byte, 16);
            if (ec != std::errc{} || ptr != hex.data() + pos + 2)
                return {};

            out.push_back(static_cast<char>(byte));
            pos += 2;

            if (sep != '\0' && pos < hex.size()) {
                if (hex[pos] != sep)
                    return {};

                ++pos;
                if (pos >= hex.size())
                    return {};
            }
        }

        return out;
    }


    // replace("this is", "this", "that") -> "that is"
    template <JobStringType S>
    [[nodiscard]] static constexpr std::string replace(const S &val, const S &before, const S &after)
    {
        std::string result(val);
        const auto pos = result.find(before);
        if (pos != std::string::npos)
            result.replace(pos, before.size(), after);

        return result;
    }


    // replace("this is", "is cool", 2) -> "this is cool"
    template <JobStringType S>
    [[nodiscard]] static constexpr std::string replace(const S &val, const S &after, std::size_t pos)
    {
        std::string result(val);
        if (pos > result.size())
            return result;

        result.replace(result.size() - pos, pos, after);
        return result;
    }


    // replace("this is", 's', "s cool", 2) -> "this is cool"
    template <JobStringType S>
    [[nodiscard]] static constexpr std::string replace(const S &val, char needle, const S &after, std::size_t pos = 0)
        pre(pos > 0)
    {
        std::string result(val);
        std::size_t found = 0;
        std::size_t count = 0;
        while (count < pos) {
            found = result.find(needle, found);
            if (found == std::string::npos)
                return result;

            ++count;

            if (count < pos)
                ++found;
        }

        result.replace(found, 1, after);
        return result;
    }

    template <JobStringType S, JobIntegerType I>
        requires (!std::same_as<std::remove_cvref_t<I>, char>)
    [[nodiscard]] static constexpr std::string split(const S &val, I pos)
        pre(pos > 0)
    {
        const auto offset = static_cast<std::size_t>(pos);
        if (offset >= val.size())
            return {};

        return std::string(val.data() + offset, val.size() - offset);
    }


    // "this is" -> sep = ' ' -> {"this", "is"}
    template <JobStringType S>
    [[nodiscard]] static constexpr std::vector<std::string> split(const S &val, char sep = ' ')
        pre(sep != '\0')
    {
        std::vector<std::string> result;
        std::size_t begin = 0;

        while (begin <= val.size()) {
            const auto end = val.find(sep, begin);

            if (end == S::npos) {
                result.emplace_back(val.data() + begin, val.size() - begin);
                break;
            }

            result.emplace_back(val.data() + begin, end - begin);
            begin = end + 1;
        }

        return result;
    }

    // "this is\r           " -> "this is"
    template <JobStringType S>
    [[nodiscard]] static std::string noTail(const S &val, JobStringCleanMode cleanMode = JobStringCleanMode::SpaceAndEsc)
    {
        std::string result(val);
        while (!result.empty() && shouldClean(static_cast<unsigned char>(result.back()), cleanMode))
            result.pop_back();

        return result;
    }

    // "  this is" -> "this is"
    template <JobStringType S>
    [[nodiscard]] static std::string noHead(const S &val, JobStringCleanMode cleanMode = JobStringCleanMode::SpaceAndEsc)
    {
        std::size_t pos = 0;
        while (pos < val.size() && shouldClean(static_cast<unsigned char>(val[pos]), cleanMode))
            ++pos;
        return std::string(val.data() + pos, val.size() - pos);
    }



    [[nodiscard]] static std::string toAscii(text::Utf8::ScalarInput val)
    {
        std::string out;
        out.reserve(val.size());
        for (const auto &scalar : val) {
            const auto value = scalar.value();
            if (value > 0x7F)
                return {};

            out.push_back(static_cast<char>(value));
        }

        return out;
    }

    [[nodiscard]] static std::string toUtf8(text::Utf8::ScalarInput val)
    {
        std::vector<std::uint8_t> output(val.size() * 4);

        const auto result = text::Utf8::encode(val, output);
        if (result.status != text::Utf8Status::Complete)
            return {};

        if (result.outputProduced == 0)
            return {};

        return std::string(
            reinterpret_cast<const char *>(output.data()),
            result.outputProduced);
    }

    [[nodiscard]] static std::u16string toUtf16(text::Utf16::ScalarInput val)
    {
        std::u16string output(val.size() * 2, u'\0');
        const auto result = text::Utf16::encode(val, output);
        if (result.status != text::Utf16Status::Complete)
            return {};

        output.resize(result.outputProduced);
        return output;
    }

    [[nodiscard]] static std::u32string toUtf32(text::Utf32::ScalarInput val)
    {
        std::u32string output(val.size(), U'\0');
        const auto result = text::Utf32::encode(val, output);
        if (result.status != text::Utf32Status::Complete)
            return {};

        output.resize(result.outputProduced);
        return output;
    }

    [[nodiscard]] static UnicodeScalars fromAscii(text::Utf8::UnitInput val)
    {
        UnicodeScalars output;
        output.reserve(val.size());
        for (const auto byte : val) {
            if (byte > 0x7F)
                return {};

            output.emplace_back( static_cast<char32_t>(byte), text::UnicodeScalar::unchecked);
        }

        return output;
    }

    [[nodiscard]] static UnicodeScalars fromUtf8(text::Utf8::UnitInput val)
    {
        UnicodeScalars output(val.size());

        const auto result = text::Utf8::decode(val, output);
        if (result.status != text::Utf8Status::Complete)
            return {};

        output.resize(result.outputProduced);
        return output;
    }

    [[nodiscard]] static UnicodeScalars fromUtf16(text::Utf16::UnitInput val)
    {
        UnicodeScalars output(val.size());

        const auto result = text::Utf16::decode(val, output);
        if (result.status != text::Utf16Status::Complete)
            return {};

        output.resize(result.outputProduced);
        return output;
    }

    [[nodiscard]] static UnicodeScalars fromUtf32(text::Utf32::UnitInput val)
    {
        UnicodeScalars output(val.size());

        const auto result = text::Utf32::decode(val, output);
        if (result.status != text::Utf32Status::Complete)
            return {};

        output.resize(result.outputProduced);
        return output;
    }




private:
    [[nodiscard]] static constexpr bool shouldClean(unsigned char c, JobStringCleanMode cleanMode) noexcept
    {
        switch (cleanMode) {
        case JobStringCleanMode::Space:
            return c == ' ';

        case JobStringCleanMode::Esc:
            return std::iscntrl(c) != 0;

        case JobStringCleanMode::SpaceAndEsc:
            return c == ' ' || std::iscntrl(c) != 0;

        case JobStringCleanMode::Tabs:
            return c == '\t';

        case JobStringCleanMode::All:
            return std::isspace(c) != 0 || std::iscntrl(c) != 0;
        }

        return false;
    }
};

}