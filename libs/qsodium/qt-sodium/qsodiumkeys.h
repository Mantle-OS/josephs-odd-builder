#pragma once

#include <QString>

#include <job_crypto_keys.h>

#include "qsecuremem.h"
#include "qsodium_export.h"

class QSODIUM_EXPORT QSodiumKeys {
public:
    using KeyType = job::crypto::JobCryptoKeys::KeyType;
    explicit QSodiumKeys();
    ~QSodiumKeys();
    QSodiumKeys(const QSodiumKeys &) = delete;
    QSodiumKeys &operator=(const QSodiumKeys &) = delete;

    QString publicKey() const noexcept;
    void setPublicKey(const QString &pubKey);

    QSecureMem privateKey() const noexcept;
    void setPrivateKey(const QSecureMem &privKey);

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool createKeys(KeyType type) noexcept;
    [[nodiscard]] bool createSeedKeys(KeyType type, const QSecureMem &seed) noexcept;

    [[nodiscard]] bool createKeysAndSave(QString outDir, KeyType type,
                                         const QString &pubName,
                                         const QString &priName);

    [[nodiscard]] bool saveKeys(QString outDir, const QString &pubName, const QString &priName);
    [[nodiscard]] bool loadKeysFromDisk(const QString &pubName, const QString &priName) noexcept;

    bool createClientSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &serverPublicKey) noexcept;
    bool createServerSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &clientPublicKey) noexcept;

private:
    job::crypto::JobCryptoKeys *m_keys{nullptr};
};