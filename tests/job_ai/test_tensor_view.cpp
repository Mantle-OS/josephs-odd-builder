#include <catch2/catch_test_macros.hpp>

#include <array>
#include <vector>

#include <real_type.h>
#include <extents.h>
#include <view.h>
#include <fiber.h>
#include <matrix.h>
#include <volume.h>
#include <batch.h>
#include <rank.h>

using job::core::real_t;
using namespace job::ai::cords;

// Tiny helper to fill with pattern: val = base + linear_index
static void fillLinear(real_t *data, std::size_t n, real_t base)
{
    for (std::size_t i = 0; i < n; ++i)
        data[i] = base + static_cast<real_t>(i);
}

TEST_CASE("Extents basic construction", "[cords][extents]")
{
    Extents<4> s1{2u, 3u};
    REQUIRE(s1.size() == 2);
    REQUIRE(s1[0] == 2u);
    REQUIRE(s1[1] == 3u);

    std::array<std::uint32_t, 3> dims{4u, 5u, 6u};
    Extents<4> s2(dims.begin(), dims.end());
    REQUIRE(s2.size() == 3);
    REQUIRE(s2[0] == 4u);
    REQUIRE(s2[1] == 5u);
    REQUIRE(s2[2] == 6u);
}

TEST_CASE("View 1D indexing", "[cords][view][1d]")
{
    std::vector<real_t> storage(5);
    fillLinear(storage.data(), storage.size(), real_t(10)); // 10,11,12,13,14

    ViewR v(storage.data(), ViewR::Extent{5u});
    REQUIRE(v.rank() == 1);
    REQUIRE(v.size() == 5);

    for (std::size_t i = 0; i < v.size(); ++i) {
        REQUIRE(v[i] == storage[i]);
        REQUIRE(v(i) == storage[i]);
    }
}

TEST_CASE("View reshape preserves layout", "[cords][view][reshape]")
{
    std::vector<real_t> storage(6);
    fillLinear(storage.data(), storage.size(), real_t(0)); // 0..5

    ViewR v1(storage.data(), ViewR::Extent{6u});
    auto v2 = v1.reshape(ViewR::Extent{2u, 3u});

    REQUIRE(v2.rank() == 2);
    REQUIRE(v2.size() == 6);

    // Check row-major layout
    REQUIRE(v2(0, 0) == storage[0]);
    REQUIRE(v2(0, 1) == storage[1]);
    REQUIRE(v2(0, 2) == storage[2]);
    REQUIRE(v2(1, 0) == storage[3]);
    REQUIRE(v2(1, 1) == storage[4]);
    REQUIRE(v2(1, 2) == storage[5]);
}

TEST_CASE("View slice drops first dimension", "[cords][view][slice]")
{
    const std::uint32_t B = 3;
    const std::uint32_t C = 2;

    std::vector<real_t> storage(B * C);
    fillLinear(storage.data(), storage.size(), real_t(100)); // 100..105

    ViewR v(storage.data(), ViewR::Extent{B, C});
    REQUIRE(v.rank() == 2);

    // slice(b) should give shape [C]
    for (std::uint32_t b = 0; b < B; ++b) {
        auto s = v.slice(b);
        REQUIRE(s.rank() == 1);
        REQUIRE(s.size() == C);

        for (std::uint32_t c = 0; c < C; ++c) {
            // Original index in flat storage
            std::size_t idx = b * C + c;
            REQUIRE(s[c] == storage[idx]);
        }
    }
}

TEST_CASE("Matrix at() matches operator()", "[cords][matrix]")
{
    const std::uint32_t rows = 3;
    const std::uint32_t cols = 4;

    std::vector<real_t> storage(rows * cols);
    fillLinear(storage.data(), storage.size(), real_t(0));

    Matrix mat(storage.data(), rows, cols);
    REQUIRE(mat.rank() == 2);
    REQUIRE(mat.rows() == rows);
    REQUIRE(mat.cols() == cols);

    for (std::uint32_t r = 0; r < rows; ++r)
        for (std::uint32_t c = 0; c < cols; ++c) {
            REQUIRE(mat.at(r, c) == mat(r, c));
        }
}

TEST_CASE("Volume and Batch basic layout", "[cords][volume][batch]")
{
    // 3D: [D0, D1, D2]
    {
        const std::uint32_t d0 = 2, d1 = 3, d2 = 4;
        std::vector<real_t> storage(d0 * d1 * d2);
        fillLinear(storage.data(), storage.size(), real_t(0));

        Volume t3(storage.data(), d0, d1, d2);
        REQUIRE(t3.rank() == 3);

        std::size_t idx = 0;
        for (std::uint32_t i = 0; i < d0; ++i)
            for (std::uint32_t j = 0; j < d1; ++j)
                for (std::uint32_t k = 0; k < d2; ++k, ++idx)
                    REQUIRE(t3.at(i, j, k) == storage[idx]);
    }

    // 4D: [B, C, H, W]
    {
        const std::uint32_t B = 2, C = 2, H = 2, W = 3;
        std::vector<real_t> storage(B * C * H * W);
        fillLinear(storage.data(), storage.size(), real_t(5));

        Batch t4(storage.data(), B, C, H, W);
        REQUIRE(t4.rank() == 4);

        std::size_t idx = 0;
        for (std::uint32_t b = 0; b < B; ++b)
            for (std::uint32_t c = 0; c < C; ++c)
                for (std::uint32_t h = 0; h < H; ++h)
                    for (std::uint32_t w = 0; w < W; ++w, ++idx)
                        REQUIRE(t4.at(b, c, h, w) == storage[idx]);
    }
}

TEST_CASE("Rank matches View indexing", "[cords][rank]")
{
    const std::uint32_t d0 = 2, d1 = 2, d2 = 3;
    std::vector<real_t> storage(d0 * d1 * d2);
    fillLinear(storage.data(), storage.size(), real_t(7));

    // std::array<std::uint32_t, 3> dims{d0, d1, d2};
    Volume tr(storage.data(), d0, d1, d2);
    ViewR v(storage.data(), ViewR::Extent{d0, d1, d2});

    for (std::uint32_t i = 0; i < d0; ++i)
        for (std::uint32_t j = 0; j < d1; ++j)
            for (std::uint32_t k = 0; k < d2; ++k) {
                REQUIRE(tr.at(i, j, k) == v(i, j, k));
            }
}


TEST_CASE("ViewIter slices 2D along first dimension", "[cords][iter][2d]")
{
    const std::uint32_t B = 3;
    const std::uint32_t C = 4;

    std::vector<real_t> storage(B * C);
    fillLinear(storage.data(), storage.size(), real_t(10)); // 10..21

    ViewR v(storage.data(), ViewR::Extent{B, C});
    REQUIRE(v.rank() == 2);
    REQUIRE(v.extent()[0] == B);
    REQUIRE(v.extent()[1] == C);

    std::uint32_t b = 0;
    for (auto it = beginSlices(v); it != endSlices(v); ++it, ++b) {
        auto row = *it; // ViewR with shape [C]
        REQUIRE(row.rank() == 1);
        REQUIRE(row.size() == C);

        for (std::uint32_t c = 0; c < C; ++c) {
            std::size_t idx = b * C + c;
            REQUIRE(row[c] == storage[idx]);
        }
    }

    REQUIRE(b == B); // we saw exactly B slices
}


TEST_CASE("ViewIter member begin/end and free beginSlices/endSlices agree on 2D",
          "[cords][iter][2d][api]")
{
    const std::uint32_t B = 3;
    const std::uint32_t C = 4;

    std::vector<real_t> storage(B * C);
    fillLinear(storage.data(), storage.size(), real_t(10)); // 10..21

    ViewR v(storage.data(), ViewR::Extent{B, C});
    REQUIRE(v.rank() == 2);

    // Member begin()/end()
    {
        std::uint32_t b = 0;
        for (auto it = v.begin(); it != v.end(); ++it, ++b) {
            auto row = *it; // shape [C]
            REQUIRE(row.rank() == 1);
            REQUIRE(row.size() == C);

            for (std::uint32_t c = 0; c < C; ++c) {
                const std::size_t idx = b * C + c;
                REQUIRE(row[c] == storage[idx]);
            }
        }
        REQUIRE(b == B);
    }

    // Free beginSlices()/endSlices()
    {
        std::uint32_t b = 0;
        for (auto it = beginSlices(v); it != endSlices(v); ++it, ++b) {
            auto row = *it; // shape [C]
            REQUIRE(row.rank() == 1);
            REQUIRE(row.size() == C);

            for (std::uint32_t c = 0; c < C; ++c) {
                const std::size_t idx = b * C + c;
                REQUIRE(row[c] == storage[idx]);
            }
        }
        REQUIRE(b == B);
    }
}

TEST_CASE("ViewIter member and free APIs agree on 3D batch slices",
          "[cords][iter][3d][api]")
{
    const std::uint32_t B = 2;
    const std::uint32_t C = 3;
    const std::uint32_t H = 4;

    std::vector<real_t> storage(B * C * H);
    fillLinear(storage.data(), storage.size(), real_t(0)); // 0..23

    ViewR v(storage.data(), ViewR::Extent{B, C, H});
    REQUIRE(v.rank() == 3);

    auto checkSlices = [&](auto beginFn, auto endFn) {
        std::uint32_t b = 0;
        for (auto it = beginFn(v); it != endFn(v); ++it, ++b) {
            auto slice = *it; // [C, H]
            REQUIRE(slice.rank() == 2);
            REQUIRE(slice.extent()[0] == C);
            REQUIRE(slice.extent()[1] == H);

            for (std::uint32_t c = 0; c < C; ++c)
                for (std::uint32_t h = 0; h < H; ++h) {
                    const std::size_t idx = b * (C * H) + c * H + h;
                    REQUIRE(slice(c, h) == storage[idx]);
                }
        }
        REQUIRE(b == B);
    };

    // Member begin/end
    checkSlices(
        [](const ViewR &tv) { return tv.begin(); },
        [](const ViewR &tv) { return tv.end(); });

    // Free beginSlices/endSlices
    checkSlices(
        [](const ViewR &tv) { return beginSlices(tv); },
        [](const ViewR &tv) { return endSlices(tv); });
}


TEST_CASE("ViewIter works with const View", "[cords][iter][const]")
{
    const std::uint32_t B = 2;
    const std::uint32_t C = 2;

    std::vector<real_t> storage(B * C);
    fillLinear(storage.data(), storage.size(), real_t(1));

    const ViewR v(storage.data(), ViewR::Extent{B, C});

    // member
    {
        std::uint32_t count = 0;
        for (auto it = v.begin(); it != v.end(); ++it)
            ++count;
        REQUIRE(count == B);
    }

    // free
    {
        std::uint32_t count = 0;
        for (auto it = beginSlices(v); it != endSlices(v); ++it)
            ++count;
        REQUIRE(count == B);
    }
}
