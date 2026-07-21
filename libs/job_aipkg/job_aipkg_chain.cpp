#include "job_aipkg_chain.h"

#include <job_hash.h>
#include <job_serializer_msgpack.h>

namespace job::aipkg {

Hash32 JobAiPkgChain::hashBlockPayload(const AiPkgBlock &block) noexcept
{
    AiPkgBlock unsigned_ = block;
    unsigned_.sig = std::array<uint8_t, 64>{}; // Zeroed out: payload only

    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> pk(&sbuf);
    unsigned_.pack_msgpack(pk);

    std::vector<uint8_t> const encoded(sbuf.data(), sbuf.data() + sbuf.size());
    auto const digest = job::crypto::JobHash::hashBuffer(encoded, 32);

    Hash32 out{};
    std::copy(digest.begin(), digest.end(), out.begin());
    return out;
}

Hash32 JobAiPkgChain::hashFullBlock(const AiPkgBlock &block) noexcept
{
    // Includes the signature! The chain link must freeze the exact signature
    // that validated the state, not just the raw data packet.
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> pk(&sbuf);
    const_cast<AiPkgBlock&>(block).pack_msgpack(pk);

    std::vector<uint8_t> const encoded(sbuf.data(), sbuf.data() + sbuf.size());
    auto const digest = job::crypto::JobHash::hashBuffer(encoded, 32);

    Hash32 out{};
    std::copy(digest.begin(), digest.end(), out.begin());
    return out;
}

bool JobAiPkgChain::verifyLinkage(const AiPkgBlock &newBlock, const AiPkgBlock &previousBlock) noexcept
{
    if (newBlock.height != previousBlock.height + 1)
        return false;

    // Secure linear linkage constraint:
    Hash32 const expectedPrev = hashFullBlock(previousBlock);
    return std::equal(expectedPrev.begin(), expectedPrev.end(), newBlock.prev.begin());
}

} // namespace job::aipkg