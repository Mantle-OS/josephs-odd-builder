#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include <simd_provider.h>

#include "jobcore_export.h"
#if defined(HAS_AVX) || defined(HAS_AVX_TWO) || defined(HAS_AVX_VNNI) || defined(HAS_AVX_512) || defined(HAS_AVX_512_VNNI)
#define JOB_DEFAULT_USE_AVX true
#else
#define JOB_DEFAULT_USE_AVX false
#endif
namespace job::core {

class JOBCORE_EXPORT JobSipHash {
public:
    using Ptr  = std::shared_ptr<JobSipHash>;
    using WPtr = std::weak_ptr<JobSipHash>;
    using UPtr = std::unique_ptr<JobSipHash>;

    constexpr explicit JobSipHash(uint64_t k0 = 0, uint64_t k1 = 0, bool useAvx = JOB_DEFAULT_USE_AVX) noexcept :
        m_k0(k0),
        m_k1(k1),
        m_useAvx(useAvx)
    {
    }

    [[nodiscard]] static Ptr createShared(uint64_t k0 = 0, uint64_t k1 = 0, bool useAvx = true)
    {
        return std::make_shared<JobSipHash>(k0, k1, useAvx);
    }

    [[nodiscard]] static UPtr createUniq(uint64_t k0 = 0, uint64_t k1 = 0, bool useAvx = true)
    {
        return std::make_unique<JobSipHash>(k0, k1, useAvx);
    }

    ~JobSipHash() = default;

    JobSipHash(const JobSipHash &) = default;
    JobSipHash &operator=(const JobSipHash &) = default;
    JobSipHash(JobSipHash &&) noexcept = default;
    JobSipHash &operator=(JobSipHash &&) noexcept = default;

    // Transparent hash overloads
    [[nodiscard]] std::size_t operator()(std::string_view s) const noexcept;
    [[nodiscard]] std::size_t operator()(const std::string &s) const noexcept;

    // Public seeding
    [[nodiscard]] bool seed() noexcept;

    [[nodiscard]] constexpr uint64_t hash(std::span<const std::byte> data) const noexcept
    {
        return siphash24Key(data, m_k0, m_k1);
    }

    [[nodiscard]] uint64_t hash(std::string_view s) const noexcept;
    // [[nodiscard]] uint64_t hash(const std::string &s) const noexcept;

    // Delegates directly to the job::simd API provider
    [[nodiscard]] inline job::simd::i64 hashAvx4(const std::uint64_t *uids) const noexcept
    {
        return job::simd::SIMD::siphashTile4(uids, m_k0, m_k1);
    }

    // [[nodiscard]] inline uint64_t hash128(const std::uint64_t *uid) const noexcept
    // {
    //     const auto *data = reinterpret_cast<const std::byte *>(uid);
    //     return siphash24Key(std::span<const std::byte>(data, 16), m_k0, m_k1);
    // }

    // [[nodiscard]] inline uint64_t hash128(const std::uint64_t *uid) const noexcept
    // {
    //     if (m_useAvx) {
    //         // Replicate the 16-byte UID into a 4-lane SIMD vector and extract lane 0
    //         const uint64_t uids[4] = { uid[0], uid[1], 0, 0 };
    //         const job::simd::i64 res = job::simd::SIMD::siphashTile4(uids, m_k0, m_k1);

    //         // Extract low 64-bit element from lane 0
    //         alignas(32) std::uint64_t out[4];
    //         job::simd::SIMD::mov_i64(reinterpret_cast<std::int64_t *>(out), res);
    //         return out[0];
    //     }

    //     // Fallback when m_useAvx is set to false at construction
    //     const auto *data = reinterpret_cast<const std::byte *>(uid);
    //     return siphash24Key(std::span<const std::byte>(data, 16), m_k0, m_k1);
    // }

    [[nodiscard]] inline uint64_t hash128(const std::uint64_t *uid) const noexcept
    {
        if (m_useAvx) {
            const job::simd::i64 m0 = job::simd::SIMD::set1_u64(uid[0]);
            const job::simd::i64 m1 = job::simd::SIMD::set1_u64(uid[1]);

            const job::simd::i64 k0 = job::simd::SIMD::set1_u64(m_k0);
            const job::simd::i64 k1 = job::simd::SIMD::set1_u64(m_k1);

            const job::simd::i64 result =
                job::simd::SIMD::siphash(m0, m1, k0, k1);

            alignas(32) std::uint64_t out[4];
            job::simd::SIMD::mov_i64(
                reinterpret_cast<std::int64_t *>(out),
                result
                );

            return out[0];
        }

        const auto *data = reinterpret_cast<const std::byte *>(uid);

        return siphash24Key(
            std::span<const std::byte>(data, 16),
            m_k0,
            m_k1
            );
    }

    [[nodiscard]] constexpr uint64_t k0() const noexcept { return m_k0; }
    [[nodiscard]] constexpr uint64_t k1() const noexcept { return m_k1; }
    [[nodiscard]] constexpr bool useAvx() const noexcept { return m_useAvx; }

private:
    [[nodiscard]] static bool seed(uint64_t *k0, uint64_t *k1) noexcept;
    [[nodiscard]] static constexpr uint64_t load64Le(const std::byte *data) noexcept
    {
        return std::to_integer<uint64_t>(data[0])       |
               std::to_integer<uint64_t>(data[1]) << 8  |
               std::to_integer<uint64_t>(data[2]) << 16 |
               std::to_integer<uint64_t>(data[3]) << 24 |
               std::to_integer<uint64_t>(data[4]) << 32 |
               std::to_integer<uint64_t>(data[5]) << 40 |
               std::to_integer<uint64_t>(data[6]) << 48 |
               std::to_integer<uint64_t>(data[7]) << 56;
    }

    [[nodiscard]] static constexpr uint64_t siphash24Key(std::span<const std::byte> data,
                                                         uint64_t k0,
                                                         uint64_t k1) noexcept
    {
        const std::size_t len = data.size();

        uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
        uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
        uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
        uint64_t v3 = 0x7465646279746573ULL ^ k1;

        uint64_t m = 0;
        uint64_t b = static_cast<uint64_t>(len) << 56;
        std::size_t i = 0;

#define SIPROUND() \
        do { \
                v0 += v1 ; \
                v2 += v3 ; \
                v1 = std::rotl( v1, 13 ) ; \
                v3 = std::rotl( v3, 16 ) ; \
                v1 ^= v0 ; \
                v3 ^= v2 ; \
                v0 = std::rotl( v0, 32 ) ; \
                v2 += v1 ; \
                v0 += v3 ; \
                v1 = std::rotl( v1, 17 ) ; \
                v3 = std::rotl( v3, 21 ) ; \
                v1 ^= v2 ; \
                v3 ^= v0 ; \
                v2 = std::rotl( v2, 32 ) ; \
        } while (0)

            for (; i + 8 <= len; i += 8) {
            m = load64Le(data.data() + i);
            v3 ^= m;
            SIPROUND();
            SIPROUND();
            v0 ^= m;
        }

        m = b;
        switch (len & 7) {
        case 7: m |= std::to_integer<uint64_t>(data[i + 6]) << 48; [[fallthrough]];
        case 6: m |= std::to_integer<uint64_t>(data[i + 5]) << 40; [[fallthrough]];
        case 5: m |= std::to_integer<uint64_t>(data[i + 4]) << 32; [[fallthrough]];
        case 4: m |= std::to_integer<uint64_t>(data[i + 3]) << 24; [[fallthrough]];
        case 3: m |= std::to_integer<uint64_t>(data[i + 2]) << 16; [[fallthrough]];
        case 2: m |= std::to_integer<uint64_t>(data[i + 1]) << 8;  [[fallthrough]];
        case 1: m |= std::to_integer<uint64_t>(data[i]);           [[fallthrough]];
        default: break;
        }

        v3 ^= m;
        SIPROUND();
        SIPROUND();
        v0 ^= m;

        v2 ^= 0xff;
        SIPROUND();
        SIPROUND();
        SIPROUND();
        SIPROUND();

#undef SIPROUND

        return v0 ^ v1 ^ v2 ^ v3;
    }

    uint64_t m_k0 = 0;
    uint64_t m_k1 = 0;
    bool m_useAvx = true;
};

} // namespace job::core