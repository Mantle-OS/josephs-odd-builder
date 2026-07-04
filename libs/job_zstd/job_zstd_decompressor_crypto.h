#pragma once

#include <job_secure_mem.h>

#include "job_zstd_decompressor.h"

namespace job::zstd {

class JobZstdDecompressorCrypto : public JobZstdDecompressor
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
};

} // namespace job::zstd