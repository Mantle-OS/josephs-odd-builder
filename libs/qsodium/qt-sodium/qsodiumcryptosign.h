#pragma once

#include <QString>
#include <QByteArray>

#include <job_crypto_sign.h>

#include "qsodiumkeys.h"

class QSodiumCryptoSign : public job::crypto::JobCryptoSign
{
public:
    explicit QSodiumCryptoSign() = default;
    ~QSodiumCryptoSign() = default;

    QSodiumCryptoSign(const QSodiumCryptoSign &other) = default;
    QSodiumCryptoSign &operator=(const QSodiumCryptoSign &other) = default;
    QSodiumCryptoSign(QSodiumCryptoSign &&other) noexcept = default;
    QSodiumCryptoSign &operator=(QSodiumCryptoSign &&other) noexcept = default;

    [[nodiscard]] bool signFile(const QString &filePath, QString &outSignatureBase64) noexcept;
    [[nodiscard]] bool verifyFile(const QString &filePath, const QString &signatureBase64) noexcept;


    [[nodiscard]] QString pubKey() const noexcept
    {
        return QString::fromStdString(publicKey());
    }

    bool signAssociatedFile(const QString &filePath, QString &outSignatureBase64) noexcept
    {
        std::string nativeSig;
        if (!job::crypto::JobCryptoSign::signAssociatedFile(filePath.toStdString(), nativeSig))
            return false;

        outSignatureBase64 = QString::fromStdString(nativeSig);
        return true;
    }


    [[nodiscard]] bool loadKeys(const QString &pubName, const QString &priName) noexcept
    {
        std::filesystem::path pubPath = pubName.toStdString();
        std::filesystem::path priPath = priName.toStdString();
        return loadKeysFromDisk(pubPath, priPath);
    }

    void setPublicKey(const QString &pubKey)
    {
        job::crypto::JobCryptoSign::setPublicKey(pubKey.toStdString());
    }

    void addKeyDirectory(const QDir &dir)
    {
        if(dir.exists()){
            std::filesystem::path const nativePath(dir.absolutePath().toStdString());
            job::crypto::JobCryptoSign::addKeyDirectory(nativePath);
        }
    }

    void addKeyDirectory(const QString &dir)
    {
        QDir d(dir);
        addKeyDirectory(d);
    }

};