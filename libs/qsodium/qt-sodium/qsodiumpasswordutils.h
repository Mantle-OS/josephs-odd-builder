#pragma once

#include <QString>
#include <QByteArray>

#include <job_password_utils.h>
#include "qsecuremem.h"

#include "qsodium_export.h"

class QSODIUM_EXPORT QSodiumPasswordUtils {
public:
    QSodiumPasswordUtils() = delete;
    ~QSodiumPasswordUtils() = delete;
    // FIXME later add the fab 5 but not really needed atm

    [[nodiscard]] static QString hashPasswordForStorage(const QSecureMem &password) noexcept;
    [[nodiscard]] static bool verifyPasswordAgainstStorage(const QSecureMem &password, const QString &storedHash) noexcept;
    [[nodiscard]] static bool deriveKeyFromPassword(QSecureMem &outDerivedKey,
                                                    const QSecureMem &password,
                                                    const QByteArray &salt) noexcept;
};