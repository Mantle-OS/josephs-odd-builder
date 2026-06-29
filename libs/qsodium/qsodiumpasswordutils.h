#ifndef QSODIUMPASSWORDUTILS_H
#define QSODIUMPASSWORDUTILS_H

#include <sodium/crypto_pwhash.h>

#include <QString>

#include "qsecuremem.h"
#include "qextrarandom.h"

class QSodiumPasswordUtils
{
public:
    QSodiumPasswordUtils() = delete;
    ~QSodiumPasswordUtils() = delete;

    [[nodiscard]] static QString hashPasswordForStorage(const QString &password) noexcept;
    [[nodiscard]] static bool verifyPasswordAgainstStorage(const QString &password, const QString &storedHash) noexcept;
    [[nodiscard]] static bool deriveKeyFromPassword(QSecureMem &outDerivedKey,
                                                    const QString &password,
                                                    const QByteArray &salt = QExtraRandom::randomSalt()) noexcept;
private:
    static constexpr unsigned long long m_opsLimitInteractive = crypto_pwhash_OPSLIMIT_INTERACTIVE;
    static constexpr size_t             m_memLimitInteractive = crypto_pwhash_MEMLIMIT_INTERACTIVE;
};

#endif // QSODIUMPASSWORDUTILS_H