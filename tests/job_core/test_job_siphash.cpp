#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_siphash.h>
#include <ctx/job_stealing_ctx.h>
#include <utils/job_parallel_for.h>

using namespace job::core;
using namespace job::simd;

namespace {

using JobUid = std::array<std::uint64_t, 2>;

constexpr std::uint64_t kSipHashK0 = UINT64_C(0x0706050403020100);
constexpr std::uint64_t kSipHashK1 = UINT64_C(0x0f0e0d0c0b0a0908);

[[nodiscard]] JobSipHash makeScalarHash()
{
    return JobSipHash{kSipHashK0, kSipHashK1, false};
}

[[nodiscard]] JobSipHash makeAutomaticHash()
{
    return JobSipHash{kSipHashK0, kSipHashK1};
}

[[nodiscard]] std::string makeUid16(std::uint64_t id)
{
    static constexpr char hex[] = "0123456789abcdef";
    std::string uid(16, '0');

    for (std::size_t i = 0; i < uid.size(); ++i) {
        const std::size_t shift = (uid.size() - 1 - i) * 4;
        uid[i] = hex[(id >> shift) & 0x0f];
    }

    return uid;
}

[[nodiscard]] std::string makeFallbackUid(std::uint64_t id)
{
    return "job-siphash-fallback-" + std::to_string(id);
}

[[nodiscard]] std::uint64_t hashUidScalar(const JobSipHash &hash, const JobUid &uid)
{
    const auto *bytes = reinterpret_cast<const std::byte *>(uid.data());
    return hash.hash(std::span<const std::byte>{bytes, sizeof(JobUid)});
}

[[nodiscard]] std::uint64_t benchScalarSipHash(const std::vector<JobUid> &uids)
{
    const JobSipHash hash = makeScalarHash();
    std::uint64_t total = 0;

    for (const JobUid &uid : uids)
        total ^= hashUidScalar(hash, uid);

    return total;
}

[[nodiscard]] std::uint64_t benchVectorSipHash(const std::vector<JobUid> &uids)
{
    const auto k0 = SIMD::set1_u64(kSipHashK0);
    const auto k1 = SIMD::set1_u64(kSipHashK1);
    alignas(32) std::int64_t results[4];
    std::uint64_t total = 0;

    std::size_t i = 0;
    for (; i + 4 <= uids.size(); i += 4) {
        alignas(32) std::uint64_t block0[4]{uids[i][0], uids[i + 1][0], uids[i + 2][0], uids[i + 3][0]};
        alignas(32) std::uint64_t block1[4]{uids[i][1], uids[i + 1][1], uids[i + 2][1], uids[i + 3][1]};

        const auto m0 = SIMD::pull_i64(reinterpret_cast<const std::int64_t *>(block0));
        const auto m1 = SIMD::pull_i64(reinterpret_cast<const std::int64_t *>(block1));
        SIMD::mov_i64(results, SIMD::siphash(m0, m1, k0, k1));

        total ^= static_cast<std::uint64_t>(results[0]) ^
                 static_cast<std::uint64_t>(results[1]) ^
                 static_cast<std::uint64_t>(results[2]) ^
                 static_cast<std::uint64_t>(results[3]);
    }

    const JobSipHash scalar = makeScalarHash();
    for (; i < uids.size(); ++i)
        total ^= hashUidScalar(scalar, uids[i]);

    return total;
}

[[nodiscard]] std::uint64_t benchTileSipHash(const std::vector<JobUid> &uids)
{
    const JobSipHash hash = makeAutomaticHash();
    alignas(32) std::uint64_t results[4];
    std::uint64_t total = 0;

    std::size_t i = 0;
    for (; i + 4 <= uids.size(); i += 4) {
        const auto *raw = reinterpret_cast<const std::uint64_t *>(&uids[i]);
        SIMD::mov_i64(reinterpret_cast<std::int64_t *>(results), hash.hashAvx4(raw));
        total ^= results[0] ^ results[1] ^ results[2] ^ results[3];
    }

    const JobSipHash scalar = makeScalarHash();
    for (; i < uids.size(); ++i)
        total ^= hashUidScalar(scalar, uids[i]);

    return total;
}

[[nodiscard]] std::uint64_t benchTileThreadedSipHash(const std::vector<JobUid> &uids, job::threads::ThreadPool &pool)
{
    constexpr std::size_t tileWidth = 4;
    const std::size_t tileCount = uids.size() / tileWidth;
    const JobSipHash hash = makeAutomaticHash();
    std::vector<std::uint64_t> tileResults(tileCount);

    job::threads::parallel_for(pool, std::size_t{0}, tileCount, [&](std::size_t tile) {
        const std::size_t i = tile * tileWidth;
        const auto *raw = reinterpret_cast<const std::uint64_t *>(&uids[i]);
        alignas(32) std::uint64_t results[4];
        SIMD::mov_i64(reinterpret_cast<std::int64_t *>(results), hash.hashAvx4(raw));
        tileResults[tile] = results[0] ^ results[1] ^ results[2] ^ results[3];
    });

    std::uint64_t total = 0;
    for (const std::uint64_t result : tileResults)
        total ^= result;

    const JobSipHash scalar = makeScalarHash();
    for (std::size_t i = tileCount * tileWidth; i < uids.size(); ++i)
        total ^= hashUidScalar(scalar, uids[i]);

    return total;
}

} // namespace

// 1 Usage / examples
TEST_CASE("JobSipHash hashes strings with an explicit key", "[core][siphash][usage]")
{
    const JobSipHash hash = makeAutomaticHash();
    const std::string value = "Joseph's Odd Builder";
    REQUIRE(hash.hash(value) == hash.hash(value));
}

TEST_CASE("JobSipHash can seed itself from the operating system", "[core][siphash][usage][seed]")
{
    JobSipHash hash{0, 0};
    REQUIRE(hash.seed());
    REQUIRE(hash.hash("Hello JOB") == hash.hash("Hello JOB"));
}

TEST_CASE("JobSipHash can be used as an unordered_map hasher", "[core][siphash][usage][unordered_map]")
{
    JobSipHash hash{0, 0};
    REQUIRE(hash.seed());

    std::unordered_map<std::string, int, JobSipHash> values{0, hash};
    values.emplace("one", 1);
    values.emplace("two", 2);
    values.emplace("three", 3);

    REQUIRE(values.at("one") == 1);
    REQUIRE(values.at("two") == 2);
    REQUIRE(values.at("three") == 3);
}

// 2 Edge cases / invariants
TEST_CASE("JobSipHash copies preserve key and dispatch state", "[core][siphash][copy]")
{
    const JobSipHash original{kSipHashK0, kSipHashK1, false};
    const JobSipHash copy = original;

    REQUIRE(copy.k0() == original.k0());
    REQUIRE(copy.k1() == original.k1());
    REQUIRE(copy.useAvx() == original.useAvx());
    REQUIRE(copy.hash("same input") == original.hash("same input"));
}

TEST_CASE("JobSipHash handles an empty string", "[core][siphash][edge][empty]")
{
    const JobSipHash hash = makeScalarHash();
    REQUIRE(hash.hash(std::string{}) == UINT64_C(0x726fdb47dd0e0e31));
}

TEST_CASE("JobSipHash matches SipHash-2-4 reference vectors", "[core][siphash][reference]")
{
    constexpr std::array<std::uint64_t, 16> expected{
        UINT64_C(0x726fdb47dd0e0e31), UINT64_C(0x74f839c593dc67fd),
        UINT64_C(0x0d6c8009d9a94f5a), UINT64_C(0x85676696d7fb7e2d),
        UINT64_C(0xcf2794e0277187b7), UINT64_C(0x18765564cd99a68d),
        UINT64_C(0xcbc9466e58fee3ce), UINT64_C(0xab0200f58b01d137),
        UINT64_C(0x93f5f5799a932462), UINT64_C(0x9e0082df0ba9e4b0),
        UINT64_C(0x7a5dbbc594ddb9f3), UINT64_C(0xf4b32f46226bada7),
        UINT64_C(0x751e8fbc860ee5fb), UINT64_C(0x14ea5627c0843d90),
        UINT64_C(0xf723ca908e7af2ee), UINT64_C(0xa129ca6149be45e5)
    };

    std::array<std::byte, 16> input{};
    for (std::size_t i = 0; i < input.size(); ++i)
        input[i] = static_cast<std::byte>(i);

    const JobSipHash hash = makeScalarHash();
    for (std::size_t len = 0; len < expected.size(); ++len) {
        INFO("Reference vector length: " << len);
        REQUIRE(hash.hash(std::span<const std::byte>{input.data(), len}) == expected[len]);
    }
}

TEST_CASE("JobSipHash string and byte APIs produce the same scalar hash", "[core][siphash][edge][bytes]")
{
    const JobSipHash hash = makeScalarHash();
    const std::string value = "same bytes, same answer";
    const auto chars = std::span<const char>(value.data(), value.size());
    REQUIRE(hash.hash(value) == hash.hash(std::as_bytes(chars)));
}

TEST_CASE("JobSipHash automatic fallback matches forced scalar", "[core][siphash][fallback]")
{
    const JobSipHash automatic = makeAutomaticHash();
    const JobSipHash scalar = makeScalarHash();
    const std::string uid = makeFallbackUid(42);

    REQUIRE(uid.size() != 16);
    REQUIRE(automatic.hash(uid) == scalar.hash(uid));
}

TEST_CASE("JobSipHash automatic 16-byte path matches scalar SipHash", "[core][siphash][simd][avx][uid16]")
{
    const JobSipHash automatic = makeAutomaticHash();
    const JobSipHash scalar = makeScalarHash();
    const std::string uid = makeUid16(UINT64_C(0x123456789abcdef0));

    REQUIRE(uid.size() == 16);
    REQUIRE(automatic.hash(uid) == scalar.hash(uid));
}

TEST_CASE("JobSipHash changes when the key changes", "[core][siphash][edge][key]")
{
    const std::string value = "same input";
    const JobSipHash first{kSipHashK0, kSipHashK1, false};
    const JobSipHash second{UINT64_C(0x1716151413121110), UINT64_C(0x1f1e1d1c1b1a1918), false};
    REQUIRE(first.hash(value) != second.hash(value));
}

TEST_CASE("JobSipHash AVX hashes four fixed-width values correctly", "[core][siphash][simd][avx]")
{
    alignas(32) std::array<std::uint64_t, 8> uids{
        UINT64_C(0x0706050403020100), UINT64_C(0x0f0e0d0c0b0a0908),
        UINT64_C(0x1716151413121110), UINT64_C(0x1f1e1d1c1b1a1918),
        UINT64_C(0x2726252423222120), UINT64_C(0x2f2e2d2c2b2a2928),
        UINT64_C(0x3736353433323130), UINT64_C(0x3f3e3d3c3b3a3938)
    };

    const JobSipHash scalar = makeScalarHash();
    const JobSipHash automatic = makeAutomaticHash();
    std::array<std::uint64_t, 4> expected{};

    for (std::size_t lane = 0; lane < expected.size(); ++lane) {
        const JobUid uid{uids[lane * 2], uids[(lane * 2) + 1]};
        expected[lane] = hashUidScalar(scalar, uid);
    }

    alignas(32) std::uint64_t actual[4];
    SIMD::mov_i64(reinterpret_cast<std::int64_t *>(actual), automatic.hashAvx4(uids.data()));

    for (std::size_t lane = 0; lane < expected.size(); ++lane) {
        INFO("SipHash AVX lane: " << lane);
        REQUIRE(actual[lane] == expected[lane]);
    }
}

TEST_CASE("SipHash scalar vector and tile paths agree", "[core][siphash][simd][tile]")
{
    constexpr std::size_t count = 64;
    std::vector<JobUid> uids(count);

    for (std::size_t i = 0; i < uids.size(); ++i) {
        uids[i][0] = UINT64_C(0x0102030405060708) + static_cast<std::uint64_t>(i);
        uids[i][1] = UINT64_C(0x1112131415161718) + static_cast<std::uint64_t>(i);
    }

    const std::uint64_t scalar = benchScalarSipHash(uids);
    const std::uint64_t vector = benchVectorSipHash(uids);
    const std::uint64_t tile = benchTileSipHash(uids);

    REQUIRE(vector == scalar);
    REQUIRE(tile == scalar);
}

// 3 Benchmarks
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobSipHash fixed short string performance", "[core][siphash][benchmark][string]")
{
    const JobSipHash hash = makeAutomaticHash();
    const std::string value = "Joseph's Odd Builder";
    BENCHMARK("SipHash-2-4 fixed short string") { return hash.hash(value); };
}

TEST_CASE("JobSipHash runtime short string performance", "[core][siphash][benchmark][string][runtime]")
{
    const JobSipHash hash = makeAutomaticHash();
    std::string value = "Joseph's Odd Builder";
    volatile char runtimeMarker = 'J';

    BENCHMARK("SipHash-2-4 runtime short string") { value[0] = runtimeMarker; return hash.hash(value); };
}

TEST_CASE("JobSipHash runtime key performance", "[core][siphash][benchmark][key][runtime]")
{
    volatile std::uint64_t runtimeK0 = kSipHashK0;
    volatile std::uint64_t runtimeK1 = kSipHashK1;
    const std::string value = "Joseph's Odd Builder";

    BENCHMARK("SipHash-2-4 runtime key") {
        const JobSipHash hash{runtimeK0, runtimeK1, false};
        return hash.hash(value);
    };
}

TEST_CASE("JobSipHash byte span performance", "[core][siphash][benchmark][bytes][runtime]")
{
    const JobSipHash hash = makeScalarHash();
    std::array<std::byte, 20> data{};
    volatile std::uint8_t runtimeByte = 0;

    BENCHMARK("SipHash-2-4 20 byte span") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), data.size()}); };
}

TEST_CASE("JobSipHash 16-byte string dispatch performance", "[core][siphash][benchmark][string][uid16]")
{
    const JobSipHash automatic = makeAutomaticHash();
    const JobSipHash scalar = makeScalarHash();
    const std::string uid = makeUid16(UINT64_C(0x123456789abcdef0));

    BENCHMARK("SipHash-2-4 automatic 16-byte string") { return automatic.hash(uid); };
    BENCHMARK("SipHash-2-4 forced scalar 16-byte string") { return scalar.hash(uid); };
}

TEST_CASE("JobSipHash fallback string dispatch performance", "[core][siphash][benchmark][string][fallback]")
{
    const JobSipHash automatic = makeAutomaticHash();
    const JobSipHash scalar = makeScalarHash();
    const std::string uid = makeFallbackUid(42);

    BENCHMARK("SipHash-2-4 automatic fallback string") { return automatic.hash(uid); };
    BENCHMARK("SipHash-2-4 forced scalar fallback string") { return scalar.hash(uid); };
}

TEST_CASE("JobSipHash input size performance", "[core][siphash][benchmark][bytes]")
{
    const JobSipHash hash = makeScalarHash();
    std::array<std::byte, 1024> data{};
    volatile std::uint8_t runtimeByte = 0x42;

    BENCHMARK("SipHash-2-4 8 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 8}); };
    BENCHMARK("SipHash-2-4 16 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 16}); };
    BENCHMARK("SipHash-2-4 32 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 32}); };
    BENCHMARK("SipHash-2-4 64 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 64}); };
    BENCHMARK("SipHash-2-4 256 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 256}); };
    BENCHMARK("SipHash-2-4 1024 bytes") { data[0] = static_cast<std::byte>(runtimeByte); return hash.hash(std::span<const std::byte>{data.data(), 1024}); };
}

TEST_CASE("JobSipHash tiled AVX four UID performance", "[core][siphash][simd][avx][tile][benchmark]")
{
    alignas(32) std::array<std::uint64_t, 8> uids{
        UINT64_C(0x0706050403020100), UINT64_C(0x0f0e0d0c0b0a0908),
        UINT64_C(0x1716151413121110), UINT64_C(0x1f1e1d1c1b1a1918),
        UINT64_C(0x2726252423222120), UINT64_C(0x2f2e2d2c2b2a2928),
        UINT64_C(0x3736353433323130), UINT64_C(0x3f3e3d3c3b3a3938)
    };

    const JobSipHash scalar = makeScalarHash();
    const JobSipHash automatic = makeAutomaticHash();
    volatile std::uint64_t runtimeWord = uids[0];

    BENCHMARK("SipHash-2-4 scalar x4 raw 16-byte UIDs") {
        uids[0] = runtimeWord;
        std::uint64_t result = 0;
        for (std::size_t lane = 0; lane < 4; ++lane) {
            const JobUid uid{uids[lane * 2], uids[(lane * 2) + 1]};
            result ^= hashUidScalar(scalar, uid);
        }
        return result;
    };

    BENCHMARK("SipHash-2-4 AVX tiled x4 raw 16-byte UIDs") {
        uids[0] = runtimeWord;
        return automatic.hashAvx4(uids.data());
    };
}

TEST_CASE("Benchmark: Scalar vs Vectorized vs Tiled SipHash", "[core][siphash][simd][benchmark]")
{
    constexpr std::size_t count = 1'000'000;
    std::vector<JobUid> uids(count);

    for (std::size_t i = 0; i < uids.size(); ++i) {
        uids[i][0] = UINT64_C(0x0102030405060708) + static_cast<std::uint64_t>(i);
        uids[i][1] = UINT64_C(0x1112131415161718) + static_cast<std::uint64_t>(i);
    }

    BENCHMARK("SipHash-2-4 forced scalar 1M") { return benchScalarSipHash(uids); };
    BENCHMARK("SipHash-2-4 vector x4 1M") { return benchVectorSipHash(uids); };
    BENCHMARK("SipHash-2-4 tile x4 1M") { return benchTileSipHash(uids); };

    job::threads::JobStealerCtx ctx{8};
    BENCHMARK("SipHash-2-4 tile x4 1M / 8 threads") { return benchTileThreadedSipHash(uids, *ctx.pool); };
}

#endif