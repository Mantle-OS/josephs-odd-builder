#pragma once

#include <aipkg_ledger/ledger_audit_node.hpp>
#include <aipkg_ledger/ledger_audit_path.hpp>

#include "job_aipkg_utils.h"

namespace job::aipkg {
using job::serializer::generated::AiPkgAuditNode;
using job::serializer::generated::AiPkgAuditPath;
class JobAiPkgInclusion
{    
public:
    JobAiPkgInclusion() = delete;
    [[nodiscard]] static bool verify(const Hash32 &leafHash, const AiPkgAuditPath &path, const Hash32 &trustedRoot) noexcept;
    [[nodiscard]] static Hash32 computeRootFromPath(const Hash32 &leafHash, const serializer::generated::AiPkgAuditPath &path) noexcept;
};

} // namespace job::aipkg