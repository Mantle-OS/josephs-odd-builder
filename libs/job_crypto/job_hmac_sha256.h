#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include <sodium/crypto_auth_hmacsha256.h>

#include "job_secure_mem.h"
#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobHmacSha256
{
public:
    static constexpr std::size_t kKeySize = crypto_auth_hmacsha256_KEYBYTES;
    static constexpr std::size_t kMacSize = crypto_auth_hmacsha256_BYTES;

    using Mac = std::array<unsigned char, kMacSize>;

    JobHmacSha256() = delete;
    ~JobHmacSha256() = delete;

    JobHmacSha256(const JobHmacSha256 &) = delete;
    JobHmacSha256 &operator=(const JobHmacSha256 &) = delete;
    JobHmacSha256(JobHmacSha256 &&) = delete;
    JobHmacSha256 &operator=(JobHmacSha256 &&) = delete;

    [[nodiscard]] static JobSecureMem generateKey() noexcept;

    [[nodiscard]] static Mac compute(const void *data,
                                     std::size_t dataSize,
                                     const void *key,
                                     std::size_t keySize) noexcept;

    [[nodiscard]] static Mac compute(const void *data,
                                     std::size_t dataSize,
                                     const JobSecureMem &key) noexcept;

    [[nodiscard]] static Mac compute(std::string_view data,
                                     const JobSecureMem &key) noexcept;

    [[nodiscard]] static JobSecureMem computeSecure(const void *data,
                                                    std::size_t dataSize,
                                                    const void *key,
                                                    std::size_t keySize) noexcept;

    [[nodiscard]] static JobSecureMem computeSecure(const void *data,
                                                    std::size_t dataSize,
                                                    const JobSecureMem &key) noexcept;

    [[nodiscard]] static JobSecureMem computeSecure(std::string_view data,
                                                    const JobSecureMem &key) noexcept;

    [[nodiscard]] static bool verify(const Mac &mac,
                                     const void *data,
                                     std::size_t dataSize,
                                     const void *key,
                                     std::size_t keySize) noexcept;

    [[nodiscard]] static bool verify(const Mac &mac,
                                     const void *data,
                                     std::size_t dataSize,
                                     const JobSecureMem &key) noexcept;

    [[nodiscard]] static bool verify(const Mac &mac,
                                     std::string_view data,
                                     const JobSecureMem &key) noexcept;
};

} // namespace job::crypto