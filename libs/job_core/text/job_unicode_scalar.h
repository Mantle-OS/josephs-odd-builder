#pragma once

#include <cstdint>
#include <optional>
#include <type_traits>
namespace job::core::text {
class UnicodeScalar
{
public:
    struct UncheckedTag { explicit constexpr UncheckedTag() = default; };
    static inline constexpr UncheckedTag unchecked{};

    constexpr UnicodeScalar() noexcept = default;
    constexpr UnicodeScalar(char32_t value, UncheckedTag) noexcept :
        m_value(value)
    {

    }
    constexpr ~UnicodeScalar() noexcept = default;

    constexpr UnicodeScalar(const UnicodeScalar &) noexcept = default;
    constexpr UnicodeScalar &operator=(const UnicodeScalar &) noexcept = default;
    constexpr UnicodeScalar(UnicodeScalar &&) noexcept = default;
    constexpr UnicodeScalar &operator=(UnicodeScalar &&) noexcept = default;
    [[nodiscard]] constexpr bool operator==(const UnicodeScalar &) const noexcept = default;

    [[nodiscard]] static constexpr bool isValid(char32_t value) noexcept
    {
        return value <= 0x10FFFF && !(value >= 0xD800 && value <= 0xDFFF);
    }

    [[nodiscard]] static constexpr std::optional<UnicodeScalar> from(char32_t value) noexcept
    {
        if (!isValid(value))
            return std::nullopt;

        return UnicodeScalar(value);
    }

    [[nodiscard]] constexpr char32_t value() const noexcept
    {
        return m_value;
    }

private:
    constexpr explicit UnicodeScalar(char32_t value) noexcept :
        m_value(value)
    {
    }

    char32_t m_value{};
};

static_assert(sizeof(UnicodeScalar) == sizeof(char32_t),    "UnicodeScalar must have the same size as char32_t");
static_assert(alignof(UnicodeScalar) == alignof(char32_t),  "UnicodeScalar must match char32_t alignment");
static_assert(std::is_trivially_copyable_v<UnicodeScalar>,  "UnicodeScalar must be trivially copyable");
static_assert(std::is_standard_layout_v<UnicodeScalar>,     "UnicodeScalar must be standard layout");

} // namespace job::core::text