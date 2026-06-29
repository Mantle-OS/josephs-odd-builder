#include "qsodiumkeys.h"
#include "qsodiuminit.h"
#include <sodium.h>
#include <QStandardPaths>
#include <QDebug>

QSodiumKeys::QSodiumKeys()
{
    if (!QSodiumInit::initialize()) {
        qWarning() << "[QSodiumKeys] Critical: Libsodium initialization failed! Primitives unavailable.";
        return;
    }

    // Default configuration workspace setup
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation).append("/qsodium");

    m_keysDirs.append(QDir(defaultPath));
}

void QSodiumKeys::addKeyDirectory(const QDir &dir)
{
    if (!m_keysDirs.contains(dir)) {
        m_keysDirs.append(dir);
    }
}
bool QSodiumKeys::createKeys(KeyType type) noexcept
{
    m_validKeys = false;

    if (type == KeyType::Exchange) {
        if (!m_privateKey.allocate(crypto_kx_SECRETKEYBYTES)) {
            qWarning() << "[QSodiumKeys] Failed to allocate secure memory for Exchange secret key.";
            return false;
        }
        QByteArray pubTemp(crypto_kx_PUBLICKEYBYTES, '\0');

        if (crypto_kx_keypair(reinterpret_cast<unsigned char*>(pubTemp.data()), m_privateKey.data()) != 0) {
            qWarning() << "[QSodiumKeys] Failed to generate Exchange keypair.";
            return false;
        }
        m_publicKey = QString::fromLatin1(pubTemp.toBase64());
        m_validKeys = true;
    }
    else if (type == KeyType::Sign) {
        if (!m_privateKey.allocate(crypto_sign_SECRETKEYBYTES)) {
            qWarning() << "[QSodiumKeys] Failed to allocate secure memory for Signature secret key.";
            return false;
        }
        QByteArray pubTemp(crypto_sign_PUBLICKEYBYTES, '\0');

        if (crypto_sign_keypair(reinterpret_cast<unsigned char*>(pubTemp.data()), m_privateKey.data()) != 0) {
            qWarning() << "[QSodiumKeys] Failed to generate Signature keypair.";
            return false;
        }
        m_publicKey = QString::fromLatin1(pubTemp.toBase64());
        m_validKeys = true;
    }

    return m_validKeys;
}

bool QSodiumKeys::createSeedKeys(KeyType type, const QSecureMem &seed) noexcept
{
    m_validKeys = false;

    if (type == KeyType::Exchange) {
        if (seed.size() != crypto_kx_SEEDBYTES) {
            qWarning() << "[QSodiumKeys] Seed keys initialization failed: Invalid seed buffer dimensions.";
            return false;
        }
        if (!m_privateKey.allocate(crypto_kx_SECRETKEYBYTES)) {
            qWarning() << "[QSodiumKeys] Failed to allocate secure memory for Exchange seed secret key.";
            return false;
        }
        QByteArray pubTemp(crypto_kx_PUBLICKEYBYTES, '\0');

        if (crypto_kx_seed_keypair(reinterpret_cast<unsigned char*>(pubTemp.data()), m_privateKey.data(), seed.data()) != 0) {
            return false;
        }
        m_publicKey = QString::fromLatin1(pubTemp.toBase64());
        m_validKeys = true;
    }
    else if (type == KeyType::Sign) {
        if (seed.size() != crypto_sign_SEEDBYTES) {
            qWarning() << "[QSodiumKeys] Seed keys initialization failed: Invalid seed buffer dimensions.";
            return false;
        }
        if (!m_privateKey.allocate(crypto_sign_SECRETKEYBYTES)) {
            qWarning() << "[QSodiumKeys] Failed to allocate secure memory for Signature seed secret key.";
            return false;
        }
        QByteArray pubTemp(crypto_sign_PUBLICKEYBYTES, '\0');

        if (crypto_sign_seed_keypair(reinterpret_cast<unsigned char*>(pubTemp.data()), m_privateKey.data(), seed.data()) != 0) {
            return false;
        }
        m_publicKey = QString::fromLatin1(pubTemp.toBase64());
        m_validKeys = true;
    }

    return m_validKeys;
}


bool QSodiumKeys::createClientSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &serverPublicKey) noexcept
{
    if (!m_validKeys || m_privateKey.size() != crypto_kx_SECRETKEYBYTES) return false;

    QByteArray const serverPubBin = QByteArray::fromBase64(serverPublicKey.toLatin1());
    if (serverPubBin.size() != crypto_kx_PUBLICKEYBYTES) return false;

    QByteArray const clientPubBin = QByteArray::fromBase64(m_publicKey.toLatin1());
    if (clientPubBin.size() != crypto_kx_PUBLICKEYBYTES) return false;

    if (!rx.allocate(crypto_kx_SESSIONKEYBYTES)) {
        qWarning() << "[QSodiumKeys] Failed to allocate RX session key buffer.";
        return false;
    }
    if (!tx.allocate(crypto_kx_SESSIONKEYBYTES)) { // Fixed: added missing logic inversion flag
        qWarning() << "[QSodiumKeys] Failed to allocate TX session key buffer.";
        return false;
    }

    // Fixed: Applied clean reinterpret_casts to match Libsodium's exact unsigned char signatures
    int const result = crypto_kx_client_session_keys(
        rx.data(),
        tx.data(),
        reinterpret_cast<const unsigned char*>(clientPubBin.constData()),
        m_privateKey.data(),
        reinterpret_cast<const unsigned char*>(serverPubBin.constData())
        );

    return (result == 0);
}

bool QSodiumKeys::createServerSessionKeys(QSecureMem &rx, QSecureMem &tx, const QString &clientPublicKey) noexcept
{
    if (!m_validKeys || m_privateKey.size() != crypto_kx_SECRETKEYBYTES) return false;

    QByteArray const clientPubBin = QByteArray::fromBase64(clientPublicKey.toLatin1());
    if (clientPubBin.size() != crypto_kx_PUBLICKEYBYTES) return false;

    QByteArray const serverPubBin = QByteArray::fromBase64(m_publicKey.toLatin1());
    if (serverPubBin.size() != crypto_kx_PUBLICKEYBYTES) return false;

    if (!rx.allocate(crypto_kx_SESSIONKEYBYTES)) {
        qWarning() << "[QSodiumKeys] Failed to allocate RX session key buffer.";
        return false;
    }
    if (!tx.allocate(crypto_kx_SESSIONKEYBYTES)) {
        qWarning() << "[QSodiumKeys] Failed to allocate TX session key buffer.";
        return false;
    }

    // Fixed: Applied clean reinterpret_casts to match Libsodium's exact unsigned char signatures
    int const result = crypto_kx_server_session_keys(
        rx.data(),
        tx.data(),
        reinterpret_cast<const unsigned char*>(serverPubBin.constData()),
        m_privateKey.data(),
        reinterpret_cast<const unsigned char*>(clientPubBin.constData())
        );

    return (result == 0);
}