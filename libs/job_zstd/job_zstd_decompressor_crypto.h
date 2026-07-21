#pragma once

#include <job_secure_mem.h>

#include "job_zstd_io.h"
#include "job_zstd_decompressor.h"
#include "job_zstd_decrypting_transport.h"

#include "jobzstd_export.h"
namespace job::zstd {

class JOBZSTD_EXPORT JobZstdDecompressorCrypto : public JobZstdDecompressor
{
public:
    JobZstdDecompressorCrypto() = default;
    ~JobZstdDecompressorCrypto() override = default;

    [[nodiscard]] const job::crypto::JobSecureMem &decryptionKey() const noexcept;
    void setDecryptionKey(const job::crypto::JobSecureMem &key);
    [[nodiscard]] bool hasKeys() const noexcept;
    [[nodiscard]] bool execute() override;
    [[nodiscard]] bool decompressFolder() override;
    [[nodiscard]] bool decompressFile() override;
    [[nodiscard]] bool decompressSymlinkArchive() override;
private:
    job::crypto::JobSecureMem m_decryptionKey;
    [[nodiscard]] static std::string bestErrorMessage(const JobZstdIO &zstd, const JobZstdDecryptingTransport &decTransport, const std::string &fallback);
};

} // namespace job::zstd