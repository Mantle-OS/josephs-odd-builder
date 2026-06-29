#pragma once

#include <QObject>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

#include "qzstdcompressor.h"
#include "qzstddecompressor.h"
#include "qzstdoptions.h"

#ifdef QZSTD_SODIUM_SUPPORT
#include "qzstdcompressorcrypto.h"
#include "qzstddecompressorcrypto.h"
#include "qzstdsign.h"
#endif


class QZstd : public QZstdOptions
{
    Q_OBJECT

#ifdef QZSTD_SODIUM_SUPPORT
    Q_PROPERTY(QString publicKey READ publicKey WRITE setPublicKey NOTIFY publicKeyChanged FINAL)
    Q_PROPERTY(QString privateKey READ privateKey WRITE setPrivateKey NOTIFY privateKeyChanged FINAL)
    Q_PROPERTY(QString signatureKey READ signatureKey WRITE setSignatureKey NOTIFY signatureKeyChanged FINAL)
#endif

public:
    explicit QZstd(QObject *parent = nullptr);
    ~QZstd() override;

    Q_INVOKABLE void decompress();
    Q_INVOKABLE void compress();

#ifdef QZSTD_SODIUM_SUPPORT
    Q_INVOKABLE void decompress(bool verify, bool decrypt);
    Q_INVOKABLE void compress(bool sign, bool encrypt);

    QString publicKey() const;
    void setPublicKey(const QString &pubKey);

    QString privateKey() const;
    void setPrivateKey(const QString &privKey);
    QString signatureKey() const;
    void setSignatureKey(const QString &sigKey);

Q_SIGNALS:
    void signatureKeyChanged();
    void publicKeyChanged();
    void privateKeyChanged();
#endif

private:
    void setupTaskConnections(QZstdOptions* task, QFutureWatcher<bool>* watcher);

#ifdef QZSTD_SODIUM_SUPPORT
    QSecureMem getSecurePrivateKey() const;
    QSecureMem getSecureEncryptionKey() const; // Maps password/raw bytes context safely
#endif

private:
    QZstdCompressor *m_compress = nullptr;
    QFutureWatcher<bool> m_compressWatcher;

    QZstdDecompressor *m_decompress = nullptr;
    QFutureWatcher<bool> m_decompressWatcher;

#ifdef QZSTD_SODIUM_SUPPORT
    QZstdCompressorCrypto *m_compressCrypto = nullptr;
    QFutureWatcher<bool> m_compressCryptoWatcher;

    QZstdDecompressorCrypto *m_decompressCrypto = nullptr;
    QFutureWatcher<bool> m_decompressCryptoWatcher;

    QZstdSign *m_signer = nullptr;
    QFutureWatcher<bool> m_signerWatcher;

    QString m_publicKey;
    QString m_privateKey;
    QString m_signatureKey;
#endif
};
