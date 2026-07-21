#include "job_zstd_sign.h"

#include <fstream>
#include <algorithm>
#include <limits>

#include <sodium/crypto_sign.h>

#include <job_secure_mem.h>

namespace job::zstd {


bool JobZstdSign::setKeyPair(const std::filesystem::path &publicKeyFile,
                             const std::filesystem::path &privateKeyFile) noexcept
{
    return m_signer.loadKeysFromDisk(publicKeyFile, privateKeyFile);
}

std::filesystem::path JobZstdSign::publicKeyFile() const
{
    return m_currentPubKeyFile;
}

bool JobZstdSign::setPublicKeyFile(const std::filesystem::path &publicKeyFile)
{
    if (!m_signer.validPublicKey(publicKeyFile))
        return false;

    std::string const data = m_signer.publicKeyData(publicKeyFile);
    if (data.empty())
        return false;

    m_pubData = data;
    m_signer.setPublicKey(m_pubData);
    m_currentPubKeyFile = publicKeyFile;
    return true;
}
std::filesystem::path JobZstdSign::privateKeyFile() const
{
    return m_currentPriKeyFile;
}
bool JobZstdSign::setPrivateKeyFile(const std::filesystem::path &privateKeyFile)
{
    if (m_currentPubKeyFile.empty())
        return false;

    if (!setKeyPair(m_currentPubKeyFile, privateKeyFile))
        return false;

    m_signer.setPublicKey(m_pubData);

    if (!m_signer.privateKeyMatchesPublicKey(job::crypto::JobCryptoKeys::KeyType::Sign)) {
        m_signer.setPrivateKey(job::crypto::JobSecureMem());
        return false;
    }

    m_currentPriKeyFile = privateKeyFile;
    return true;
}

bool JobZstdSign::hasSigningKeys() const noexcept
{
    return m_signer.isValid();
}

bool JobZstdSign::hasVerificationKey() noexcept
{
    return !m_pubData.empty();
}

bool JobZstdSign::signFile(const std::filesystem::path &inPath, const std::filesystem::path &outPath, bool overwrite)
{
    if (!hasSigningKeys()) {
        setErrorString("Signing pipeline missing a valid public/private key pair -- call setKeyPair() with loadPrivate=true first.");
        return false;
    }

    std::error_code inExistsEc;
    if (!std::filesystem::exists(inPath, inExistsEc)) {
        setErrorString("Target data file does not exist: " + inPath.string());
        return false;
    }

    if (!overwrite) {
        std::error_code outExistsEc;
        if (std::filesystem::exists(outPath, outExistsEc)) {
            setErrorString("Signature output file already exists and overwrite is false: " + outPath.string());
            return false;
        }
    }

    std::error_code sizeEc;
    std::uint64_t const fileSize = std::filesystem::file_size(inPath, sizeEc);
    setTotal(static_cast<int>(std::min<std::uint64_t>(fileSize, static_cast<std::uint64_t>(std::numeric_limits<int>::max()))));
    setCurrent(0);

    std::string signatureBase64;
    if (!m_signer.signFile(inPath.string(), signatureBase64)) {
        setErrorString("Crypto signing runtime failure. Ensure private key parameters are locked in.");
        return false;
    }

    std::ofstream sigFile(outPath, std::ios::binary | std::ios::trunc);
    if (!sigFile) {
        setErrorString("Failed to open signature output file for writing: " + outPath.string());
        return false;
    }

    sigFile.write(signatureBase64.data(), static_cast<std::streamsize>(signatureBase64.size()));
    if (!sigFile) {
        setErrorString("Failed to write signature to output file: " + outPath.string());
        return false;
    }

    setCurrent(total());
    return true;
}

bool JobZstdSign::verifyFile(const std::filesystem::path &filePath, const std::string &signatureBase64)
{
    return m_signer.verifyFile(filePath.string(), signatureBase64);
}

} // namespace job::zstd