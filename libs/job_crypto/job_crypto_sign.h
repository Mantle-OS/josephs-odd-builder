#pragma once

#include <cstddef>
#include <fstream>
#include <memory>
#include <string>

#include "job_crypto_keys.h"
#include "job_crypto_utils.h"

namespace job::crypto {
class JobCryptoSign : public JobCryptoKeys
{
public:
    using Ptr       = std::shared_ptr<JobCryptoSign>;
    using File_Ptr  = std::shared_ptr<std::ifstream>;
    // the rest is the same numus the FILE_PTR
    explicit JobCryptoSign();
    ~JobCryptoSign() = default;

    JobCryptoSign(const JobCryptoSign &other) = default;
    JobCryptoSign &operator=(const JobCryptoSign &other) = default;
    JobCryptoSign(JobCryptoSign &&other) noexcept = default;
    JobCryptoSign &operator=(JobCryptoSign &&other) noexcept = default;

    [[nodiscard]] File_Ptr file() const noexcept;
    void setFile(File_Ptr newFile);

    [[nodiscard]] bool signFile(const std::string &filePath, std::string &outSignatureBase64) noexcept;
    [[nodiscard]] bool signAssociatedFile(std::string &outSignatureBase64) noexcept;
    [[nodiscard]] bool signAssociatedFile(const std::string &associatedPath, std::string &outSignatureBase64) noexcept;

    [[nodiscard]] bool verifyFile(const std::string &filePath, const std::string &signatureBase64) noexcept
    {
        std::vector<unsigned char> sigBin;
        if (!crypto::utils::base64ToBin(sigBin, signatureBase64))
            return false;
        return verifyFile(filePath, sigBin);
    }

    [[nodiscard]] bool verifyFile(const std::string &filePath,
                                  const std::vector<unsigned char> &signatureBytes) noexcept
    {
        if (signatureBytes.empty() || signatureBytes.size() != crypto_sign_BYTES)
            return false;

        std::vector<unsigned char> pubKeyBin;
        if (!crypto::utils::base64ToBin(pubKeyBin, publicKey()))
            return false;

        if (pubKeyBin.size() != crypto_sign_PUBLICKEYBYTES)
            return false;

        std::ifstream stream(filePath, std::ios::binary);
        if (!stream.is_open())
            return false;

        crypto_sign_state state;
        crypto_sign_init(&state);

        std::vector<char> buffer(kChunkSize);
        while (stream.read(buffer.data(), kChunkSize) || stream.gcount() > 0) {
            crypto_sign_update(&state,
                               reinterpret_cast<const unsigned char*>(buffer.data()),
                               stream.gcount());
        }

        int const result = crypto_sign_final_verify(&state,
                                                    signatureBytes.data(),
                                                    pubKeyBin.data());
        return (result == 0);
    }

    [[nodiscard]] bool verifyAssociatedFile(const std::string &signatureBase64) noexcept;

private:
    File_Ptr        m_file{nullptr};
    std::string     m_associatedPath;

    static constexpr std::size_t kChunkSize = 65536;
};

} // namespace job::crypto