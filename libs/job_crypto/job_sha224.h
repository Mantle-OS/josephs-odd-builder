#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha224
{
public:
    static constexpr std::size_t kHashSize = 28;

    using Hash = std::array<unsigned char, kHashSize>;

    JobSha224() = delete;
    ~JobSha224() = delete;

    JobSha224(const JobSha224 &) = delete;
    JobSha224 &operator=(const JobSha224 &) = delete;
    JobSha224(JobSha224 &&) = delete;
    JobSha224 &operator=(JobSha224 &&) = delete;

    [[nodiscard]] static Hash compute(const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(std::string_view data);
};

} // namespace job::crypto