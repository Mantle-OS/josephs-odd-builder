#include "qmlsodiumkeys.h"
#include <QDebug>

QString QmlSodiumKeys::getFullPath(const QString &fileName) const noexcept
{
    if (get_keyDir().isEmpty() || fileName.isEmpty())
        return {};

    if (get_keyDir().endsWith('/')) {
        return get_keyDir() + fileName;
    }
    return get_keyDir() + "/" + fileName;
}

bool QmlSodiumKeys::create(QmlSodiumKeys::KeyType type) noexcept
{
    if (!m_keys)
        return false;

    auto const coreType = static_cast<QSodiumKeys::KeyType>(type);
    if (!m_keys->createKeys(coreType))
        return false;

    set_publicKeyBase64(m_keys->publicKey());
    return true;
}

bool QmlSodiumKeys::saveKeysToDisk() noexcept
{
    if (!m_keys || !m_keys->isValid()) {
        qWarning() << "[QmlSodiumKeys] Cannot write invalid or empty key states to storage.";
        return false;
    }

    QString const pubPath = getFullPath(get_publicKeyFile());
    QString const privPath = getFullPath(get_privateKeyFile());

    if (pubPath.isEmpty() || privPath.isEmpty()) {
        qWarning() << "[QmlSodiumKeys] Target key file path variables have not been configured.";
        return false;
    }

    if (!QAiUtils::writeTextFile(pubPath, m_keys->publicKey()))
        return false;

    if (!QAiUtils::writeTextFile(privPath, m_keys->privateKey().toString()))
        return false;

    return true;
}

bool QmlSodiumKeys::loadKeysFromDisk() noexcept
{
    if (!m_keys)
        return false;

    QString const pubPath = getFullPath(get_publicKeyFile());
    QString const privPath = getFullPath(get_privateKeyFile());

    if (!QAiUtils::fileExists(pubPath) || !QAiUtils::fileExists(privPath))
        return false;

    QString const pubKeyData = QAiUtils::readTextFile(pubPath).trimmed();
    m_keys->setPublicKey(pubKeyData);

    QFile privFile(privPath);
    if (!privFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[QmlSodiumKeys] Security Error: Unable to open private key file asset.";
        return false;
    }

    // Read directly into a continuous binary array block
    QByteArray rawPrivData = privFile.readAll().trimmed();
    privFile.close();

    if (rawPrivData.isEmpty())
        return false;

    QSecureMem securePrivMem;
    if (!securePrivMem.allocate(rawPrivData.size())) {
        // Zero-out sensitive data buffer instantly on allocation fault
        sodium_memzero(rawPrivData.data(), rawPrivData.size());
        return false;
    }

    std::memcpy(securePrivMem.data(), rawPrivData.constData(), rawPrivData.size());

    sodium_memzero(rawPrivData.data(), rawPrivData.size());
    rawPrivData.clear();

    m_keys->setPrivateKey(securePrivMem);

    if (m_keys->isValid()) {
        set_publicKeyBase64(m_keys->publicKey());
        return true;
    }

    return false;
}