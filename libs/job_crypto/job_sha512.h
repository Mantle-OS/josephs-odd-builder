#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include <sodium/crypto_hash_sha512.h>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha512
{
public:
    static constexpr std::size_t kHashSize = crypto_hash_sha512_BYTES;

    using Hash = std::array<unsigned char, kHashSize>;

    JobSha512() = delete;
    ~JobSha512() = delete;

    JobSha512(const JobSha512 &) = delete;
    JobSha512 &operator=(const JobSha512 &) = delete;
    JobSha512(JobSha512 &&) = delete;
    JobSha512 &operator=(JobSha512 &&) = delete;

    [[nodiscard]] static Hash compute(const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(std::string_view data);
};

} // namespace job::crypto