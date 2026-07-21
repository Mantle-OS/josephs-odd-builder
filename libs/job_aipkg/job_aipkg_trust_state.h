// job_aipkg_trust_state.h
#pragma once

#include "job_aipkg_utils.h"
#include <aipkg_ledger/ledger_sth.hpp>

namespace job::aipkg {

using job::serializer::generated::AiPkgSTH;

class JobAiPkgTrustState
{
public:
    JobAiPkgTrustState() = default;

    [[nodiscard]] bool     hasTrustedState() const noexcept { return m_hasState; }
    [[nodiscard]] AiPkgSTH trustedSTH() const noexcept { return m_trusted; }

    [[nodiscard]] bool bootstrap(const AiPkgSTH &initialSTH) noexcept;
    [[nodiscard]] bool tryAdvance(const AiPkgSTH &candidate, const std::vector<Hash32> &consistencyProof) noexcept;

private:
    AiPkgSTH m_trusted{};
    bool     m_hasState{false};
};

} // namespace job::aipkg