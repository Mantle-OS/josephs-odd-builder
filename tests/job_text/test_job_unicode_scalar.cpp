#include <catch2/catch_test_macros.hpp>

#include <job_unicode_scalar.h>

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("UnicodeScalar creates valid Unicode scalar values", "[core][text][unicode][usage]")
{
    const auto scalar = UnicodeScalar::from(U'A');

    REQUIRE(scalar.has_value());
    CHECK(scalar->value() == U'A');
}

TEST_CASE("UnicodeScalar represents supplementary Unicode scalar values", "[core][text][unicode][usage]")
{
    constexpr char32_t grinningFace = 0x1F600;

    const auto scalar = UnicodeScalar::from(grinningFace);

    REQUIRE(scalar.has_value());
    CHECK(scalar->value() == grinningFace);
}

TEST_CASE("UnicodeScalar default value represents U+0000", "[core][text][unicode][usage]")
{
    constexpr UnicodeScalar scalar{};

    CHECK(scalar.value() == U'\0');
    STATIC_CHECK(UnicodeScalar::isValid(scalar.value()));
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("UnicodeScalar accepts Unicode scalar boundary values", "[core][text][unicode][edge]")
{
    STATIC_CHECK(UnicodeScalar::isValid(0x000000));
    STATIC_CHECK(UnicodeScalar::isValid(0x00D7FF));
    STATIC_CHECK(UnicodeScalar::isValid(0x00E000));
    STATIC_CHECK(UnicodeScalar::isValid(0x10FFFF));

    const auto first = UnicodeScalar::from(0x000000);
    const auto beforeSurrogates = UnicodeScalar::from(0x00D7FF);
    const auto afterSurrogates = UnicodeScalar::from(0x00E000);
    const auto last = UnicodeScalar::from(0x10FFFF);

    REQUIRE(first.has_value());
    REQUIRE(beforeSurrogates.has_value());
    REQUIRE(afterSurrogates.has_value());
    REQUIRE(last.has_value());

    CHECK(first->value() == 0x000000);
    CHECK(beforeSurrogates->value() == 0x00D7FF);
    CHECK(afterSurrogates->value() == 0x00E000);
    CHECK(last->value() == 0x10FFFF);
}

TEST_CASE("UnicodeScalar rejects UTF-16 surrogate code points", "[core][text][unicode][edge]")
{
    STATIC_CHECK_FALSE(UnicodeScalar::isValid(0x00D800));
    STATIC_CHECK_FALSE(UnicodeScalar::isValid(0x00DBFF));
    STATIC_CHECK_FALSE(UnicodeScalar::isValid(0x00DC00));
    STATIC_CHECK_FALSE(UnicodeScalar::isValid(0x00DFFF));

    CHECK_FALSE(UnicodeScalar::from(0x00D800).has_value());
    CHECK_FALSE(UnicodeScalar::from(0x00DBFF).has_value());
    CHECK_FALSE(UnicodeScalar::from(0x00DC00).has_value());
    CHECK_FALSE(UnicodeScalar::from(0x00DFFF).has_value());
}

TEST_CASE("UnicodeScalar rejects values above the Unicode scalar range", "[core][text][unicode][edge]")
{
    constexpr char32_t firstInvalid = 0x110000;
    constexpr char32_t maximumChar32 = 0xFFFFFFFF;

    STATIC_CHECK_FALSE(UnicodeScalar::isValid(firstInvalid));
    STATIC_CHECK_FALSE(UnicodeScalar::isValid(maximumChar32));

    CHECK_FALSE(UnicodeScalar::from(firstInvalid).has_value());
    CHECK_FALSE(UnicodeScalar::from(maximumChar32).has_value());
}

TEST_CASE("UnicodeScalar validity is constexpr", "[core][text][unicode][constexpr]")
{
    constexpr auto ascii = UnicodeScalar::from(U'J');
    constexpr auto supplementary = UnicodeScalar::from(0x1F680);
    constexpr auto surrogate = UnicodeScalar::from(0xD800);
    constexpr auto tooLarge = UnicodeScalar::from(0x110000);

    STATIC_REQUIRE(ascii.has_value());
    STATIC_REQUIRE(supplementary.has_value());
    STATIC_REQUIRE_FALSE(surrogate.has_value());
    STATIC_REQUIRE_FALSE(tooLarge.has_value());

    STATIC_CHECK(ascii->value() == U'J');
    STATIC_CHECK(supplementary->value() == 0x1F680);
}

} // namespace job::core::text::tests