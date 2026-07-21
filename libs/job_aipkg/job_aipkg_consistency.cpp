#include "job_aipkg_consistency.h"
#include "job_aipkg_utils.h"
#include "job_aipkg_merkle.h"
#include <functional>

namespace job::aipkg {
bool JobAiPkgConsistency::verify(uint64_t oldSize, const Hash32 &oldRoot,
                                 uint64_t newSize, const Hash32 &newRoot,
                                 const std::vector<Hash32> &proof) noexcept
{
    if (oldSize == 0 || oldSize > newSize)
        return false;

    if (oldSize == newSize)
        return proof.empty() && oldRoot == newRoot;

    size_t idx = 0;
    size_t const total = proof.size();

    // Reconstructs RFC 6962 SUBPROOF(m, D[n], b)
    // Returns {oldSubroot, newSubroot}
    std::function<std::pair<Hash32, Hash32>(uint64_t, uint64_t, bool)> verifySubproof =
        [&](uint64_t m, uint64_t n, bool b) -> std::pair<Hash32, Hash32> {

        // Base Case: m == n
        if (m == n) {
            if (b) {
                // SUBPROOF(m, D[m], true) = {}
                // Pristine boundary matching the old trusted tree root perfectly.
                return {oldRoot, oldRoot};
            } else {
                // SUBPROOF(m, D[m], false) = {MTH(D[m])}
                // Internal node hash pulled verbatim from the proof.
                if (idx >= total) return {Hash32{}, Hash32{}};
                Hash32 const h = proof[idx++];
                return {h, h};
            }
        }

        size_t const k = largestPowerOfTwoLessThan(static_cast<size_t>(n));

        if (m <= k) {
            // SUBPROOF(m, D[n], b) = SUBPROOF(m, D[0:k], b) : MTH(D[k:n])
            auto [oldLeft, newLeft] = verifySubproof(m, k, b);

            if (idx >= total) return {Hash32{}, Hash32{}};
            Hash32 const right = proof[idx++];

            // If 'b' is active and we are exactly at the oldSize boundary (m == oldSize),
            // then oldLeft is already the complete, fully-validated oldRoot.
            // We preserve it exactly rather than re-hashing it.
            Hash32 const oldSubroot = (b && m == oldSize) ? oldRoot : oldLeft;
            Hash32 const newSubroot = JobAiPkgMerkle::nodeHash(newLeft, right);

            return {oldSubroot, newSubroot};
        } else {
            // SUBPROOF(m, D[n], b) = SUBPROOF(m - k, D[k:n], false) : MTH(D[0:k])
            auto [oldRight, newRight] = verifySubproof(m - k, n - k, false);

            if (idx >= total) return {Hash32{}, Hash32{}};
            Hash32 const left = proof[idx++];

            Hash32 const oldSubroot = JobAiPkgMerkle::nodeHash(left, oldRight);
            Hash32 const newSubroot = JobAiPkgMerkle::nodeHash(left, newRight);

            return {oldSubroot, newSubroot};
        }
    };

    auto [computedOldRoot, computedNewRoot] = verifySubproof(oldSize, newSize, true);

    if (idx != total)
        return false;

    return computedOldRoot == oldRoot && computedNewRoot == newRoot;
}
} // namespace job::aipkg