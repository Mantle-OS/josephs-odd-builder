#pragma once
#include <QObject>
#include <QString>
#include <QUrl>

#include <qqmlregistration.h>

#include <property-macros.h>
#include <qaiutils.h>

#include <qsodiumcryptosign.h>
#include <qsodiumhash.h>

#include "qmlsodium_export.h"

class QMLSODIUM_EXPORT QmlSodiumCryptoSign : public QObject
{
    Q_OBJECT
    QP_RW(QString, filePath,        "") // the file to sign
    QP_RW(QString, publicKey,       "") // the public key path + file
    QP_RW(QString, privateKey,      "") // the private key path + file
    QP_RW(QString, signatureBase64, "") // The OUT signature Base 64
    QML_ELEMENT
public:
    explicit QmlSodiumCryptoSign(QObject *parent = nullptr);
    ~QmlSodiumCryptoSign() override;
    enum Stage{
        File = 0,
        PublicKey = 1,
        PrivateKey = 2,
        Unknown = 99
    };
    Q_ENUMS(Stage)

    Q_INVOKABLE bool signFile() noexcept;
    Q_INVOKABLE bool signAssociatedFile() noexcept;
    Q_INVOKABLE bool verifyAssociatedFile() noexcept;
    Q_INVOKABLE QString computeFileBlake2b() noexcept;
    Q_INVOKABLE bool hasKeys() noexcept;

public Q_SLOTS:
    [[nodiscard]] bool update_filePath (const QUrl &url) noexcept;
    [[nodiscard]] bool update_publicKey (const QUrl &url) noexcept;
    [[nodiscard]] bool update_privateKey(const QUrl &url) noexcept;

protected:
    QP_RO(QmlSodiumCryptoSign::Stage , lastStage , QmlSodiumCryptoSign::Unknown)

private:
    [[nodiscard]] bool loadKeysFromDisk() noexcept;
    [[nodiscard]] bool urlStr( const QUrl &url, QString &path ) const noexcept; // might move this over to shome helper lib...
    QSodiumCryptoSign *m_signer = nullptr;
};
