#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_utf32_kernel.h>
#include <job_utf32_simd_kernel.h>

#include "test_job_text_fixtures.h"

namespace job::core::text::tests {

#ifdef JOB_TEST_BENCHMARKS

static void benchmarkUtf32ValidateSize(std::size_t size)
{
    const std::vector<char32_t> input(size, U'A');

    BENCHMARK("UTF-32 validate scalar " + std::to_string(size) + " units")
    {
        return Utf32Kernel::validate(input);
    };

    BENCHMARK("UTF-32 validate SIMD " + std::to_string(size) + " units")
    {
        return Utf32SimdKernel::validate(input);
    };
}

static void benchmarkUtf32Validation(std::string_view name, const std::vector<char32_t> &input)
{
    BENCHMARK(std::string{name} + " UTF-32 validate scalar")
    {
        return Utf32Kernel::validate(input);
    };

    BENCHMARK(std::string{name} + " UTF-32 validate SIMD")
    {
        return Utf32SimdKernel::validate(input);
    };
}

template <typename Tile>
static void benchmarkUtf32Tile(std::string_view name, const std::array<char32_t, Tile::kUnitCount> &input)
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

TEST_CASE("UTF-32 SIMD size benchmarks",
          "[core][text][benchmark][simd][utf32][size]")
{
    constexpr std::array<std::size_t, 16> sizes{
        4,
        7,
        8,
        9,
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
        129
    };

    for (const std::size_t size : sizes)
        benchmarkUtf32ValidateSize(size);

    benchmarkUtf32ValidateSize(4096);
}

// -----------------------------------------------------------------------------
// Block two: direct SIMD tile behavior
// -----------------------------------------------------------------------------

TEST_CASE("UTF-32 SIMD tile benchmarks",
          "[core][text][benchmark][simd][utf32][tile]")
{
    {
        using Tile = Utf32SimdKernel::Tile1;

        std::array<char32_t, Tile::kUnitCount> ascii{};
        ascii.fill(U'A');

        std::array<char32_t, Tile::kUnitCount> boundaryMixed{};
        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < boundaryMixed.size(); ++i)
            boundaryMixed[i] = pattern[i % pattern.size()];

        std::array<char32_t, Tile::kUnitCount> highScalar{};
        constexpr std::array<char32_t, 4> highPattern{
            0x0010FFFC,
            0x0010FFFD,
            0x0010FFFE,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < highScalar.size(); ++i)
            highScalar[i] = highPattern[i % highPattern.size()];

        benchmarkUtf32Tile<Tile>("UTF-32 Tile1 ASCII", ascii);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile1 boundary-mixed", boundaryMixed);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile1 high-scalar", highScalar);
    }

    {
        using Tile = Utf32SimdKernel::Tile2;

        std::array<char32_t, Tile::kUnitCount> ascii{};
        ascii.fill(U'A');

        std::array<char32_t, Tile::kUnitCount> boundaryMixed{};
        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < boundaryMixed.size(); ++i)
            boundaryMixed[i] = pattern[i % pattern.size()];

        std::array<char32_t, Tile::kUnitCount> highScalar{};
        constexpr std::array<char32_t, 4> highPattern{
            0x0010FFFC,
            0x0010FFFD,
            0x0010FFFE,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < highScalar.size(); ++i)
            highScalar[i] = highPattern[i % highPattern.size()];

        benchmarkUtf32Tile<Tile>("UTF-32 Tile2 ASCII", ascii);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile2 boundary-mixed", boundaryMixed);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile2 high-scalar", highScalar);
    }

    {
        using Tile = Utf32SimdKernel::Tile4;

        std::array<char32_t, Tile::kUnitCount> ascii{};
        ascii.fill(U'A');

        std::array<char32_t, Tile::kUnitCount> boundaryMixed{};
        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < boundaryMixed.size(); ++i)
            boundaryMixed[i] = pattern[i % pattern.size()];

        std::array<char32_t, Tile::kUnitCount> highScalar{};
        constexpr std::array<char32_t, 4> highPattern{
            0x0010FFFC,
            0x0010FFFD,
            0x0010FFFE,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < highScalar.size(); ++i)
            highScalar[i] = highPattern[i % highPattern.size()];

        benchmarkUtf32Tile<Tile>("UTF-32 Tile4 ASCII", ascii);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile4 boundary-mixed", boundaryMixed);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile4 high-scalar", highScalar);
    }

    {
        using Tile = Utf32SimdKernel::Tile8;

        std::array<char32_t, Tile::kUnitCount> ascii{};
        ascii.fill(U'A');

        std::array<char32_t, Tile::kUnitCount> boundaryMixed{};
        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < boundaryMixed.size(); ++i)
            boundaryMixed[i] = pattern[i % pattern.size()];

        std::array<char32_t, Tile::kUnitCount> highScalar{};
        constexpr std::array<char32_t, 4> highPattern{
            0x0010FFFC,
            0x0010FFFD,
            0x0010FFFE,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < highScalar.size(); ++i)
            highScalar[i] = highPattern[i % highPattern.size()];

        benchmarkUtf32Tile<Tile>("UTF-32 Tile8 ASCII", ascii);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile8 boundary-mixed", boundaryMixed);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile8 high-scalar", highScalar);
    }

    {
        using Tile = Utf32SimdKernel::Tile16;

        std::array<char32_t, Tile::kUnitCount> ascii{};
        ascii.fill(U'A');

        std::array<char32_t, Tile::kUnitCount> boundaryMixed{};
        constexpr std::array<char32_t, 4> pattern{
            U'A',
            utf32SurrogateBefore,
            utf32SurrogateAfter,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < boundaryMixed.size(); ++i)
            boundaryMixed[i] = pattern[i % pattern.size()];

        std::array<char32_t, Tile::kUnitCount> highScalar{};
        constexpr std::array<char32_t, 4> highPattern{
            0x0010FFFC,
            0x0010FFFD,
            0x0010FFFE,
            utf32ScalarMax
        };

        for (std::size_t i = 0; i < highScalar.size(); ++i)
            highScalar[i] = highPattern[i % highPattern.size()];

        benchmarkUtf32Tile<Tile>("UTF-32 Tile16 ASCII", ascii);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile16 boundary-mixed", boundaryMixed);
        benchmarkUtf32Tile<Tile>("UTF-32 Tile16 high-scalar", highScalar);
    }
}

// -----------------------------------------------------------------------------
// Block three: ASCII-heavy validation
// -----------------------------------------------------------------------------

TEST_CASE("UTF-32 SIMD ASCII validation benchmarks",
          "[core][text][benchmark][simd][utf32][ascii]")
{
    constexpr std::size_t unitCount = 4096;

    const std::vector<char32_t> ascii(unitCount, U'A');

    benchmarkUtf32Validation("ASCII-heavy", ascii);
}

// -----------------------------------------------------------------------------
// Block four: mixed valid scalar validation
// -----------------------------------------------------------------------------

TEST_CASE("UTF-32 SIMD mixed scalar validation benchmarks",
          "[core][text][benchmark][simd][utf32][scalar]")
{
    constexpr std::size_t unitCount = 4096;

    constexpr std::array<char32_t, 8> scalarPattern{
        U'A',
        0x0000007F,
        0x000007FF,
        utf32SurrogateBefore,
        utf32SurrogateAfter,
        0x0000FFFF,
        0x00100000,
        utf32ScalarMax
    };

    std::vector<char32_t> mixed(unitCount);

    for (std::size_t i = 0; i < mixed.size(); ++i)
        mixed[i] = scalarPattern[i % scalarPattern.size()];

    benchmarkUtf32Validation("Mixed scalar", mixed);
}

// -----------------------------------------------------------------------------
// Block five: high scalar validation
// -----------------------------------------------------------------------------

TEST_CASE("UTF-32 SIMD high scalar validation benchmarks",
          "[core][text][benchmark][simd][utf32][high]")
{
    constexpr std::size_t unitCount = 4096;

    constexpr std::array<char32_t, 8> highPattern{
        0x0010FFF8,
        0x0010FFF9,
        0x0010FFFA,
        0x0010FFFB,
        0x0010FFFC,
        0x0010FFFD,
        0x0010FFFE,
        utf32ScalarMax
    };

    std::vector<char32_t> high(unitCount);

    for (std::size_t i = 0; i < high.size(); ++i)
        high[i] = highPattern[i % highPattern.size()];

    benchmarkUtf32Validation("High scalar", high);
}

#endif

} // namespace job::core::text::tests