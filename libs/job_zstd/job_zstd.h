// job_zstd.h
#pragma once

#include <filesystem>
#include <future>

#include "job_zstd_options.h"
#include "job_zstd_compressor.h"
#include "job_zstd_decompressor.h"
#include "job_zstd_compressor_crypto.h"
#include "job_zstd_decompressor_crypto.h"
#include "job_zstd_sign.h"
#include "job_secure_mem.h"

namespace job::zstd {

class JobZstd : public JobZstdOptions
{
public:
    JobZstd();
    ~JobZstd() override;

    JobZstd(const JobZstd &) = delete;
    JobZstd &operator=(const JobZstd &) = delete;

    void compress();
    void decompress();

    void compress(bool sign, bool encrypt);
    void decompress(bool verify, bool decrypt);

    [[nodiscard]] std::filesystem::path publicKeyFile() const noexcept;
    [[nodiscard]] bool setPublicKeyFile(const std::filesystem::path &pubKey) noexcept;

    [[nodiscard]] std::filesystem::path privateKeyFile() const noexcept;
    [[nodiscard]] bool setPrivateKeyFile(const std::filesystem::path &privKey) noexcept;

    [[nodiscard]] job::crypto::JobSecureMem getPrivateKey() const noexcept;
    void setPrivateKey(const job::crypto::JobSecureMem &key) noexcept;

    [[nodiscard]] job::crypto::JobSecureMem getSignKey() const noexcept;
    void setSignKey(const job::crypto::JobSecureMem &key) noexcept;

    [[nodiscard]] bool isRunning() const noexcept;

private:
    void setupTaskConnections(JobZstdOptions *task, std::future<bool> *watcher);
    [[nodiscard]] static bool futureIsRunning(const std::future<bool> &f) noexcept;

private:
    JobZstdCompressor           *m_compress = nullptr; // We own this raw pointer.
    std::future<bool>           m_compressFuture;

    JobZstdDecompressor         *m_decompress = nullptr; // We own this raw pointer.
    std::future<bool>           m_decompressFuture;

    JobZstdCompressorCrypto     *m_compressCrypto = nullptr; // We own this raw pointer.
    std::future<bool>           m_compressCryptoFuture;

    JobZstdDecompressorCrypto   *m_decompressCrypto = nullptr; // We own this raw pointer.
    std::future<bool>           m_decompressCryptoFuture;

    JobZstdSign                 *m_signer = nullptr; // We own this raw pointer.

    std::filesystem::path       m_publicKeyFile;
    std::filesystem::path       m_privateKeyFile;
    job::crypto::JobSecureMem   m_privateKey;
    job::crypto::JobSecureMem   m_signKey;
};

} // namespace job::zstd