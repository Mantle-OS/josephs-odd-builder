#pragma once

#include <cstdint>
#include <vector>

#include "job_aipkg_utils.h"

namespace job::aipkg {
class JobAiPkgConsistency
{
public:
    JobAiPkgConsistency() = delete;

    [[nodiscard]] static bool verify(uint64_t oldSize, const Hash32 &oldRoot,
                                     uint64_t newSize, const Hash32 &newRoot,
                                     const std::vector<Hash32> &proof) noexcept;
};

} // namespace job::aipkg