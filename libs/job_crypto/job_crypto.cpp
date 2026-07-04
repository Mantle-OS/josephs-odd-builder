#include "job_crypto.h"
#include "job_crypto_init.h"
#include "job_crypto_sign.h"
#include "job_hash.h"
#include "job_secret_box.h"

#include <iostream>
#include <iomanip>
#include <sstream>

namespace job::crypto {

JobCrypto::JobCrypto() noexcept
    : m_initSuccess(false)
{
    if (JobCryptoInit::isInitialized()) {
        m_initSuccess = true;
    } else {
        m_initSuccess = JobCryptoInit::initialize();
#ifndef NDEBUG
        if (!m_initSuccess) {
            std::cerr << "[JobCrypto] CRITICAL: Hardware initialization failed! Primitives are locked.\n";
        }
#endif
    }
}

bool JobCrypto::isInitialized() const noexcept
{
    return m_initSuccess;
}

// bool JobCrypto::verifyFileSignature(const std::string &filePath,
//                                     const std::vector<unsigned char> &publicKeyBytes,
//                                     const std::vector<unsigned char> &signatureBytes) noexcept
// {
//     if (!m_initSuccess)
//         return false;
//     JobCryptoSign verifier;
//     if (!verifier.setPublicKey(publicKeyBytes))
//         return false;

//     return verifier.verifyFile(filePath, signatureBytes);
// }

std::string JobCrypto::computeFileBlake2bHex(const std::string &filePath) noexcept
{
    if (!m_initSuccess)
        return {};

    std::vector<unsigned char> const binaryHash = JobHash::hashFile(filePath);
    if (binaryHash.empty())
        return {};


    // Convert raw digest to standard lowercase Hex string representation
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char const b : binaryHash)
        ss << std::setw(2) << static_cast<int>(b);

    return ss.str();
}

bool JobCrypto::encryptConfig(const std::vector<unsigned char> &plainText,
                              const JobSecureMem &key,
                              std::vector<unsigned char> &outCipherText,
                              std::vector<unsigned char> &outNonce) noexcept
{
    return JobSecretBox::encrypt(plainText, key, outCipherText, outNonce);
}

bool JobCrypto::decryptConfig(const std::vector<unsigned char> &cipherText,
                              const JobSecureMem &key,
                              const std::vector<unsigned char> &nonce,
                              JobSecureMem &outPlainText) noexcept
{
    return JobSecretBox::decrypt(cipherText, key, nonce, outPlainText);
}

} // namespace job::crypto