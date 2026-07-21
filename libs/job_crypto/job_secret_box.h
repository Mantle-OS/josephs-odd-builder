#pragma once

#include <vector>

#include "job_secure_mem.h"
#include "jobcrypto_export.h"
namespace job::crypto {

class JOBCRYPTO_EXPORT JobSecretBox {
public:
    JobSecretBox() = delete;
    ~JobSecretBox() = delete;

    [[nodiscard]] static bool encrypt(const std::vector<unsigned char> &plainText,
                                      const JobSecureMem &key,
                                      std::vector<unsigned char> &outCipherText,
                                      std::vector<unsigned char> &outNonce) noexcept;

    [[nodiscard]] static bool decrypt(const std::vector<unsigned char> &cipherText,
                                      const JobSecureMem &key,
                                      const std::vector<unsigned char> &nonce,
                                      JobSecureMem &outPlainText) noexcept;

    [[nodiscard]] static std::vector<unsigned char> generateNonce() noexcept;
};

} // namespace job::crypto