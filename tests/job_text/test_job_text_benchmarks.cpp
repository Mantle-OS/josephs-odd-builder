#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>
#include <span>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_text.h>

namespace job::core::text::tests {

#ifdef JOB_TEST_BENCHMARKS

struct TextBenchmarkCorpus
{
    std::vector<UnicodeScalar>  scalars;
    std::vector<std::uint8_t>   utf8;
    std::vector<char16_t>       utf16;
    std::vector<char32_t>       utf32;
};

[[nodiscard]] static TextBenchmarkCorpus makeBenchmarkCorpus(std::span<const char32_t> pattern, std::size_t scalarCount)
{
    TextBenchmarkCorpus corpus{};
    corpus.scalars.reserve(scalarCount);

    for (std::size_t i = 0; i < scalarCount; ++i) {
        const auto scalar = UnicodeScalar::from(pattern[i % pattern.size()]);
        REQUIRE(scalar.has_value());
        corpus.scalars.push_back(*scalar);
    }

    corpus.utf8.resize(scalarCount * 4);
    corpus.utf16.resize(scalarCount * 2);
    corpus.utf32.resize(scalarCount);

    const auto utf8Result = Utf8::encode(corpus.scalars, corpus.utf8);
    const auto utf16Result = Utf16::encode(corpus.scalars, corpus.utf16);
    const auto utf32Result = Utf32::encode(corpus.scalars, corpus.utf32);

    REQUIRE(utf8Result.status == Utf8Status::Complete);
    REQUIRE(utf16Result.status == Utf16Status::Complete);
    REQUIRE(utf32Result.status == Utf32Status::Complete);

    corpus.utf8.resize(utf8Result.outputProduced);
    corpus.utf16.resize(utf16Result.outputProduced);
    corpus.utf32.resize(utf32Result.outputProduced);

    return corpus;
}

static void benchmarkCodecBatch(std::string_view name, const TextBenchmarkCorpus &corpus)
{
    std::vector<UnicodeScalar> scalarOutput(corpus.scalars.size());
    std::vector<std::uint8_t> utf8Output(corpus.scalars.size() * 4);
    std::vector<char16_t> utf16Output(corpus.scalars.size() * 2);
    std::vector<char32_t> utf32Output(corpus.scalars.size());

    BENCHMARK(std::string{name} + " UTF-8 decode")
    {
        return Utf8::decode(corpus.utf8, scalarOutput);
    };

    BENCHMARK(std::string{name} + " UTF-8 encode")
    {
        return Utf8::encode(corpus.scalars, utf8Output);
    };

    BENCHMARK(std::string{name} + " UTF-8 validate generic")
    {
        return Utf8Kernel::validate(corpus.utf8);
    };

    BENCHMARK(std::string{name} + " UTF-8 validate policy")
    {
        return Utf8::validate(corpus.utf8);
    };

    BENCHMARK(std::string{name} + " UTF-16 decode")
    {
        return Utf16::decode(corpus.utf16, scalarOutput);
    };

    BENCHMARK(std::string{name} + " UTF-16 encode")
    {
        return Utf16::encode(corpus.scalars, utf16Output);
    };

    BENCHMARK(std::string{name} + " UTF-16 validate generic")
    {
        return Utf16Kernel::validate(corpus.utf16);
    };

    BENCHMARK(std::string{name} + " UTF-16 validate policy")
    {
        return Utf16::validate(corpus.utf16);
    };

    BENCHMARK(std::string{name} + " UTF-32 decode")
    {
        return Utf32::decode(corpus.utf32, scalarOutput);
    };

    BENCHMARK(std::string{name} + " UTF-32 encode")
    {
        return Utf32::encode(corpus.scalars, utf32Output);
    };

    BENCHMARK(std::string{name} + " UTF-32 validate generic")
    {
        return Utf32Kernel::validate(corpus.utf32);
    };

    BENCHMARK(std::string{name} + " UTF-32 validate policy")
    {
        return Utf32::validate(corpus.utf32);
    };
}
static void benchmarkTranscoding(std::string_view name, const TextBenchmarkCorpus &corpus)
{
    std::vector<std::uint8_t> utf8Output(corpus.scalars.size() * 4);
    std::vector<char16_t> utf16Output(corpus.scalars.size() * 2);
    std::vector<char32_t> utf32Output(corpus.scalars.size());

    BENCHMARK(std::string{name} + " UTF-8 -> UTF-16")
    {
        return TextTranscoder::transcode<Utf8, Utf16>(corpus.utf8, utf16Output);
    };

    BENCHMARK(std::string{name} + " UTF-8 -> UTF-32")
    {
        return TextTranscoder::transcode<Utf8, Utf32>(corpus.utf8, utf32Output);
    };

    BENCHMARK(std::string{name} + " UTF-16 -> UTF-8")
    {
        return TextTranscoder::transcode<Utf16, Utf8>(corpus.utf16, utf8Output);
    };

    BENCHMARK(std::string{name} + " UTF-16 -> UTF-32")
    {
        return TextTranscoder::transcode<Utf16, Utf32>(corpus.utf16, utf32Output);
    };

    BENCHMARK(std::string{name} + " UTF-32 -> UTF-8")
    {
        return TextTranscoder::transcode<Utf32, Utf8>(corpus.utf32, utf8Output);
    };

    BENCHMARK(std::string{name} + " UTF-32 -> UTF-16")
    {
        return TextTranscoder::transcode<Utf32, Utf16>(corpus.utf32, utf16Output);
    };
}

TEST_CASE("Text codec batch benchmarks", "[core][text][benchmark][codec]")
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

    const auto asciiCorpus = makeBenchmarkCorpus(asciiPattern, scalarCount);
    const auto bmpCorpus = makeBenchmarkCorpus(bmpPattern, scalarCount);
    const auto supplementaryCorpus = makeBenchmarkCorpus(supplementaryPattern, scalarCount);

    benchmarkCodecBatch("ASCII-heavy", asciiCorpus);
    benchmarkCodecBatch("BMP-mixed", bmpCorpus);
    benchmarkCodecBatch("Supplementary-heavy", supplementaryCorpus);
}

TEST_CASE("Text transcoding benchmarks", "[core][text][benchmark][transcode]")
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

    const auto asciiCorpus = makeBenchmarkCorpus(asciiPattern, scalarCount);
    const auto bmpCorpus = makeBenchmarkCorpus(bmpPattern, scalarCount);
    const auto supplementaryCorpus = makeBenchmarkCorpus(supplementaryPattern, scalarCount);

    benchmarkTranscoding("ASCII-heavy", asciiCorpus);
    benchmarkTranscoding("BMP-mixed", bmpCorpus);
    benchmarkTranscoding("Supplementary-heavy", supplementaryCorpus);
}

#endif

} // namespace job::core::text::tests