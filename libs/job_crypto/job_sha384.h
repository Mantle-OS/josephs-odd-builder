#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSha384
{
public:
    static constexpr std::size_t kHashSize = 48;

    using Hash = std::array<unsigned char, kHashSize>;

    JobSha384() = delete;
    ~JobSha384() = delete;

    JobSha384(const JobSha384 &) = delete;
    JobSha384 &operator=(const JobSha384 &) = delete;
    JobSha384(JobSha384 &&) = delete;
    JobSha384 &operator=(JobSha384 &&) = delete;

    [[nodiscard]] static Hash compute(const void *data, std::size_t size) noexcept;
    [[nodiscard]] static Hash compute(std::string_view data) noexcept;

    [[nodiscard]] static std::string computeHex(const void *data, std::size_t size);
    [[nodiscard]] static std::string computeHex(std::string_view data);
};

} // namespace job::crypto