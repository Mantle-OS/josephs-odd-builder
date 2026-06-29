#include "qsodium.h"
#include "qsodiuminit.h"
#include <QDebug>

QSodium::QSodium(QObject *parent) :
    QObject(parent)
    , m_initSuccess(false)
{
    if (QSodiumInit::isInitialized()) {
        m_initSuccess = true;
    } else {
        m_initSuccess = QSodiumInit::initialize();
        if (!m_initSuccess) {
            qCritical() << "[QSodium] CRITICAL: Cryptographic hardware initialization failed! Primitives are locked.";
        } else {
            qDebug() << "[QSodium] Service Entry Point actively hooked to meta-object tree.";
        }
    }
}

bool QSodium::isInitialized() const noexcept
{
    return m_initSuccess;
}

bool QSodium::verifyFileSignature(const QString &filePath,
                                  const QString &publicKeyBase64,
                                  const QString &signatureBase64) noexcept
{
    if (!m_initSuccess)
        return false;
    QSodiumCryptoSign verifier;
    verifier.setPublicKey(publicKeyBase64);

    return verifier.verifyFile(filePath, signatureBase64);
}

QString QSodium::computeFileBlake2b(const QString &filePath) noexcept
{
    if (!m_initSuccess) return {};

    QByteArray const binaryHash = QSodiumHash::hashFile(filePath);
    if (binaryHash.isEmpty()) {
        return {};
    }

    return QString::fromLatin1(binaryHash.toHex());
}


bool QSodium::encryptConfig(const QByteArray &plainText, const QSecureMem &key,
                            QByteArray &outCipherText, QByteArray &outNonce) noexcept
{
    return QSodiumSecretBox::encrypt(plainText, key, outCipherText, outNonce);
}

bool QSodium::decryptConfig(const QByteArray &cipherText, const QSecureMem &key,
                            const QByteArray &nonce, QSecureMem &outPlainText) noexcept
{
    return QSodiumSecretBox::decrypt(cipherText, key, nonce, outPlainText);
}