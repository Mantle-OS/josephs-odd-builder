// job_aipkg_trust_state.cpp
#include "job_aipkg_trust_state.h"
#include "job_aipkg_consistency.h"

namespace job::aipkg {

bool JobAiPkgTrustState::bootstrap(const AiPkgSTH &initialSTH) noexcept
{
    if (m_hasState)
        return false; // Already have trusted state, bootstrap is a one-way street

    m_trusted = initialSTH;
    m_hasState = true;
    return true;
}

bool JobAiPkgTrustState::tryAdvance(const AiPkgSTH &candidate, const std::vector<Hash32> &consistencyProof) noexcept
{
    if (!m_hasState)
        return false; // Must bootstrap first, no stepping into a void

    Hash32 oldRoot{}, newRoot{};
    std::copy(m_trusted.root.begin(), m_trusted.root.end(), oldRoot.begin());
    std::copy(candidate.root.begin(), candidate.root.end(), newRoot.begin());

    // Evaluate tree continuity across historical sizes
    bool const consistent = JobAiPkgConsistency::verify(
        m_trusted.size, oldRoot,
        candidate.size, newRoot,
        consistencyProof
        );

    if (!consistent)
        return false; // Who's the boss? The consistency proof. Rejecting advance.

    m_trusted = candidate;
    return true;
}

} // namespace job::aipkg