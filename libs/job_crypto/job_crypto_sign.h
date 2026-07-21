#pragma once

#include <cstddef>
#include <fstream>
#include <memory>
#include <string>

#include "job_crypto_keys.h"
#include "job_crypto_utils.h"
#include "jobcrypto_export.h"
namespace job::crypto {
class JOBCRYPTO_EXPORT JobCryptoSign : public JobCryptoKeys
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

    [[nodiscard]] bool verifyFile(const std::string &filePath, const std::string &signatureBase64) noexcept;

    [[nodiscard]] bool verifyFile(const std::string &filePath,
                                  const std::vector<unsigned char> &signatureBytes) noexcept;

    [[nodiscard]] bool verifyAssociatedFile(const std::string &signatureBase64) noexcept;

    // NEW  Raw buffer cryptographics (No disk I/O)
    [[nodiscard]] bool signBuffer(const std::vector<uint8_t> &buffer, std::string &outSignatureBase64) noexcept;
    [[nodiscard]] bool signBuffer(const uint8_t *data, std::size_t size, std::string &outSignatureBase64) noexcept;

    [[nodiscard]] bool verifyBuffer(const std::vector<uint8_t> &buffer, const std::string &signatureBase64) noexcept;
    [[nodiscard]] bool verifyBuffer(const uint8_t *data, std::size_t size, const std::vector<uint8_t> &signatureBytes) noexcept;

private:
    File_Ptr        m_file{nullptr};
    std::string     m_associatedPath;

    static constexpr std::size_t kChunkSize = 65536;
};

} // namespace job::crypto