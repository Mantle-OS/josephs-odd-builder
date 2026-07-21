#pragma once

#include <QDir>
#include <QString>

#include <job_crypto_sign.h>

#include "qsodium_export.h"

class QSODIUM_EXPORT QSodiumCryptoSign : public job::crypto::JobCryptoSign
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

    [[nodiscard]] QString pubKey() const noexcept;

    [[nodiscard]] bool signAssociatedFile(const QString &filePath, QString &outSignatureBase64) noexcept;
    [[nodiscard]] bool loadKeys(const QString &pubName, const QString &priName) noexcept;

    void setPublicKey(const QString &pubKey);
    void addKeyDirectory(const QDir &dir);
    void addKeyDirectory(const QString &dir);

};