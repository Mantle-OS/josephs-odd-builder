#pragma once

#include "job_aipkg_utils.h"
#include <aipkg_ledger/ledger_block.hpp>

namespace job::aipkg {

using job::serializer::generated::AiPkgBlock;

class JobAiPkgChain
{
public:
    JobAiPkgChain() = delete;

    // Generates the deterministic hash of the entire block payload for signature checking.
    // Explicitly zeroes out the signature slot to avoid circular dependency loops.
    [[nodiscard]] static Hash32 hashBlockPayload(const AiPkgBlock &block) noexcept;

    // Computes the absolute, un-prefixed hash of a complete block (including its signature).
    // This is what child blocks must store in their 'prev' field to link the chain securely.
    [[nodiscard]] static Hash32 hashFullBlock(const AiPkgBlock &block) noexcept;

    [[nodiscard]] static bool verifyLinkage(const AiPkgBlock &newBlock, const AiPkgBlock &previousBlock) noexcept;
};

} // namespace job::aipkg