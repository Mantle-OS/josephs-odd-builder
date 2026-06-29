#include "qsodiumpasswordutils.h"
#include "qsodiuminit.h"
#include <sodium.h>
#include <QByteArray>
#include <QDebug>



QString QSodiumPasswordUtils::hashPasswordForStorage(const QString &password) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize()) {
        qWarning() << "[QSodiumPasswordUtils] Initialization failed; cannot compute verification hash.";
        return {};
    }

    QByteArray const pwdBin = password.toUtf8();
    QByteArray hashedStr(crypto_pwhash_STRBYTES, '\0');

    int const result = crypto_pwhash_str(
        hashedStr.data(),
        pwdBin.constData(),
        pwdBin.size(),
        m_opsLimitInteractive,
        m_memLimitInteractive
        );

    if (result != 0) {
        qWarning() << "[QSodiumPasswordUtils] Error executing crypto_pwhash_str algorithm.";
        return {};
    }

    return QString::fromLatin1(hashedStr.constData());
}

bool QSodiumPasswordUtils::verifyPasswordAgainstStorage(const QString &password, const QString &storedHash) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize())
        return false;

    QByteArray const pwdBin = password.toUtf8();
    QByteArray const hashBin = storedHash.toLatin1();

    // crypto_pwhash_str_verify returns 0 if verification worksd that is ...
    int const result = crypto_pwhash_str_verify(
        hashBin.constData(),
        pwdBin.constData(),
        pwdBin.size()
        );

    return (result == 0);
}


bool QSodiumPasswordUtils::deriveKeyFromPassword(QSecureMem &outDerivedKey,
                                                 const QString &password,
                                                 const QByteArray &salt) noexcept
{
    if (!QSodiumInit::isInitialized() && !QSodiumInit::initialize()) return false;

    if (salt.size() != crypto_pwhash_SALTBYTES) {
        qWarning() << "[QSodiumPasswordUtils] Key derivation aborted: Provided salt size must equal exactly"
                   << crypto_pwhash_SALTBYTES << "bytes.";
        return false;
    }

    // Allocate the secure output target buffer row (Default: 256-bit symmetric key)
    if (!outDerivedKey.allocate(crypto_secretbox_KEYBYTES)) {
        qWarning() << "[QSodiumPasswordUtils] Failed to allocate secure memory row for derived token.";
        return false;
    }

    QByteArray const pwdBin = password.toUtf8();

    int const result = crypto_pwhash(
        outDerivedKey.data(),
        outDerivedKey.size(),
        pwdBin.constData(),
        pwdBin.size(),
        reinterpret_cast<const unsigned char*>(salt.constData()),
        m_opsLimitInteractive,
        m_memLimitInteractive,
        crypto_pwhash_ALG_ARGON2ID13
        );

    if (result != 0) {
        qWarning() << "[QSodiumPasswordUtils] Deterministic sub-key derivation pass failed.";
        outDerivedKey.free();
        return false;
    }

    return true;
}