// job_aipkg_sign.cpp
#include "job_aipkg_sign.h"

#include <job_crypto_utils.h>
#include <algorithm>

namespace job::aipkg {

namespace {

// Shared core template for record signing mutations. Writes the signature
// AND the signer's own pubkey into the record -- see the class-level note
// on why those two are deliberately coupled here.
template <typename T, typename SigField, typename PubField>
[[nodiscard]] bool signRecord(job::crypto::JobCryptoSign &signer, T &record,
                              SigField T::*sigMember, PubField T::*pubMember,
                              const std::vector<uint8_t> &payload) noexcept
{
    std::string sigBase64;
    if (!signer.signBuffer(payload, sigBase64))
        return false;

    std::vector<unsigned char> sigBin;
    if (!job::crypto::utils::base64ToBin(sigBin, sigBase64))
        return false;

    if (sigBin.size() != (record.*sigMember).size())
        return false;

    std::copy(sigBin.begin(), sigBin.end(), (record.*sigMember).begin());

    std::vector<unsigned char> pubBin;
    if (!job::crypto::utils::base64ToBin(pubBin, signer.publicKey()))
        return false;

    if (pubBin.size() != (record.*pubMember).size())
        return false;

    std::copy(pubBin.begin(), pubBin.end(), (record.*pubMember).begin());
    return true;
}

// PERFORMANCE NOTE: This stateless design allocates a fresh engine and
// decodes the embedded base64 key from scratch on every single call. If
// higher-level synchronization loops ever need to bulk-verify thousands of
// sequential records, this should be refactored to support a stateful,
// pre-cached validation context to eliminate key decoding overhead. Known,
// deliberate trade-off -- not a premature optimization target today.
// Shared core template for stateless record verification
template <typename T, typename SigField, typename PubField>
[[nodiscard]] bool verifyRecord(const T &record, SigField T::*sigMember, PubField T::*pubMember,
                                const std::vector<uint8_t> &payload) noexcept
{
    job::crypto::JobCryptoSign verifier;

    // Allocate fixed size and copy to bypass GCC 15 range-ctor LTO bug
    std::vector<unsigned char> pubBytes((record.*pubMember).size());
    std::copy((record.*pubMember).begin(), (record.*pubMember).end(), pubBytes.begin());

    std::string const pubBase64 = job::crypto::utils::toBase64(pubBytes);
    verifier.setPublicKey(pubBase64);

    // Same treatment here: size-allocation + explicit copy
    std::vector<uint8_t> sigBytes((record.*sigMember).size());
    std::copy((record.*sigMember).begin(), (record.*sigMember).end(), sigBytes.begin());

    return verifier.verifyBuffer(payload.data(), payload.size(), sigBytes);
}

} // namespace

bool JobAiPkgSign::loadKeyPair(const std::filesystem::path &publicKeyFile,
                               const std::filesystem::path &privateKeyFile) noexcept
{
    return m_signer.loadKeysFromDisk(publicKeyFile, privateKeyFile);
}

bool JobAiPkgSign::hasSigningKeys() const noexcept
{
    return m_signer.isValid();
}

bool JobAiPkgSign::signBlock(AiPkgBlock &block) noexcept
{
    if (!hasSigningKeys())
        return false;
    return signRecord(m_signer, block, &AiPkgBlock::sig, &AiPkgBlock::mint_pub,
                      encodeUnsignedPayload(block, &AiPkgBlock::sig, &AiPkgBlock::mint_pub));
}

bool JobAiPkgSign::signAttestation(AiPkgAttestation &attestation) noexcept
{
    if (!hasSigningKeys())
        return false;
    return signRecord(m_signer, attestation, &AiPkgAttestation::signature, &AiPkgAttestation::signer_pub,
                      encodeUnsignedPayload(attestation, &AiPkgAttestation::signature, &AiPkgAttestation::signer_pub));
}

bool JobAiPkgSign::signDelegate(AiPkgDelegate &delegate) noexcept
{
    if (!hasSigningKeys())
        return false;
    return signRecord(m_signer, delegate, &AiPkgDelegate::sig_by_dev, &AiPkgDelegate::dev_pub,
                      encodeUnsignedPayload(delegate, &AiPkgDelegate::sig_by_dev, &AiPkgDelegate::dev_pub));
}

bool JobAiPkgSign::signTx(AiPkgTx &tx) noexcept
{
    if (!hasSigningKeys())
        return false;
    return signRecord(m_signer, tx, &AiPkgTx::sig_from, &AiPkgTx::from_pub,
                      encodeUnsignedPayload(tx, &AiPkgTx::sig_from, &AiPkgTx::from_pub));
}

bool JobAiPkgSign::verifyBlock(const AiPkgBlock &block) noexcept
{
    return verifyRecord(block, &AiPkgBlock::sig, &AiPkgBlock::mint_pub,
                        encodeUnsignedPayload(block, &AiPkgBlock::sig, &AiPkgBlock::mint_pub));
}

bool JobAiPkgSign::verifyAttestation(const AiPkgAttestation &attestation) noexcept
{
    return verifyRecord(attestation, &AiPkgAttestation::signature, &AiPkgAttestation::signer_pub,
                        encodeUnsignedPayload(attestation, &AiPkgAttestation::signature, &AiPkgAttestation::signer_pub));
}

bool JobAiPkgSign::verifyDelegate(const AiPkgDelegate &delegate) noexcept
{
    return verifyRecord(delegate, &AiPkgDelegate::sig_by_dev, &AiPkgDelegate::dev_pub,
                        encodeUnsignedPayload(delegate, &AiPkgDelegate::sig_by_dev, &AiPkgDelegate::dev_pub));
}

bool JobAiPkgSign::verifyTx(const AiPkgTx &tx) noexcept
{
    return verifyRecord(tx, &AiPkgTx::sig_from, &AiPkgTx::from_pub,
                        encodeUnsignedPayload(tx, &AiPkgTx::sig_from, &AiPkgTx::from_pub));
}

bool JobAiPkgSign::signRevoke(AiPkgRevoke &revoke) noexcept
{
    if (!hasSigningKeys())
        return false;
    return signRecord(m_signer, revoke, &AiPkgRevoke::signature, &AiPkgRevoke::signer_pub,
                      encodeUnsignedPayload(revoke, &AiPkgRevoke::signature, &AiPkgRevoke::signer_pub));
}

bool JobAiPkgSign::verifyRevoke(const AiPkgRevoke &revoke) noexcept
{
    return verifyRecord(revoke, &AiPkgRevoke::signature, &AiPkgRevoke::signer_pub,
                        encodeUnsignedPayload(revoke, &AiPkgRevoke::signature, &AiPkgRevoke::signer_pub));
}


} // namespace job::aipkg