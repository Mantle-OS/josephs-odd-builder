#pragma once

#include "qzstdoptions.h"
#include <job_zstd_compressor_crypto.h>
#include <qsecuremem.h>

#include <QObject>
#include <QString>

class QZstdCompressorCrypto : public job::zstd::JobZstdCompressorCrypto
{
public:
    explicit QZstdCompressorCrypto();
    QZstdCompressorCrypto(QZstdOptions *opts);
    ~QZstdCompressorCrypto();
    QZstdCompressorCrypto(const QZstdCompressorCrypto &) = delete;
    QZstdCompressorCrypto(QZstdCompressorCrypto &&) noexcept = delete;
    QZstdCompressorCrypto &operator=(const QZstdCompressorCrypto &) = delete;
    QZstdCompressorCrypto &operator=(QZstdCompressorCrypto &&) noexcept = delete;

    [[nodiscard]] const QSecureMem &encryptionKey() const noexcept;
    void setEncryptionKey(const QSecureMem &key) noexcept;

    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);

    [[nodiscard]] bool compressAndEncrypt() noexcept;

private:
    // func
    void disconnectOptionConnections() noexcept;
    void setupOptionConnections() noexcept;
    //  mem
    QZstdOptions *m_opts = nullptr;
    bool m_ownsOpts = true;
    QSecureMem m_encryptionKey;
};
