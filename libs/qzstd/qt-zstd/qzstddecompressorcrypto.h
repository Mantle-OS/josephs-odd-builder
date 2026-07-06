#pragma once
#include <qsecuremem.h>

#include <job_zstd_decompressor_crypto.h>

#include "qzstdoptions.h"

class QZstdDecompressorCrypto : public  job::zstd::JobZstdDecompressorCrypto
{
public:
    explicit QZstdDecompressorCrypto();
    QZstdDecompressorCrypto(QZstdOptions *opts);
    ~QZstdDecompressorCrypto();
    QZstdDecompressorCrypto(const QZstdDecompressorCrypto &) = delete;
    QZstdDecompressorCrypto(QZstdDecompressorCrypto &&) noexcept = delete;
    QZstdDecompressorCrypto &operator=(const QZstdDecompressorCrypto &) = delete;
    QZstdDecompressorCrypto &operator=(QZstdDecompressorCrypto &&) noexcept = delete;

    [[nodiscard]] const QSecureMem &decryptionKey() const noexcept;
    void setDecryptionKey(const QSecureMem &key) noexcept;

    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);

    [[nodiscard]] bool decryptAndDecompress() noexcept;

private:
    void disconnectOptionConnections() noexcept;
    void setupOptionConnections() noexcept;
    QZstdOptions    *m_opts         = nullptr;
    bool            m_ownsOpts      = true;
    QSecureMem      m_decryptionKey;
};

