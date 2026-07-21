#include "qmlsodiumkeys.h"
#include <QDebug>

QmlSodiumKeys::QmlSodiumKeys(QObject *parent) :
    QObject{parent},
    m_keys{new QSodiumKeys{}}
{
    // Safe value capture of the 'this' object instance context pointer
    connect(this, &QmlSodiumKeys::publicKeyBase64Changed, this, [this](const QString &key){
        if (m_keys)
            m_keys->setPublicKey(key);
    });
}

QmlSodiumKeys::~QmlSodiumKeys()
{
    delete m_keys;
    m_keys = nullptr;
}

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

    auto const coreType = static_cast<job::crypto::JobCryptoKeys::KeyType>(type);
    qDebug() << "KEY TYPE FROM QML"  << static_cast<int>(coreType) << " vs " <<  type;
    if (!m_keys->createKeys(coreType))
        return false;

    set_publicKeyBase64(m_keys->publicKey());
    return true;
}

bool QmlSodiumKeys::validSet() noexcept
{
    return m_keys && m_keys->isValid();
}

bool QmlSodiumKeys::saveKeysToDisk() noexcept
{
    if (!m_keys || !m_keys->isValid()) {
        qWarning() << "[QmlSodiumKeys] Cannot write invalid or empty key states to storage.";
        return false;
    }

    QString const pubPath =  getFullPath(get_publicKeyFile());
    QString const priPath = getFullPath(get_privateKeyFile());

    if (pubPath.isEmpty() || priPath.isEmpty()) {
        qWarning() << "[QmlSodiumKeys] Target key file path variables have not been configured.";
        return false;
    }
    return m_keys->saveKeys(get_keyDir(), pubPath, priPath);
}

bool QmlSodiumKeys::loadKeysFromDisk() noexcept
{
    if (!m_keys)
        return false;

    QString const pubPath = getFullPath(get_publicKeyFile());
    QString const priPath = getFullPath(get_privateKeyFile());

    if (pubPath.isEmpty() || priPath.isEmpty()) {
        qWarning() << "[QmlSodiumKeys] Source key file path variables have not been configured.";
        return false;
    }

    return m_keys->loadKeysFromDisk(pubPath, priPath);
}

// bool QmlSodiumKeys::loadKeysFromDisk() noexcept
// {
//     if (!m_keys)
//         return false;

//     QString const pubPath = getFullPath(get_publicKeyFile());
//     QString const priPath = getFullPath(get_privateKeyFile());

//     return m_keys->loadKeysFromDisk(pubPath, priPath);
// }