#include "job_aipkg_merkle.h"

#include <job_hash.h>

namespace job::aipkg {

Hash32 JobAiPkgMerkle::leafHash(const std::vector<uint8_t> &data) noexcept
{
    std::vector<uint8_t> prefixed;
    prefixed.reserve(data.size() + 1);
    prefixed.push_back(0x00);
    prefixed.insert(prefixed.end(), data.begin(), data.end());

    Hash32 out{};
    auto const digest = job::crypto::JobHash::hashBuffer(prefixed, 32);
    std::copy(digest.begin(), digest.end(), out.begin());
    return out;
}

Hash32 JobAiPkgMerkle::nodeHash(const Hash32 &left, const Hash32 &right) noexcept
{
    std::vector<uint8_t> prefixed;
    prefixed.reserve(1 + left.size() + right.size());
    prefixed.push_back(0x01);
    prefixed.insert(prefixed.end(), left.begin(), left.end());
    prefixed.insert(prefixed.end(), right.begin(), right.end());

    Hash32 out{};
    auto const digest = job::crypto::JobHash::hashBuffer(prefixed, 32);
    std::copy(digest.begin(), digest.end(), out.begin());
    return out;
}

Hash32 JobAiPkgMerkle::computeSubtreeRoot(const std::vector<Hash32> &leafHashes, size_t start, size_t count) noexcept
{
    if (count == 1)
        return leafHashes[start];

    size_t const k      = largestPowerOfTwoLessThan(count);
    Hash32 const left   = computeSubtreeRoot(leafHashes, start, k);
    Hash32 const right  = computeSubtreeRoot(leafHashes, start + k, count - k);
    return nodeHash(left, right);
}

Hash32 JobAiPkgMerkle::computeRoot(const std::vector<Hash32> &leafHashes) noexcept
{
    if (leafHashes.empty()) {
        Hash32 out{};
        auto const digest = job::crypto::JobHash::hashBuffer(std::vector<uint8_t>{}, 32);
        std::copy(digest.begin(), digest.end(), out.begin());
        return out;
    }

    return computeSubtreeRoot(leafHashes, 0, leafHashes.size());
}

} // namespace job::aipkg