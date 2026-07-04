#pragma once

#include <QDir>
#include <QString>
#include <QList>
#include <QDebug>

#include <job_crypto_keys.h>
#include "qsecuremem.h"

class QSodiumKeys
{
public:
    using KeyType = job::crypto::JobCryptoKeys::KeyType;

    explicit QSodiumKeys() :
        m_keys{new job::crypto::JobCryptoKeys()}
    {

    }
    ~QSodiumKeys()
    {
        delete m_keys;
    }

    QString publicKey() const noexcept { return QString::fromStdString(m_keys->publicKey()); }
    void setPublicKey(const QString &pubKey) { m_keys->setPublicKey(pubKey.toStdString()); }

    QSecureMem privateKey() const noexcept
    {
        QSecureMem wrapped;
        wrapped = m_keys->privateKey(); // no overload =
        return wrapped;
    }

    void setPrivateKey(const QSecureMem &privKey) { m_keys->setPrivateKey(privKey); }
    bool isValid() const noexcept { return m_keys->isValid(); }

    bool createKeys(KeyType type) noexcept
    {
        qDebug() << "Qt Land keytype" << static_cast<int>(type);
        return m_keys->createKeys(type);

    }
    bool createSeedKeys(KeyType type, const QSecureMem &seed) noexcept { return m_keys->createSeedKeys(type, seed); }


    [[nodiscard]] bool createKeysAndSave(QString outDir, KeyType type, const QString &pubName, const QString &priName)
    {

        std::filesystem::path path = outDir.toStdString();
        return m_keys->createAndSaveKeys(path,
                                         type,
                                         pubName.toStdString(),
                                         priName.toStdString()
                                         );
    }

    [[nodiscard]] bool saveKeys(QString outDir, const QString &pubName, const QString &priName);

    [[nodiscard]] bool loadKeysFromDisk(const QString &pubName, const QString &priName) noexcept;





    bool createClientSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &serverPublicKey) noexcept {
        return m_keys->createClientSessionKeys(rx, tx, serverPublicKey.toStdString());
    }
    bool createServerSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &clientPublicKey) noexcept {
        return m_keys->createServerSessionKeys(rx, tx, clientPublicKey.toStdString());
    }

private:
    job::crypto::JobCryptoKeys *m_keys{nullptr};
};