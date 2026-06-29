#include "qzstd.h"
#include <sodium.h>
#include <QFile>

QZstd::QZstd(QObject *parent)
    : QZstdOptions{parent}
    , m_compress{new QZstdCompressor{this}}
    , m_decompress{new QZstdDecompressor{this}}
{
    setupTaskConnections(m_compress, &m_compressWatcher);
    setupTaskConnections(m_decompress, &m_decompressWatcher);

#ifdef QZSTD_SODIUM_SUPPORT
    m_compressCrypto = new QZstdCompressorCrypto{this};
    m_decompressCrypto = new QZstdDecompressorCrypto{this};
    m_signer = new QZstdSign{this};

    setupTaskConnections(m_compressCrypto, &m_compressCryptoWatcher);
    setupTaskConnections(m_decompressCrypto, &m_decompressCryptoWatcher);
    setupTaskConnections(m_signer, &m_signerWatcher);
#endif
}

QZstd::~QZstd()
{
    m_compressWatcher.cancel();
    m_decompressWatcher.cancel();
    m_compressWatcher.waitForFinished();
    m_decompressWatcher.waitForFinished();

#ifdef QZSTD_SODIUM_SUPPORT
    m_compressCryptoWatcher.cancel();
    m_decompressCryptoWatcher.cancel();
    m_signerWatcher.cancel();
    m_compressCryptoWatcher.waitForFinished();
    m_decompressCryptoWatcher.waitForFinished();
    m_signerWatcher.waitForFinished();
#endif
}

void QZstd::setupTaskConnections(QZstdOptions* task, QFutureWatcher<bool>* watcher)
{
    connect(task, &QZstdOptions::currentChanged, this, [this, task]() {
        setCurrent(task->current());
    });
    connect(task, &QZstdOptions::totalChanged, this, [this, task]() {
        setTotal(task->total());
    });
    connect(task, &QZstdOptions::errorStringChanged, this, [this, task]() {
        setErrorString(task->errorString());
    });
    // connect(task, &QZstdOptions::finished, this, &QZstd::finished);

    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher]() {
        if (!watcher->result() && errorString().isEmpty()) {
            setErrorString(QStringLiteral("Background pipeline task processing failed."));
        }
        Q_EMIT finished(); // Fail out cleanly
    });
}

void QZstd::decompress()
{
    if (m_decompressWatcher.isRunning()) return;
    setErrorString(QString());
    setCurrent(0); setTotal(0);

    m_decompress->setInput(input());
    m_decompress->setOutput(output());

    m_decompressWatcher.setFuture(QtConcurrent::run(&QZstdDecompressor::execute, m_decompress));
}

void QZstd::compress()
{
    if (m_compressWatcher.isRunning()) return;
    setErrorString(QString());
    setCurrent(0); setTotal(0);

    m_compress->setInput(input());
    m_compress->setOutput(output());
    m_compress->setCompressionLevel(compressionLevel());

    m_compressWatcher.setFuture(QtConcurrent::run(&QZstdCompressor::execute, m_compress));
}

#ifdef QZSTD_SODIUM_SUPPORT
QString QZstd::publicKey() const { return m_publicKey; }
void QZstd::setPublicKey(const QString &pubKey)
{
    if (m_publicKey != pubKey) {
        m_publicKey = pubKey;
        Q_EMIT publicKeyChanged();
    }
}

QString QZstd::privateKey() const { return m_privateKey; }
void QZstd::setPrivateKey(const QString &privKey)
{
    if (m_privateKey != privKey) {
        m_privateKey = privKey;
        Q_EMIT privateKeyChanged();
    }
}

QString QZstd::signatureKey() const
{
    return m_signatureKey;
}

void QZstd::setSignatureKey(const QString &sigKey)
{
    if (m_signatureKey != sigKey) {
        m_signatureKey = sigKey; Q_EMIT signatureKeyChanged();
    }
}

QSecureMem QZstd::getSecurePrivateKey() const
{
    QSecureMem secureKey;
    secureKey.fromBase64(m_privateKey);
    return secureKey;
}

QSecureMem QZstd::getSecureEncryptionKey() const
{
    QSecureMem secureKey;
    secureKey.fromBase64(m_privateKey);
    return secureKey;
}

void QZstd::compress(bool sign, bool encrypt)
{
    setErrorString(QString());
    setCurrent(0); setTotal(0);

    // Isolate coordinator flags completely
    disconnect(m_compress, &QZstdOptions::finished, this, &QZstd::finished);
    disconnect(m_compressCrypto, &QZstdOptions::finished, this, &QZstd::finished);
    disconnect(m_signer, &QZstdOptions::finished, this, &QZstd::finished);

    QString const finalDestination = output();

    if (encrypt) {
        if (m_compressCryptoWatcher.isRunning())
            return;

        m_compressCrypto->setInput(input());
        m_compressCrypto->setCompressionLevel(compressionLevel());
        m_compressCrypto->setOutput(sign ? (finalDestination + QStringLiteral(".tmp")) : finalDestination);
        m_compressCrypto->setEncryptionKeyB64(privateKey());

        // Handle the transition when the compression watcher finishes
        disconnect(&m_compressCryptoWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
        connect(&m_compressCryptoWatcher, &QFutureWatcher<bool>::finished, this, [this, sign, finalDestination]() {
            if (!m_compressCryptoWatcher.result()) {
                if (errorString().isEmpty()) setErrorString(m_compressCrypto->errorString());
                Q_EMIT finished();
                return;
            }

            if (sign) {
                m_signer->setInput(m_compressCrypto->output()); // secure_package.pkg.tmp
                m_signer->setOutput(finalDestination + QStringLiteral(".sig"));
                m_signer->setSigningKeyPairB64(publicKey(), signatureKey());

                disconnect(&m_signerWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
                connect(&m_signerWatcher, &QFutureWatcher<bool>::finished, this, [this, finalDestination]() {
                    if (m_signerWatcher.result()) {
                        QFile::remove(finalDestination);
                        if (QFile::rename(finalDestination + QStringLiteral(".tmp"), finalDestination)) {
                            Q_EMIT finished(); // Hand over final loop completion token safely!
                        } else {
                            setErrorString(QStringLiteral("Pipeline failed to promote signed asset package."));
                            Q_EMIT finished();
                        }
                    } else {
                        if (errorString().isEmpty()) setErrorString(m_signer->errorString());
                        Q_EMIT finished();
                    }
                });
                m_signerWatcher.setFuture(QtConcurrent::run(&QZstdSign::sign, m_signer));
            } else {
                Q_EMIT finished();
            }
        });
        m_compressCryptoWatcher.setFuture(QtConcurrent::run(&QZstdCompressorCrypto::execute, m_compressCrypto));
    }  else {
        if (m_compressWatcher.isRunning())
            return;
        m_compress->setInput(input());
        m_compress->setCompressionLevel(compressionLevel());
        m_compress->setOutput(sign ? (finalDestination + QStringLiteral(".tmp")) : finalDestination);

        disconnect(&m_compressWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
        connect(&m_compressWatcher, &QFutureWatcher<bool>::finished, this, [this, sign, finalDestination]() {
            if (!m_compressWatcher.result()) {
                if (errorString().isEmpty()) setErrorString(m_compress->errorString());
                Q_EMIT finished();
                return;
            }
            // This was a PITA ....
            if (sign) {
                m_signer->setInput(m_compress->output());
                m_signer->setOutput(finalDestination + QStringLiteral(".sig"));
                m_signer->setSigningKeyPairB64(publicKey(), signatureKey());

                disconnect(&m_signerWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
                connect(&m_signerWatcher, &QFutureWatcher<bool>::finished, this, [this, finalDestination]() {
                    if (m_signerWatcher.result()) {
                        QFile::remove(finalDestination);
                        if (QFile::rename(finalDestination + QStringLiteral(".tmp"), finalDestination)) {
                            Q_EMIT finished();
                        } else {
                            setErrorString(QStringLiteral("Pipeline failed to promote signed asset package."));
                            Q_EMIT finished();
                        }
                    } else {
                        if (errorString().isEmpty()) setErrorString(m_signer->errorString());
                        Q_EMIT finished();
                    }
                });
                m_signerWatcher.setFuture(QtConcurrent::run(&QZstdSign::sign, m_signer));
            } else {
                Q_EMIT finished();
            }
        });
        m_compressWatcher.setFuture(QtConcurrent::run(&QZstdCompressor::execute, m_compress));
    }
}

void QZstd::decompress(bool verify, bool decrypt)
{
    setErrorString(QString());
    setCurrent(0); setTotal(0);

    disconnect(m_compress, &QZstdOptions::finished, this, &QZstd::finished);
    disconnect(m_decompress, &QZstdOptions::finished, this, &QZstd::finished);

    disconnect(m_compressCrypto, &QZstdOptions::finished, this, &QZstd::finished);
    disconnect(m_decompressCrypto, &QZstdOptions::finished, this, &QZstd::finished);
    disconnect(m_signer, &QZstdOptions::finished, this, &QZstd::finished);


    auto proceedToExtraction = [this, decrypt]() {
        if (decrypt) {
            if (m_decompressCryptoWatcher.isRunning())
                return;
            m_decompressCrypto->setInput(input());
            m_decompressCrypto->setOutput(output());
            m_decompressCrypto->setDecryptionKeyB64(privateKey());

            // Bind explicitly to the WATCHER completion, not the inner task signal
            disconnect(&m_decompressCryptoWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
            connect(&m_decompressCryptoWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
                if (m_decompressCryptoWatcher.result()) {
                    Q_EMIT finished(); // Hand over final loop victory token here!
                } else {
                    if (errorString().isEmpty()) setErrorString(m_decompressCrypto->errorString());
                    Q_EMIT finished();
                }
            });

            m_decompressCryptoWatcher.setFuture(QtConcurrent::run(&QZstdDecompressorCrypto::execute, m_decompressCrypto));
        } else {
            if (m_decompressWatcher.isRunning())
                return;
            m_decompress->setInput(input());
            m_decompress->setOutput(output());

            disconnect(&m_decompressWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
            connect(&m_decompressWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
                if (m_decompressWatcher.result()) {
                    Q_EMIT finished();
                } else {
                    if (errorString().isEmpty()) setErrorString(m_decompress->errorString());
                    Q_EMIT finished();
                }
            });

            m_decompressWatcher.setFuture(QtConcurrent::run(&QZstdDecompressor::execute, m_decompress));
        }
    };
    if (verify) {
        if (m_signerWatcher.isRunning())
            return;

        m_signer->setInput(input() + QStringLiteral(".sig"));
        m_signer->setVerificationKey(publicKey());

        // Leftover debugging
        qDebug() << "[?] Inspecting disk layout targets:";
        qDebug() << "    - Package Exists?   " << QFile::exists(input());
        qDebug() << "    - Package Tmp?      " << QFile::exists(input() + ".tmp");
        qDebug() << "    - Signature Exists? " << QFile::exists(input() + ".sig");

        disconnect(&m_signerWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);
        connect(&m_signerWatcher, &QFutureWatcher<bool>::finished, this, [this, proceedToExtraction]() {
            if (m_signerWatcher.result()) {
                proceedToExtraction(); // Hands off cleanly to decompression stage
            } else {
                setErrorString(m_signer->errorString().isEmpty()
                               ? QStringLiteral("Signature verification rejected the asset.")
                               : m_signer->errorString());
                Q_EMIT finished();
            }
        });
        m_signerWatcher.setFuture(QtConcurrent::run(&QZstdSign::verify, m_signer));
    } else {
        proceedToExtraction();
    }
}
#endif