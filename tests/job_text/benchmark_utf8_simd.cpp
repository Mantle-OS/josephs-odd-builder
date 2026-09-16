#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_text.h>

namespace job::core::text::tests {

#ifdef JOB_TEST_BENCHMARKS

struct SimdTextBenchmarkCorpus
{
    std::vector<UnicodeScalar> scalars;
    std::vector<std::uint8_t> utf8;
};

[[nodiscard]] static SimdTextBenchmarkCorpus makeSimdBenchmarkCorpus(std::span<const char32_t> pattern, std::size_t scalarCount)
{
    SimdTextBenchmarkCorpus corpus{};
    corpus.scalars.reserve(scalarCount);

    for (std::size_t i = 0; i < scalarCount; ++i) {
        const auto scalar = UnicodeScalar::from(pattern[i % pattern.size()]);
        REQUIRE(scalar.has_value());
        corpus.scalars.push_back(*scalar);
    }

    corpus.utf8.resize(scalarCount * 4);

    const auto result = Utf8::encode(corpus.scalars, corpus.utf8);
    REQUIRE(result.status == Utf8Status::Complete);

    corpus.utf8.resize(result.outputProduced);

    return corpus;
}

static void benchmarkUtf8ValidateSize(std::size_t size)
{
    const std::vector<std::uint8_t> input(
        size,
        static_cast<std::uint8_t>('A'));

    BENCHMARK("UTF-8 validate batch " + std::to_string(size) + " bytes")
    {
        return Utf8Kernel::validate(input);
    };

    BENCHMARK("UTF-8 validate ASCII SIMD " + std::to_string(size) + " bytes")
    {
        return AsciiSimdKernel::validate(input);
    };

    BENCHMARK("UTF-8 validate UTF-8 SIMD " + std::to_string(size) + " bytes")
    {
        return Utf8SimdKernel::validate(input);
    };

    BENCHMARK("UTF-8 validate policy " + std::to_string(size) + " bytes")
    {
        return Utf8::validate(input);
    };
}

static void benchmarkUtf8Validation(
    std::string_view name,
    const SimdTextBenchmarkCorpus &corpus)
{
    BENCHMARK(std::string{name} + " UTF-8 validate batch")
    {
        return Utf8Kernel::validate(corpus.utf8);
    };

    BENCHMARK(std::string{name} + " UTF-8 validate ASCII SIMD")
    {
        return AsciiSimdKernel::validate(corpus.utf8);
    };

    BENCHMARK(std::string{name} + " UTF-8 validate UTF-8 SIMD")
    {
        return Utf8SimdKernel::validate(corpus.utf8);
    };

    BENCHMARK(std::string{name} + " UTF-8 validate policy")
    {
        return Utf8::validate(corpus.utf8);
    };
}

// -----------------------------------------------------------------------------
// Block one: ASCII grain / policy crossover
// -----------------------------------------------------------------------------
TEST_CASE("SIMD text UTF-8 ASCII grain benchmarks",
          "[core][text][benchmark][simd][utf8][grain]")
{
#ifdef JOB_TEXT_STRESS_UTF8
    constexpr std::array<std::size_t, 22> sizes{
        8,
        15,
        16,
        31,
        32,
        33,
        63,
        64,
        65,
        95,
        96,
        97,
        127,
        128,
        129,
        192,
        255,
        256,
        512,
        1024,
        4096,
        16384
    };
#else
    constexpr std::array<std::size_t, 4> sizes{ 32, 64, 128, 256 };
#endif

    for (const std::size_t size : sizes)
        benchmarkUtf8ValidateSize(size);
}

// -----------------------------------------------------------------------------
// Block two: real UTF-8 workload comparison
// -----------------------------------------------------------------------------

TEST_CASE("SIMD text UTF-8 corpus validation benchmarks",
          "[core][text][benchmark][simd][utf8][corpus]")
{
    constexpr std::size_t scalarCount = 4096;

    constexpr std::array<char32_t, 16> asciiPattern{
        U'J', U'O', U'B', U' ',
        U't', U'e', U'x', U't',
        U' ', U'b', U'e', U'n',
        U'c', U'h', U'!', U' '
    };

    constexpr std::array<char32_t, 8> bmpPattern{
        U'A',
        0x00E9,
        0x03A9,
        0x20AC,
        0x4E2D,
        0x65E5,
        U'Z',
        U' '
    };

    constexpr std::array<char32_t, 8> supplementaryPattern{
        0x1F600,
        0x1F680,
        0x1F4A1,
        0x1F642,
        0x1F525,
        0x1F44D,
        U'A',
        U' '
    };

    constexpr std::array<char32_t, 16> mixedPattern{
        U'J',
        U'O',
        U'B',
        U' ',
        0x00E9,
        U' ',
        U't',
        U'e',
        U'x',
        U't',
        U' ',
        0x20AC,
        U' ',
        0x1F680,
        U'!',
        U' '
    };

    const auto asciiCorpus = makeSimdBenchmarkCorpus(asciiPattern, scalarCount);
    const auto bmpCorpus = makeSimdBenchmarkCorpus(bmpPattern, scalarCount);
    const auto supplementaryCorpus = makeSimdBenchmarkCorpus(supplementaryPattern, scalarCount);
    const auto mixedCorpus = makeSimdBenchmarkCorpus(mixedPattern, scalarCount);
    benchmarkUtf8Validation("ASCII-heavy", asciiCorpus);
    benchmarkUtf8Validation("BMP-mixed", bmpCorpus);
    benchmarkUtf8Validation("Supplementary-heavy", supplementaryCorpus);
    benchmarkUtf8Validation("Mixed realistic", mixedCorpus);
}

// -----------------------------------------------------------------------------
// Block three: fixed byte-size multibyte scouting
// -----------------------------------------------------------------------------

TEST_CASE("SIMD text UTF-8 repeated multibyte benchmarks",
          "[core][text][benchmark][simd][utf8][multibyte]")
{
    constexpr std::size_t scalarCount = 4096;
    constexpr std::array<char32_t, 1> twoBytePattern{ 0x00E9 };
    constexpr std::array<char32_t, 1> threeBytePattern{ 0x20AC };
    constexpr std::array<char32_t, 1> fourBytePattern{ 0x1F600 };
    const auto twoByteCorpus = makeSimdBenchmarkCorpus(twoBytePattern, scalarCount);
    const auto threeByteCorpus = makeSimdBenchmarkCorpus(threeBytePattern, scalarCount);
    const auto fourByteCorpus = makeSimdBenchmarkCorpus(fourBytePattern, scalarCount);

    benchmarkUtf8Validation("Repeated 2-byte", twoByteCorpus);
    benchmarkUtf8Validation("Repeated 3-byte", threeByteCorpus);
    benchmarkUtf8Validation("Repeated 4-byte", fourByteCorpus);
}

#endif

} // namespace job::core::text::tests