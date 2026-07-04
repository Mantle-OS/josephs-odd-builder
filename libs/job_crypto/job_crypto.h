#pragma once

#include <string>
#include <vector>

#include "job_secure_mem.h"

namespace job::crypto {

class JobCrypto
{
public:
    JobCrypto() noexcept;
    ~JobCrypto() = default;

    [[nodiscard]] bool isInitialized() const noexcept;

    // [[nodiscard]] bool verifyFileSignature(const std::string &filePath,
    //                                        const std::vector<unsigned char> &publicKeyBytes,
    //                                        const std::vector<unsigned char> &signatureBytes) noexcept;

    [[nodiscard]] std::string computeFileBlake2bHex(const std::string &filePath) noexcept;

    [[nodiscard]] static bool encryptConfig(const std::vector<unsigned char> &plainText,
                                            const JobSecureMem &key,
                                            std::vector<unsigned char> &outCipherText,
                                            std::vector<unsigned char> &outNonce) noexcept;

    [[nodiscard]] static bool decryptConfig(const std::vector<unsigned char> &cipherText,
                                            const JobSecureMem &key,
                                            const std::vector<unsigned char> &nonce,
                                            JobSecureMem &outPlainText) noexcept;

private:
    bool m_initSuccess{false};
};

} // namespace job::crypto