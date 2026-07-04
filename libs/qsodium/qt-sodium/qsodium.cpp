#include "qsodium.h"
#include <QDebug>
#include <vector>

QSodium::QSodium(QObject *parent) :
    QObject(parent)
    , m_cryptoBackend()
{
    if (!m_cryptoBackend.isInitialized()) {
        qCritical() << "[QSodium] CRITICAL: Cryptographic hardware initialization failed! Primitives are locked.";
    } else {
        qDebug() << "[QSodium] Service Entry Point actively hooked to meta-object tree via JobCrypto.";
    }
}

bool QSodium::isInitialized() const noexcept
{
    return m_cryptoBackend.isInitialized();
}

QString QSodium::computeFileBlake2b(const QString &filePath) noexcept
{
    if (!isInitialized())
        return {};

    std::string const hexHash = m_cryptoBackend.computeFileBlake2bHex(filePath.toStdString());
    return QString::fromStdString(hexHash);
}

bool QSodium::encryptConfig(const QByteArray &plainText, const QSecureMem &key,
                            QByteArray &outCipherText, QByteArray &outNonce) noexcept
{
    std::vector<unsigned char> const nativePlain(plainText.constData(), plainText.constData() + plainText.size());
    std::vector<unsigned char> nativeCipher;
    std::vector<unsigned char> nativeNonce;

    // Delegate static symmetric workload directly to the JobCrypto core
    if (!job::crypto::JobCrypto::encryptConfig(nativePlain, key, nativeCipher, nativeNonce)) {
        outCipherText.clear();
        outNonce.clear();
        return false;
    }

    outCipherText = QByteArray(reinterpret_cast<const char*>(nativeCipher.data()), static_cast<int>(nativeCipher.size()));
    outNonce = QByteArray(reinterpret_cast<const char*>(nativeNonce.data()), static_cast<int>(nativeNonce.size()));
    return true;
}

bool QSodium::decryptConfig(const QByteArray &cipherText, const QSecureMem &key,
                            const QByteArray &nonce, QSecureMem &outPlainText) noexcept
{
    std::vector<unsigned char> const nativeCipher(cipherText.constData(), cipherText.constData() + cipherText.size());
    std::vector<unsigned char> const nativeNonce(nonce.constData(), nonce.constData() + nonce.size());

    return job::crypto::JobCrypto::decryptConfig(nativeCipher, key, nativeNonce, outPlainText);
}