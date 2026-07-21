#include <vector>

#include <catch2/catch_all.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <simd_provider.h>
#include <transpose.h>

using namespace job::simd;

TEST_CASE("transpose then transpose back is lossless, matching the StencilAdapter round trip", "[simd][transpose]")
{
    constexpr int seq = 64;
    constexpr int dim = 32;

    std::vector<float> src(seq * dim);
    for (int i = 0; i < seq * dim; ++i)
        src[i] = static_cast<float>(i);

    std::vector<float> transposed(seq * dim);
    std::vector<float> roundTripped(seq * dim);

    transpose(src.data(), transposed.data(), seq, dim);
    transpose(transposed.data(), roundTripped.data(), dim, seq);

    REQUIRE(roundTripped == src);
}

TEST_CASE("transpose produces the mathematically correct layout, matching LowRankAdapter's K->KT step", "[simd][transpose]")
{
    // LowRankAdapter::apply calls transpose(k_ptr, kt_ptr, S, D) to turn
    // a row-major [S,D] buffer into [D,S] before feeding it to sgemm.
    constexpr int seq = 16;
    constexpr int dim = 8;

    std::vector<float> src(seq * dim);
    for (int row = 0; row < seq; ++row)
        for (int col = 0; col < dim; ++col)
            src[row * dim + col] = static_cast<float>(row * 100 + col);

    std::vector<float> dst(seq * dim);
    transpose(src.data(), dst.data(), seq, dim);

    // dst is [dim, seq]: dst[col][row] should equal src[row][col].
    for (int row = 0; row < seq; ++row)
        for (int col = 0; col < dim; ++col)
            REQUIRE(dst[col * seq + row] == src[row * dim + col]);
}

TEST_CASE("transpose of a square matrix matches a hand-checked 8x8 block", "[simd][transpose]")
{
    // Exercises transpose_kernel_8x8 directly, one full SIMD::width() block.
    constexpr int n = 8;
    std::vector<float> src(n * n);
    for (int i = 0; i < n * n; ++i)
        src[i] = static_cast<float>(i);

    std::vector<float> dst(n * n);
    transpose(src.data(), dst.data(), n, n);

    for (int row = 0; row < n; ++row)
        for (int col = 0; col < n; ++col)
            REQUIRE(dst[col * n + row] == src[row * n + col]);
}

// 2
TEST_CASE("transpose handles dimensions that are not multiples of SIMD::width()", "[simd][transpose][edge]")
{
    // 19 and 11 both leave a remainder against width()==8, exercising
    // the scalar fallback loops at the bottom of transpose().
    constexpr int rows = 19;
    constexpr int cols = 11;

    std::vector<float> src(rows * cols);
    for (int i = 0; i < rows * cols; ++i)
        src[i] = static_cast<float>(i);

    std::vector<float> dst(rows * cols);
    transpose(src.data(), dst.data(), rows, cols);

    for (int row = 0; row < rows; ++row)
        for (int col = 0; col < cols; ++col)
            REQUIRE(dst[col * rows + row] == src[row * cols + col]);
}

TEST_CASE("transpose handles a single row and a single column", "[simd][transpose][edge]")
{
    constexpr int n = 5;

    std::vector<float> rowSrc(1 * n);
    for (int i = 0; i < n; ++i)
        rowSrc[i] = static_cast<float>(i);
    std::vector<float> rowDst(n * 1);
    transpose(rowSrc.data(), rowDst.data(), 1, n);
    REQUIRE(rowDst == rowSrc);

    std::vector<float> colSrc(n * 1);
    for (int i = 0; i < n; ++i)
        colSrc[i] = static_cast<float>(i);
    std::vector<float> colDst(1 * n);
    transpose(colSrc.data(), colDst.data(), n, 1);
    REQUIRE(colDst == colSrc);
}

TEST_CASE("transpose smaller than SIMD::width() falls through to the scalar path entirely", "[simd][transpose][edge]")
{
    // 3x3 never reaches transpose_kernel_8x8/4x4
    constexpr int n = 3;
    std::vector<float> src = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<float> dst(n * n);

    transpose(src.data(), dst.data(), n, n);

    for (int row = 0; row < n; ++row)
        for (int col = 0; col < n; ++col)
            REQUIRE(dst[col * n + row] == src[row * n + col]);
}

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("Benchmark: Scalar Loop vs Vectorized Kernel (transpose)", "[simd][transpose][benchmark]")
{
    constexpr int rows = 1024;
    constexpr int cols = 1024;
    std::vector<float> src(rows * cols);
    for (int i = 0; i < rows * cols; ++i)
        src[i] = static_cast<float>(i);

    const float* src_float = src.data();
    std::vector<float> dstBuffer(rows * cols);
    float* dst_float = dstBuffer.data();

    BENCHMARK("transpose (Scalar)")
    {
        std::vector<float> dst(rows * cols);
        for (int row = 0; row < rows; ++row)
            for (int col = 0; col < cols; ++col)
                dst[col * rows + row] = src[row * cols + col];
        return dst[0];
    };

    BENCHMARK("transpose Tile (Fresh Alloc)")
    {
        std::vector<float> dst(rows * cols);
        transpose(src.data(), dst.data(), rows, cols);
        return dst[0];
    };

    BENCHMARK("transpose Tile (Reused Buffer)")
    {
        transpose(src_float, dst_float, rows, cols);
        return dst_float[0];
    };
}
#endif