#include <concepts>

#include <catch2/catch_test_macros.hpp>

#include <job_lattice_codec.h>
#include <job_text_dispatch.h>
#include <job_text_encoding.h>
#include <job_utf8.h>
#include <job_utf16.h>
#include <job_utf32.h>

namespace job::core::text::tests {

// -----------------------------------------------------------------------------
// Block one: usage / examples
// -----------------------------------------------------------------------------

TEST_CASE("TextCodec maps UTF-8 encoding to Utf8", "[core][text][dispatch][utf8][usage]")
{
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf8>, Utf8>);
}

TEST_CASE("TextCodec maps UTF-16 encoding to Utf16", "[core][text][dispatch][utf16][usage]")
{
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf16>, Utf16>);
}

TEST_CASE("TextCodec maps UTF-32 encoding to Utf32", "[core][text][dispatch][utf32][usage]")
{
    STATIC_CHECK(std::same_as<TextCodec<TextEncoding::Utf32>, Utf32>);
}

TEST_CASE("TextCodec dispatch results satisfy LatticeCodec", "[core][text][dispatch][lattice][usage]")
{
    STATIC_CHECK(lattice::LatticeCodec<TextCodec<TextEncoding::Utf8>>);
    STATIC_CHECK(lattice::LatticeCodec<TextCodec<TextEncoding::Utf16>>);
    STATIC_CHECK(lattice::LatticeCodec<TextCodec<TextEncoding::Utf32>>);
}

// -----------------------------------------------------------------------------
// Block two: edge cases
// -----------------------------------------------------------------------------

TEST_CASE("TextCodec dispatch preserves codec unit types", "[core][text][dispatch][types][edge]")
{
    using Utf8Codec  = TextCodec<TextEncoding::Utf8>;
    using Utf16Codec = TextCodec<TextEncoding::Utf16>;
    using Utf32Codec = TextCodec<TextEncoding::Utf32>;

    STATIC_CHECK(std::same_as<typename Utf8Codec::UnitInput, lattice::LatticeKernelInput<std::uint8_t>>);
    STATIC_CHECK(std::same_as<typename Utf8Codec::UnitOutput, lattice::LatticeKernelOutput<std::uint8_t>>);

    STATIC_CHECK(std::same_as<typename Utf16Codec::UnitInput, lattice::LatticeKernelInput<char16_t>>);
    STATIC_CHECK(std::same_as<typename Utf16Codec::UnitOutput, lattice::LatticeKernelOutput<char16_t>>);

    STATIC_CHECK(std::same_as<typename Utf32Codec::UnitInput, lattice::LatticeKernelInput<char32_t>>);
    STATIC_CHECK(std::same_as<typename Utf32Codec::UnitOutput, lattice::LatticeKernelOutput<char32_t>>);
}

} // namespace job::core::text::tests