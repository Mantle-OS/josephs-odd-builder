#include "job_crypto.h"

#include <iomanip>
#include <sstream>

#include <job_logger.h>

#include "job_crypto_init.h"
#include "job_hash.h"
#include "job_secret_box.h"


namespace job::crypto {

JobCrypto::JobCrypto() noexcept :
    m_initSuccess(false)
{
    if (JobCryptoInit::isInitialized()) {
        m_initSuccess = true;
    } else {
        m_initSuccess = JobCryptoInit::initialize();

        if (!m_initSuccess) {
            JOB_LOG_ERROR(
                "[JobCrypto] CRITICAL: Crypto runtime initialization failed! "
                "Primitives are locked."
                );
        }
    }
}
bool JobCrypto::isInitialized() const noexcept
{
    return m_initSuccess;
}

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