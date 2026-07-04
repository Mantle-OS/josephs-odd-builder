#pragma once
#include <job_crypto_sign.h>
#include "job_zstd_options.h"

namespace job::zstd {

class JobZstdSign : public JobZstdOptions
{
public:
    JobZstdSign() = default;
    ~JobZstdSign() override = default;

    [[nodiscard]] std::filesystem::path publicKeyFile() const noexcept;
    [[nodiscard]] bool setPublicKeyFile(const std::filesystem::path &publicKeyFile) noexcept;

    [[nodiscard]] std::filesystem::path privateKeyFile() const noexcept;
    [[nodiscard]] bool setPrivateKeyFile(const std::filesystem::path &privateKeyFile) noexcept;

    [[nodiscard]] bool hasSigningKeys() const noexcept;
    [[nodiscard]] bool hasVerificationKey() noexcept;

    [[nodiscard]] bool signFile(const std::filesystem::path &inPath, const std::filesystem::path &outPath, bool overwrite = true);
    [[nodiscard]] bool verifyFile(const std::filesystem::path &filePath, const std::string &signatureBase64) noexcept;

private:
    [[nodiscard]] bool setKeyPair(const std::filesystem::path &publicKeyFile,
                                  const std::filesystem::path &privateKeyFile) noexcept;

    job::crypto::JobCryptoSign  m_signer;
    std::filesystem::path       m_currentPubKeyFile;
    std::string                 m_pubData;
    std::filesystem::path       m_currentPriKeyFile;
};

} // namespace job::zstd