#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <sodium/crypto_generichash.h>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobShaGeneric
{
public:
    static constexpr std::size_t kMinHashSize = crypto_generichash_BYTES_MIN;
    static constexpr std::size_t kHashSize    = crypto_generichash_BYTES;
    static constexpr std::size_t kMaxHashSize = crypto_generichash_BYTES_MAX;

    using Hash = std::vector<unsigned char>;

    JobShaGeneric() = delete;
    ~JobShaGeneric() = delete;

    JobShaGeneric(const JobShaGeneric &) = delete;
    JobShaGeneric &operator=(const JobShaGeneric &) = delete;
    JobShaGeneric(JobShaGeneric &&) = delete;
    JobShaGeneric &operator=(JobShaGeneric &&) = delete;

    [[nodiscard]] static Hash compute(const void *data,
                                      std::size_t size,
                                      std::size_t hashSize = kHashSize) noexcept;

    [[nodiscard]] static Hash compute(std::string_view data,
                                      std::size_t hashSize = kHashSize) noexcept;

    [[nodiscard]] static std::string computeHex(const void *data,
                                                std::size_t size,
                                                std::size_t hashSize = kHashSize);

    [[nodiscard]] static std::string computeHex(std::string_view data,
                                                std::size_t hashSize = kHashSize);
};

} // namespace job::crypto