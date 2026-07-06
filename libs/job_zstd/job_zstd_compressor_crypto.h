// job_zstd_compressor_crypto.h
#pragma once

#include "job_zstd_compressor.h"
#include "job_secure_mem.h"

namespace job::zstd {

// Encrypted counterpart to JobZstdCompressor. The inner content is a
// completely ordinary, unmodified job_zstd archive --
// encryption is purely an outer envelope built on JobZstdEncryptingTransport. There is no
// "this is encrypted" tag anywhere in the format: calling this class at
// all is the caller's explicit declaration of intent.... Use the right tool for the JOB
class JobZstdCompressorCrypto : public JobZstdCompressor
{
public:
    JobZstdCompressorCrypto() = default;
    ~JobZstdCompressorCrypto() override = default;

    [[nodiscard]] const job::crypto::JobSecureMem &encryptionKey() const noexcept;
    void setEncryptionKey(const job::crypto::JobSecureMem &key);
    [[nodiscard]] bool hasKeys() const noexcept;

    [[nodiscard]] bool execute() override;
    [[nodiscard]] bool compressFolder() override;
    [[nodiscard]] bool compressFile() override;

private:
    job::crypto::JobSecureMem m_encryptionKey;

};

} // namespace job::zstd