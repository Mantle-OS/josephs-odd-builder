#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_utf16_kernel.h>
#include <job_utf16_simd_kernel.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

#ifdef JOB_TEST_BENCHMARKS

static void benchmarkUtf16ValidateSize(std::size_t size)
{
    const std::vector<char16_t> input(size, u'A');

    BENCHMARK("UTF-16 validate scalar " + std::to_string(size) + " units")
    {
        return Utf16Kernel::validate(input);
    };

    BENCHMARK("UTF-16 validate SIMD " + std::to_string(size) + " units")
    {
        return Utf16SimdKernel::validate(input);
    };
}

static void benchmarkUtf16Validation(std::string_view name, const std::vector<char16_t> &input)
{
    BENCHMARK(std::string{name} + " UTF-16 validate scalar")
    {
        return Utf16Kernel::validate(input);
    };

    BENCHMARK(std::string{name} + " UTF-16 validate SIMD")
    {
        return Utf16SimdKernel::validate(input);
    };
}

template <typename Tile>
static void benchmarkUtf16Tile(std::string_view name, const std::array<char16_t, Tile::kUnitCount> &input)
{
    const typename Tile::Input tile{input.data(), Tile::kUnitCount};

    BENCHMARK(std::string{name})
    {
        return Tile::validate(tile);
    };
}


// -----------------------------------------------------------------------------
// Block one: SIMD scheduling / crossover
// -----------------------------------------------------------------------------

TEST_CASE("UTF-16 SIMD size benchmarks",
          "[core][text][benchmark][simd][utf16][size]")
{
    constexpr std::array<std::size_t, 14> sizes{
        8,
        15,
        16,
        17,
        31,
        32,
        33,
        63,
        64,
        65,
        127,
        128,
        129,
        4096
    };

    for (const std::size_t size : sizes)
        benchmarkUtf16ValidateSize(size);
}

// -----------------------------------------------------------------------------
// Block two: direct SIMD tile behavior
// -----------------------------------------------------------------------------

TEST_CASE("UTF-16 SIMD tile benchmarks",
          "[core][text][benchmark][simd][utf16][tile]")
{
    {
        using Tile = Utf16SimdKernel::Tile1;

        const auto bmp = makeBmpTile<Tile>();

        std::array<char16_t, Tile::kUnitCount> surrogate{};
        for (std::size_t i = 0; i < surrogate.size(); i += 2) {
            surrogate[i] = utf16HighMin;
            surrogate[i + 1] = utf16LowMin;
        }

        std::array<char16_t, Tile::kUnitCount> mixed{};
        for (std::size_t i = 0; i < mixed.size(); i += 4) {
            mixed[i] = u'A';
            mixed[i + 1] = u'B';
            mixed[i + 2] = utf16HighMin;
            mixed[i + 3] = utf16LowMin;
        }

        benchmarkUtf16Tile<Tile>("UTF-16 Tile1 BMP", bmp);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile1 surrogate", surrogate);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile1 mixed", mixed);
    }

    {
        using Tile = Utf16SimdKernel::Tile2;

        const auto bmp = makeBmpTile<Tile>();

        std::array<char16_t, Tile::kUnitCount> surrogate{};
        for (std::size_t i = 0; i < surrogate.size(); i += 2) {
            surrogate[i] = utf16HighMin;
            surrogate[i + 1] = utf16LowMin;
        }

        std::array<char16_t, Tile::kUnitCount> mixed{};
        for (std::size_t i = 0; i < mixed.size(); i += 4) {
            mixed[i] = u'A';
            mixed[i + 1] = u'B';
            mixed[i + 2] = utf16HighMin;
            mixed[i + 3] = utf16LowMin;
        }

        benchmarkUtf16Tile<Tile>("UTF-16 Tile2 BMP", bmp);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile2 surrogate", surrogate);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile2 mixed", mixed);
    }

    {
        using Tile = Utf16SimdKernel::Tile4;

        const auto bmp = makeBmpTile<Tile>();

        std::array<char16_t, Tile::kUnitCount> surrogate{};
        for (std::size_t i = 0; i < surrogate.size(); i += 2) {
            surrogate[i] = utf16HighMin;
            surrogate[i + 1] = utf16LowMin;
        }

        std::array<char16_t, Tile::kUnitCount> mixed{};
        for (std::size_t i = 0; i < mixed.size(); i += 4) {
            mixed[i] = u'A';
            mixed[i + 1] = u'B';
            mixed[i + 2] = utf16HighMin;
            mixed[i + 3] = utf16LowMin;
        }

        benchmarkUtf16Tile<Tile>("UTF-16 Tile4 BMP", bmp);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile4 surrogate", surrogate);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile4 mixed", mixed);
    }

    {
        using Tile = Utf16SimdKernel::Tile8;

        const auto bmp = makeBmpTile<Tile>();

        std::array<char16_t, Tile::kUnitCount> surrogate{};
        for (std::size_t i = 0; i < surrogate.size(); i += 2) {
            surrogate[i] = utf16HighMin;
            surrogate[i + 1] = utf16LowMin;
        }

        std::array<char16_t, Tile::kUnitCount> mixed{};
        for (std::size_t i = 0; i < mixed.size(); i += 4) {
            mixed[i] = u'A';
            mixed[i + 1] = u'B';
            mixed[i + 2] = utf16HighMin;
            mixed[i + 3] = utf16LowMin;
        }

        benchmarkUtf16Tile<Tile>("UTF-16 Tile8 BMP", bmp);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile8 surrogate", surrogate);
        benchmarkUtf16Tile<Tile>("UTF-16 Tile8 mixed", mixed);
    }
}

// -----------------------------------------------------------------------------
// Block three: BMP-heavy validation
// -----------------------------------------------------------------------------

TEST_CASE("UTF-16 SIMD BMP validation benchmarks",
          "[core][text][benchmark][simd][utf16][bmp]")
{
    constexpr std::size_t unitCount = 4096;

    std::vector<char16_t> ascii(unitCount, u'A');

    constexpr std::array<char16_t, 8> bmpPattern{
        u'A',
        0x00E9,
        0x03A9,
        0x20AC,
        0x4E2D,
        0x65E5,
        u'Z',
        u' '
    };

    std::vector<char16_t> mixedBmp(unitCount);

    for (std::size_t i = 0; i < mixedBmp.size(); ++i)
        mixedBmp[i] = bmpPattern[i % bmpPattern.size()];

    benchmarkUtf16Validation("ASCII-heavy", ascii);
    benchmarkUtf16Validation("BMP-mixed", mixedBmp);
}

// -----------------------------------------------------------------------------
// Block four: surrogate-heavy validation
// -----------------------------------------------------------------------------

TEST_CASE("UTF-16 SIMD surrogate validation benchmarks",
          "[core][text][benchmark][simd][utf16][surrogate]")
{
    constexpr std::size_t pairCount = 2048;

    std::vector<char16_t> repeatedMin;
    repeatedMin.reserve(pairCount * 2);

    std::vector<char16_t> repeatedMax;
    repeatedMax.reserve(pairCount * 2);

    std::vector<char16_t> mixed;
    mixed.reserve(pairCount * 2);

    for (std::size_t i = 0; i < pairCount; ++i) {
        repeatedMin.push_back(utf16PairMin.high);
        repeatedMin.push_back(utf16PairMin.low);

        repeatedMax.push_back(utf16PairMax.high);
        repeatedMax.push_back(utf16PairMax.low);

        if ((i & 1u) == 0) {
            mixed.push_back(u'A');
            mixed.push_back(u'B');
        } else {
            mixed.push_back(utf16PairMin.high);
            mixed.push_back(utf16PairMin.low);
        }
    }

    benchmarkUtf16Validation("Repeated minimum surrogate pair", repeatedMin);
    benchmarkUtf16Validation("Repeated maximum surrogate pair", repeatedMax);
    benchmarkUtf16Validation("Mixed BMP and surrogate", mixed);
}

// -----------------------------------------------------------------------------
// Block five: register-boundary surrogate pairs
// -----------------------------------------------------------------------------

TEST_CASE("UTF-16 SIMD boundary surrogate benchmarks",
          "[core][text][benchmark][simd][utf16][boundary]")
{
    using Tile = Utf16SimdKernel::Tile8;

    constexpr std::size_t totalUnits = 4096;

    std::vector<char16_t> registerBoundary(totalUnits, u'A');

    for (std::size_t offset = Tile::kRegisterUnits;
         offset < registerBoundary.size();
         offset += Tile::kRegisterUnits) {
        registerBoundary[offset - 1] = utf16PairMin.high;
        registerBoundary[offset] = utf16PairMin.low;
    }

    std::vector<char16_t> tileBoundary(totalUnits, u'A');

    for (std::size_t offset = Tile::kUnitCount;
         offset < tileBoundary.size();
         offset += Tile::kUnitCount) {
        tileBoundary[offset - 1] = utf16PairMin.high;
        tileBoundary[offset] = utf16PairMin.low;
    }

    benchmarkUtf16Validation("Register-boundary surrogate pairs", registerBoundary);
    benchmarkUtf16Validation("Tile-boundary surrogate pairs", tileBoundary);
}

#endif

} // namespace job::core::text::tests