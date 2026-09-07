#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <sodium/crypto_hash_sha256.h>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha256
{
public:
    static constexpr std::size_t kHashSize = crypto_hash_sha256_BYTES;

    using Hash = std::array<unsigned char, kHashSize>;

    JobSha256() = delete;
    ~JobSha256() = delete;

    JobSha256(const JobSha256 &) = delete;
    JobSha256 &operator=(const JobSha256 &) = delete;
    JobSha256(JobSha256 &&) = delete;
    JobSha256 &operator=(JobSha256 &&) = delete;

    [[nodiscard]] static Hash compute(const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(std::string_view data);
};

} // namespace job::crypto