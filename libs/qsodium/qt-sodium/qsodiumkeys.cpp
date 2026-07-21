#include "qsodiumkeys.h"

#include <filesystem>

#include <QFileInfo>
#include <QDebug>


QSodiumKeys::QSodiumKeys() :
    m_keys{new job::crypto::JobCryptoKeys()}
{

}

QSodiumKeys::~QSodiumKeys()
{
    delete m_keys;
}

QString QSodiumKeys::publicKey() const noexcept
{
    return QString::fromStdString(m_keys->publicKey());
}

void QSodiumKeys::setPublicKey(const QString &pubKey)
{
    m_keys->setPublicKey(pubKey.toStdString());
}

QSecureMem QSodiumKeys::privateKey() const noexcept
{
    QSecureMem wrapped;
    wrapped = m_keys->privateKey();
    return wrapped;
}

void QSodiumKeys::setPrivateKey(const QSecureMem &privKey)
{
    m_keys->setPrivateKey(privKey);
}

bool QSodiumKeys::isValid() const noexcept
{
    return m_keys->isValid();
}

bool QSodiumKeys::createKeys(KeyType type) noexcept
{
    qDebug() << "Qt Land keytype" << static_cast<int>(type);
    return m_keys->createKeys(type);
}

bool QSodiumKeys::createSeedKeys(KeyType type, const QSecureMem &seed) noexcept
{
    return m_keys->createSeedKeys(type, seed);
}

bool QSodiumKeys::createKeysAndSave(QString outDir, KeyType type, const QString &pubName, const QString &priName)
{
    std::filesystem::path const path = QFileInfo(outDir).filesystemFilePath();
    return m_keys->createAndSaveKeys(path,
                                     type,
                                     pubName.toStdString(),
                                     priName.toStdString()
                                     );
}

bool QSodiumKeys::saveKeys(QString outDir, const QString &pubName, const QString &priName)
{
    std::filesystem::path const path = QFileInfo(outDir).filesystemFilePath();
    // Extract just the filename components ("identity.pub", "identity.key")
    std::string const pubPath = QFileInfo(pubName).fileName().toStdString();
    std::string const priPath = QFileInfo(priName).fileName().toStdString();

    return m_keys->saveKeys(path, pubPath, priPath);
}



bool QSodiumKeys::loadKeysFromDisk(const QString &pubName,
                                   const QString &priName) noexcept
{
    std::filesystem::path const pubPath = QFileInfo(pubName).filesystemFilePath();
    std::filesystem::path const priPath = QFileInfo(priName).filesystemFilePath();
    return m_keys->loadKeysFromDisk(pubPath, priPath);
}

bool QSodiumKeys::createClientSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &serverPublicKey) noexcept
{
    return m_keys->createClientSessionKeys(rx, tx, serverPublicKey.toStdString());
}

bool QSodiumKeys::createServerSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &clientPublicKey) noexcept
{
    return m_keys->createServerSessionKeys(rx, tx, clientPublicKey.toStdString());
}

