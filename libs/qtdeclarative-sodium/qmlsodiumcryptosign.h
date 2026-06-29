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
    QP_RW(QString, filePath,        "")
    QP_RW(QString, publicKeyBase64, "")
    QP_RW(QString, signatureBase64, "")

    QML_ELEMENT
public:
    explicit QmlSodiumCryptoSign(QObject *parent = nullptr) :
        QObject{parent},
        m_signer{new QSodiumCryptoSign{}}
    {
        connect(this, &QmlSodiumCryptoSign::publicKeyBase64Changed, this, [&](const QString &key){
            m_signer->setPublicKey(key);
        });
    }

    ~QmlSodiumCryptoSign() override {
        delete m_signer;
        m_signer = nullptr;
    }

    Q_INVOKABLE bool signFile() noexcept {
        if (!m_signer || get_filePath().isEmpty())
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
        if (!m_signer)
            return false;

        QString outSig;
        if (m_signer->signAssociatedFile(outSig)) {
            set_signatureBase64(outSig);
            return true;
        }
        return false;
    }

    Q_INVOKABLE bool verifyAssociatedFile() noexcept {
        if (vaildProperties()) {
            return m_signer->verifyFile(get_filePath(), get_signatureBase64());
        }
        return false;
    }

    Q_INVOKABLE QString computeFileBlake2b() noexcept {
        if (get_filePath().isEmpty())
            return {};

        QByteArray const binaryHash = QSodiumHash::hashFile(get_filePath());
        return QString::fromLatin1(binaryHash.toHex());
    }

    Q_INVOKABLE bool loadPublicKeyFromFile(const QString &localFilePath) noexcept
    {
        if (localFilePath.isEmpty() || !QAiUtils::fileExists(localFilePath)) {
            qWarning() << "[QmlSodiumCryptoSign] Public key file path missing or invalid:" << localFilePath;
            return false;
        }
        QString const pubKeyData = QAiUtils::readTextFile(localFilePath).trimmed();

        if (pubKeyData.isEmpty()) {
            return false;
        }
        set_publicKeyBase64(pubKeyData);
        return true;
    }

private:
    QSodiumCryptoSign *m_signer = nullptr;

    bool vaildProperties() noexcept {
        if (!m_signer || get_filePath().isEmpty() || get_publicKeyBase64().isEmpty() || get_signatureBase64().isEmpty()) {
            return false;
        }
        return true;
    }
};

#endif // QMLSODIUMCRYPTOSIGN_H