#ifndef QSODIUMKEYS_H
#define QSODIUMKEYS_H

#include <QFile>
#include <QDir>
#include <QHash>
#include <QString>
#include <QList>
#include <utility>

#include "qsecuremem.h"

class QSodiumKeys
{
public:
    enum class KeyType {
        Sign,    // Ed25519 (Signature matching)
        Exchange // X25519 (Diffie-Hellman Key exchange)
    };

    enum class HashType {
        Blake2b_256,
        Blake2b_512,
        Sha256,
        Sha512
    };

    explicit QSodiumKeys();
    ~QSodiumKeys() = default;

    QSodiumKeys(const QSodiumKeys &other) = default;
    QSodiumKeys &operator=(const QSodiumKeys &other) = default;
    QSodiumKeys(QSodiumKeys &&other) noexcept = default;
    QSodiumKeys &operator=(QSodiumKeys &&other) noexcept = default;


    QString publicKey() const noexcept { return m_publicKey; }
    void setPublicKey(const QString &pubKey) { m_publicKey = pubKey; }

    QSecureMem privateKey() const noexcept { return m_privateKey; }
    void setPrivateKey(const QSecureMem &privKey) {
        m_privateKey.free();
        if (privKey.size() > 0) {
            if(!m_privateKey.allocate(privKey.size())){
                qCritical() << "[QSodiumKeys] CRITICAL memory allocation failure! Unable to secure kernel-locked RAM page row.";
                return;
            }
            std::memcpy(m_privateKey.data(), privKey.data(), privKey.size());
        }
        m_validKeys = (m_privateKey.size() > 0 && !m_publicKey.isEmpty());
    }

    [[nodiscard]] bool isValid() const noexcept { return m_validKeys; }

    [[nodiscard]] bool createKeys(KeyType type) noexcept;
    [[nodiscard]] bool createSeedKeys(KeyType type, const QSecureMem &seed) noexcept;

    [[nodiscard]] bool createClientSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &serverPublicKey) noexcept;
    [[nodiscard]] bool createServerSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &clientPublicKey) noexcept;

    void addKeyDirectory(const QDir &dir);
    [[nodiscard]] QList<QDir> keyDirectories() const noexcept { return m_keysDirs; }

private:
    QList<QDir> m_keysDirs;

    QString m_publicKey;
    QSecureMem m_privateKey; // Guaranteed cryptographic lock in RAM
    bool m_validKeys{false};

    // Keystore maps: Identifiers track paired allocations safely <Uid, <PublicKey, ProtectedPrivateKey>>
    QHash<QString, std::pair<QString, QSecureMem>> m_keyStore;
};

#endif // QSODIUMKEYS_H