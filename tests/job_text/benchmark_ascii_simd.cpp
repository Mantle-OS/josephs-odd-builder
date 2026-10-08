#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_text.h>

namespace job::core::text::tests {

#ifdef JOB_TEST_BENCHMARKS

static void benchmarkAsciiValidateSize(std::size_t size)
{
    const std::vector<std::uint8_t> input(size, static_cast<std::uint8_t>('A'));

    BENCHMARK("ASCII validate batch " + std::to_string(size) + " bytes")
    {
        return Utf8Kernel::validate(input);
    };

    BENCHMARK("ASCII validate specialist " + std::to_string(size) + " bytes")
    {
        return AsciiSimdKernel::validate(input);
    };

    BENCHMARK("ASCII validate UTF-8 SIMD " + std::to_string(size) + " bytes")
    {
        return Utf8SimdKernel::validate(input);
    };
}

// -----------------------------------------------------------------------------
// Permanent benchmark: exact SIMD decomposition
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD exact decomposition benchmarks",
          "[core][text][benchmark][simd][ascii][decomposition]")
{
    constexpr std::array<std::size_t, 12> sizes{
        32,
        64,
        96,
        128,
        160,
        192,
        224,
        256,
        512,
        1024,
        2048,
        4096
    };

    for (const std::size_t size : sizes)
        benchmarkAsciiValidateSize(size);
}

#ifdef JOB_TEXT_STRESS_UTF8

// -----------------------------------------------------------------------------
// Stress block one: register boundary grain
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD register grain benchmarks",
          "[core][text][benchmark][simd][ascii][grain][stress]")
{
    constexpr std::array<std::size_t, 24> sizes{
        1,
        8,
        15,
        16,
        24,
        31,
        32,
        33,
        47,
        48,
        63,
        64,
        65,
        95,
        96,
        97,
        127,
        128,
        129,
        191,
        192,
        193,
        255,
        256
    };

    for (const std::size_t size : sizes)
        benchmarkAsciiValidateSize(size);
}

// -----------------------------------------------------------------------------
// Stress block two: scalar tail cost
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD scalar tail benchmarks",
          "[core][text][benchmark][simd][ascii][tail][stress]")
{
    constexpr std::array<std::size_t, 18> sizes{
        128,
        129,
        143,
        159,
        160,
        161,
        175,
        191,
        192,
        193,
        207,
        223,
        224,
        225,
        239,
        255,
        256,
        257
    };

    for (const std::size_t size : sizes)
        benchmarkAsciiValidateSize(size);
}

// -----------------------------------------------------------------------------
// Stress block three: hot-cache throughput scaling
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD hot-cache scaling benchmarks",
          "[core][text][benchmark][simd][ascii][scaling][stress]")
{
    constexpr std::array<std::size_t, 10> sizes{
        256,
        512,
        1024,
        2048,
        4096,
        8192,
        16384,
        32768,
        65536,
        131072
    };

    for (const std::size_t size : sizes)
        benchmarkAsciiValidateSize(size);
}

// -----------------------------------------------------------------------------
// Stress block four: hot saturation
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD saturation benchmarks",
          "[core][text][benchmark][simd][ascii][saturation][stress]")
{
    constexpr std::array<std::size_t, 12> sizes{
        4 * 1024,
        8 * 1024,
        16 * 1024,
        32 * 1024,
        64 * 1024,
        128 * 1024,
        256 * 1024,
        512 * 1024,
        1 * 1024 * 1024,
        2 * 1024 * 1024,
        4 * 1024 * 1024,
        8 * 1024 * 1024
    };

    for (const std::size_t size : sizes) {
        const std::vector<std::uint8_t> input(
            size,
            static_cast<std::uint8_t>('A'));

        BENCHMARK("ASCII saturation hot " + std::to_string(size) + " bytes")
        {
            return AsciiSimdKernel::validate(input);
        };
    }
}

// -----------------------------------------------------------------------------
// Stress block five: streaming saturation
// -----------------------------------------------------------------------------

TEST_CASE("ASCII SIMD streaming saturation benchmarks",
          "[core][text][benchmark][simd][ascii][saturation][streaming][stress]")
{
    constexpr std::size_t workingSetSize = 256 * 1024 * 1024;
    const std::vector<std::uint8_t> input(
        workingSetSize,
        static_cast<std::uint8_t>('A'));

    constexpr std::array<std::size_t, 9> chunkSizes{
        4 * 1024,
        8 * 1024,
        16 * 1024,
        32 * 1024,
        64 * 1024,
        128 * 1024,
        256 * 1024,
        512 * 1024,
        1 * 1024 * 1024
    };

    for (const std::size_t chunkSize : chunkSizes) {
        std::size_t offset = 0;

        BENCHMARK("ASCII saturation streaming " + std::to_string(chunkSize) + " byte chunks")
        {
            if (offset + chunkSize > input.size())
                offset = 0;

            const std::span<const std::uint8_t> chunk{
                input.data() + offset,
                chunkSize
            };

            offset += chunkSize;

            return AsciiSimdKernel::validate(chunk);
        };
    }
}

#endif // JOB_TEXT_STRESS_UTF8

#endif // JOB_TEST_BENCHMARKS

} // namespace job::core::text::tests