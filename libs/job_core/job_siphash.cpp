#include "job_siphash.h"

#if JOB_LINUX
#include <errno.h>
#include <sys/random.h>
#elif JOB_OSX || JOB_FreeBSD
#include <stdlib.h>
#elif JOB_WINDOWS
#include <bcrypt.h>
#else
#include "job_assert.h"
#endif

namespace job::core {

std::size_t JobSipHash::operator()(std::string_view s) const noexcept
{
    return static_cast<std::size_t>(hash(s));
}

std::size_t JobSipHash::operator()(const std::string &s) const noexcept
{
    return static_cast<std::size_t>(hash(s));
}

// PUBLIC
bool JobSipHash::seed() noexcept
{
    uint64_t k0 = 0;
    uint64_t k1 = 0;

    if (!seed(&k0, &k1))
        return false;

    m_k0 = k0;
    m_k1 = k1;

    return true;
}
// uint64_t JobSipHash::hash(std::string_view s) const noexcept
// {
//     const auto chars = std::span<const char>(s.data(), s.size());

//     // Automatically fast-path 16-byte fixed UIDs through AVX when enabled
//     if (m_useAvx && s.size() == 16) {
//         return hash128(reinterpret_cast<const uint64_t *>(s.data()));
//     }

//     // Quiet, zero-overhead scalar fallback for all other string sizes
//     return siphash24Key(std::as_bytes(chars), m_k0, m_k1);
// }

uint64_t JobSipHash::hash(std::string_view s) const noexcept
{
    const auto chars = std::span<const char>(s.data(), s.size());

    if (m_useAvx && s.size() == 16) {
        const auto *data = reinterpret_cast<const std::byte *>(s.data());

        const std::uint64_t uid[2]{
            load64Le(data),
            load64Le(data + 8)
        };

        return hash128(uid);
    }

    return siphash24Key(std::as_bytes(chars), m_k0, m_k1);
}


// PRIVATE
bool JobSipHash::seed(uint64_t *k0, uint64_t *k1) noexcept
{
    if (!k0 || !k1)
        return false;

    uint64_t seeds[2] = { 0, 0 };

#if JOB_LINUX

    std::byte *buffer = reinterpret_cast<std::byte *>(seeds);
    std::size_t remaining = sizeof(seeds);
    std::size_t offset = 0;

    while (remaining > 0) {
        const ssize_t n = ::getrandom(
            buffer + offset,
            remaining,
            0
            );

        if (n > 0) {
            offset += static_cast<std::size_t>(n);
            remaining -= static_cast<std::size_t>(n);
            continue;
        }

        if (n < 0 && errno == EINTR)
            continue;

        return false;
    }

#elif JOB_OSX || JOB_FreeBSD
    arc4random_buf(seeds, sizeof(seeds));

#elif JOB_WINDOWS

    const NTSTATUS status = BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(seeds),
        static_cast<ULONG>(sizeof(seeds)),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );

    if (status < 0)
        return false;

#else
    JOB_ASSERT("OS Platform not supported. std::abort incoming")
    return false;

#endif

    *k0 = seeds[0];
    *k1 = seeds[1];

    return true;
}

} // namespace job::core