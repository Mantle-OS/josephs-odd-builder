#ifndef QMLSODIUMCRYPTOSIGN_H
#define QMLSODIUMCRYPTOSIGN_H

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

#include <property-macros.h>
#include <qaiutils.h>

#include <qsodiumcryptosign.h>
#include <qsodiumhash.h>


class QmlSodiumCryptoSign : public QObject
{
    Q_OBJECT
    QP_RW(QString, filePath,        "") // the file to sign
    QP_RW(QString, publicKey,       "") // the public key path + file
    QP_RW(QString, privateKey,      "") // the private key path + file
    QP_RW(QString, signatureBase64, "") // The OUT signature Base 64
    QML_ELEMENT
public:
    explicit QmlSodiumCryptoSign(QObject *parent = nullptr) :
        QObject{parent},
        m_signer{new QSodiumCryptoSign{}}
    {
        connect(this, &QmlSodiumCryptoSign::publicKeyChanged, this, [&](const QString &key) {
            QFileInfo pubFi(key);
            QFileInfo priFi(get_privateKey());
            if(pubFi.exists() && priFi.exists()){
                if(m_signer->loadKeys(key, get_privateKey())){
                    qDebug() << "Keys Loaded:";
                    qDebug() << "PUB: " << key;
                    qDebug() << "PRI: " << get_privateKey();
                }
            }
        });

        connect(this, &QmlSodiumCryptoSign::privateKeyChanged, this, [&](const QString &key) {
            QFileInfo pubFi(get_publicKey());
            QFileInfo priFi(key);
            if(pubFi.exists() && priFi.exists()){
                if(m_signer->loadKeys(get_publicKey(), key)){
                    qDebug() << "Keys Loaded";
                    qDebug() << "PUB: " << get_publicKey();
                    qDebug() << "PRI: " << key;
                }
            }
        });
    }

    ~QmlSodiumCryptoSign() override
    {
        delete m_signer;
        m_signer = nullptr;
    }

    Q_INVOKABLE bool signFile() noexcept
    {
        if (!hasKeys() || get_filePath().isEmpty())
            return false;

        QString outSig;
        // Extracts underlying properties natively, then syncs state back out to QML row properties
        if (m_signer->signFile(get_filePath(), outSig)) {
            set_signatureBase64(outSig);
            return true;
        }
        return false;
    }

    Q_INVOKABLE bool signAssociatedFile() noexcept {
        if (!hasKeys() || get_filePath().isEmpty())
            return false;

        QString outSig;
        if (m_signer->signAssociatedFile(get_filePath(), outSig)) {
            set_signatureBase64(outSig);
            return true;
        }
        return false;
    }

    Q_INVOKABLE bool verifyAssociatedFile() noexcept
    {
        if (!hasKeys() || get_filePath().isEmpty())
            return false;

        return m_signer->verifyFile(get_filePath(), get_signatureBase64());
    }

    // dont need keys
    Q_INVOKABLE QString computeFileBlake2b() noexcept {
        if (get_filePath().isEmpty())
            return {};

        QByteArray const binaryHash = QSodiumHash::hashFile(get_filePath());
        return QString::fromLatin1(binaryHash.toHex());
    }


    Q_INVOKABLE bool hasKeys() noexcept
    {
        if (!m_signer || !m_signer->isValid())
            return false;
        return true;
    }

private:
    bool loadKeysFromDisk() noexcept
    {
        if (!m_signer)
            return false;

        return m_signer->loadKeys(get_publicKey(), get_privateKey());
    }
    QSodiumCryptoSign *m_signer = nullptr;
};

#endif // QMLSODIUMCRYPTOSIGN_H