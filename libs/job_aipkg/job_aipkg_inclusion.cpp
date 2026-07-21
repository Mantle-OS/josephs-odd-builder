#include "job_aipkg_inclusion.h"
#include "job_aipkg_merkle.h"
namespace job::aipkg {

Hash32 JobAiPkgInclusion::computeRootFromPath(const Hash32 &leafHash, const AiPkgAuditPath &path) noexcept
{
    Hash32 current = leafHash;

    for (const auto &step : path.nodes) {
        // is_right == 1 means the sibling is on the right of `current`,
        // so `current` combines as the LEFT argument -- nodeHash(current, sibling).
        // is_right == 0 means the sibling is on the left, so `current`
        // combines as the RIGHT argument -- nodeHash(sibling, current).
        Hash32 sibling{};
        std::copy(step.sibling.begin(), step.sibling.end(), sibling.begin());

        current = (step.is_right == 1) ?
                      JobAiPkgMerkle::nodeHash(current, sibling) :
                      JobAiPkgMerkle::nodeHash(sibling, current);
    }

    return current;
}

bool JobAiPkgInclusion::verify(const Hash32 &leafHash, const AiPkgAuditPath &path, const Hash32 &trustedRoot) noexcept
{
    Hash32 const computed = computeRootFromPath(leafHash, path);
    return computed == trustedRoot;
}

} // namespace job::aipkg