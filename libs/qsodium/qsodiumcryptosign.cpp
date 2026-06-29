#include "qsodiumcryptosign.h"
#include <sodium.h>
#include <QByteArray>
#include <QDebug>

QSodiumCryptoSign::QSodiumCryptoSign() :
    QSodiumKeys()
    , m_file(nullptr)
{
}

std::shared_ptr<QFile> QSodiumCryptoSign::file() const noexcept
{
    return m_file;
}

void QSodiumCryptoSign::setFile(std::shared_ptr<QFile> newFile)
{
    if (m_file == newFile)
        return;
    if (m_file && m_file.use_count() == 1 && m_file->isOpen()) {
        m_file->close();
    }
    m_file = std::move(newFile);
}

bool QSodiumCryptoSign::signFile(const QString &filePath, QString &outSignatureBase64) noexcept
{
    QFile transientFile(filePath);
    if (!transientFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[QSodiumCryptoSign] Cannot open targeted file for signing:" << filePath;
        return false;
    }

    if (!isValid() || privateKey().size() != crypto_sign_SECRETKEYBYTES) {
        qWarning() << "[QSodiumCryptoSign] Invalid key state or wrong key type for signature operations.";
        return false;
    }

    crypto_sign_state state;
    crypto_sign_init(&state);

    QByteArray buffer(m_chunkSize, '\0');
    while (!transientFile.atEnd()) {
        qint64 bytesRead = transientFile.read(buffer.data(), m_chunkSize);
        if (bytesRead < 0) {
            qWarning() << "[QSodiumCryptoSign] Read error encountered during file processing pass.";
            return false;
        }
        crypto_sign_update(&state, reinterpret_cast<const unsigned char*>(buffer.constData()), bytesRead);
    }

    QByteArray sigBin(crypto_sign_BYTES, '\0');
    unsigned long long sigLen = 0;

    if (crypto_sign_final_create(&state, reinterpret_cast<unsigned char*>(sigBin.data()), &sigLen, privateKey().data()) != 0) {
        qWarning() << "[QSodiumCryptoSign] Cryptographic signature finalization step failed.";
        return false;
    }

    outSignatureBase64 = QString::fromLatin1(sigBin.toBase64());
    return true;
}

bool QSodiumCryptoSign::signAssociatedFile(QString &outSignatureBase64) noexcept
{
    if (!m_file) {
        qWarning() << "[QSodiumCryptoSign] Error: No associated internal QFile pointer found.";
        return false;
    }

    bool wasOpen = m_file->isOpen();
    if (!wasOpen && !m_file->open(QIODevice::ReadOnly)) {
        qWarning() << "[QSodiumCryptoSign] Associated file could not be queried.";
        return false;
    } else if (wasOpen && !m_file->isReadable()) {
        qWarning() << "[QSodiumCryptoSign] Associated file isn't marked as readable.";
        return false;
    }

    qint64 originalPosition = m_file->pos();
    m_file->seek(0);

    bool result = signFile(m_file->fileName(), outSignatureBase64);

    if (wasOpen) {
        m_file->seek(originalPosition);
    } else {
        m_file->close();
    }

    return result;
}

bool QSodiumCryptoSign::verifyFile(const QString &filePath, const QString &signatureBase64) noexcept
{
    QByteArray const sigBin = QByteArray::fromBase64(signatureBase64.toLatin1());
    if (sigBin.size() != crypto_sign_BYTES) {
        qWarning() << "[QSodiumCryptoSign] Verification aborted: Target signature dimensions are invalid.";
        return false;
    }

    QByteArray const pubKeyBin = QByteArray::fromBase64(publicKey().toLatin1());
    if (pubKeyBin.size() != crypto_sign_PUBLICKEYBYTES) {
        qWarning() << "[QSodiumCryptoSign] Verification aborted: Internal public key structure is empty or corrupted.";
        return false;
    }

    QFile transientFile(filePath);
    if (!transientFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[QSodiumCryptoSign] Cannot open target verification file source:" << filePath;
        return false;
    }

    crypto_sign_state state;
    crypto_sign_init(&state);

    QByteArray buffer(m_chunkSize, '\0');
    while (!transientFile.atEnd()) {
        qint64 bytesRead = transientFile.read(buffer.data(), m_chunkSize);
        if (bytesRead < 0) {
            return false;
        }
        crypto_sign_update(&state, reinterpret_cast<const unsigned char*>(buffer.constData()), bytesRead);
    }

    int const result = crypto_sign_final_verify(&state,
                                                reinterpret_cast<const unsigned char*>(sigBin.constData()),
                                                reinterpret_cast<const unsigned char*>(pubKeyBin.constData())
                                                );

    return (result == 0);
}

bool QSodiumCryptoSign::verifyAssociatedFile(const QString &signatureBase64) noexcept
{
    if (!m_file) return false;

    bool wasOpen = m_file->isOpen();
    if (!wasOpen && !m_file->open(QIODevice::ReadOnly)) return false;

    qint64 originalPosition = m_file->pos();
    m_file->seek(0);

    bool result = verifyFile(m_file->fileName(), signatureBase64);

    if (wasOpen) {
        m_file->seek(originalPosition);
    } else {
        m_file->close();
    }

    return result;
}