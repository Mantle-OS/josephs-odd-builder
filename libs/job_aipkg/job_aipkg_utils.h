#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <aipkg_ledger/ledger_block.hpp>
#include <aipkg_ledger/ledger_attestation.hpp>
#include <aipkg_ledger/ledger_delegate.hpp>
#include <aipkg_ledger/ledger_tx.hpp>

namespace job::aipkg {

using Hash32 = std::array<uint8_t, 32>;

[[nodiscard]] size_t largestPowerOfTwoLessThan(size_t n) noexcept;

// Generic, compile-time checked payload encoder.
// Clears BOTH the signature and public key slots to ensure signing and
// verification are operating on the exact same canonical blank canvas.
template <typename T, typename SigField, typename PubField>
[[nodiscard]] std::vector<uint8_t> encodeUnsignedPayload(T record,
                                                         SigField T::*sigMember,
                                                         PubField T::*pubMember) noexcept
{
    record.*sigMember = SigField{}; // Wipe signature slot
    record.*pubMember = PubField{}; // Wipe public key slot to match pre-sign state

    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> pk(&sbuf);
    record.pack_msgpack(pk);

    return std::vector<uint8_t>(sbuf.data(), sbuf.data() + sbuf.size());
}
} // namespace job::aipkg
