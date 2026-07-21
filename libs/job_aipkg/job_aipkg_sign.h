// job_aipkg_sign.h
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

#include <job_crypto_sign.h>

#include "job_aipkg_utils.h"

#include <aipkg_ledger/ledger_block.hpp>
#include <aipkg_ledger/ledger_attestation.hpp>
#include <aipkg_ledger/ledger_delegate.hpp>
#include <aipkg_ledger/ledger_tx.hpp>
#include <aipkg_ledger/ledger_revoke.hpp>
namespace job::aipkg {

using job::serializer::generated::AiPkgBlock;
using job::serializer::generated::AiPkgAttestation;
using job::serializer::generated::AiPkgDelegate;
using job::serializer::generated::AiPkgTx;
using job::serializer::generated::AiPkgRevoke;

class JobAiPkgSign
{
public:
    JobAiPkgSign() = default;

    [[nodiscard]] bool loadKeyPair(const std::filesystem::path &publicKeyFile,
                                   const std::filesystem::path &privateKeyFile) noexcept;

    [[nodiscard]] bool hasSigningKeys() const noexcept;

    [[nodiscard]] bool signBlock(AiPkgBlock &block) noexcept;
    [[nodiscard]] bool signAttestation(AiPkgAttestation &attestation) noexcept;
    [[nodiscard]] bool signDelegate(AiPkgDelegate &delegate) noexcept;
    [[nodiscard]] bool signTx(AiPkgTx &tx) noexcept;

    [[nodiscard]] static bool verifyBlock(const AiPkgBlock &block) noexcept;
    [[nodiscard]] static bool verifyAttestation(const AiPkgAttestation &attestation) noexcept;
    [[nodiscard]] static bool verifyDelegate(const AiPkgDelegate &delegate) noexcept;
    [[nodiscard]] static bool verifyTx(const AiPkgTx &tx) noexcept;


    [[nodiscard]] bool signRevoke(AiPkgRevoke &revoke) noexcept;
    [[nodiscard]] static bool verifyRevoke(const AiPkgRevoke &revoke) noexcept;

private:
    job::crypto::JobCryptoSign m_signer;
};

} // namespace job::aipkg