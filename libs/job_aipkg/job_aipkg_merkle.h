#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "job_aipkg_utils.h"
#include "jobaipkg_export.h"

namespace job::aipkg {

class JOBAIPKG_EXPORT JobAiPkgMerkle
{
public:
    JobAiPkgMerkle() = delete;

    // H(0x00 || data)
    [[nodiscard]] static Hash32 leafHash(const std::vector<uint8_t> &data) noexcept;

    // H(0x01 || left || right) the 0x01 prefix, distinct from leafHash's 0x00.
    [[nodiscard]] static Hash32 nodeHash(const Hash32 &left, const Hash32 &right) noexcept;

    [[nodiscard]] static Hash32 computeRoot(const std::vector<Hash32> &leafHashes) noexcept;

private:
    [[nodiscard]] static Hash32 computeSubtreeRoot(const std::vector<Hash32> &leafHashes,
                                                   size_t start,
                                                   size_t count) noexcept;
};

}