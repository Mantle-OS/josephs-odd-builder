#include "job_siphash.h"

#include <errno.h>
#include <sys/random.h>
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

    *k0 = seeds[0];
    *k1 = seeds[1];

    return true;
}

} // namespace job::core