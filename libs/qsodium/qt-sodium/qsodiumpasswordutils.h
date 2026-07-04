#pragma once

#include <QString>
#include <QByteArray>

#include <job_password_utils.h>
#include "qsecuremem.h"

class QSodiumPasswordUtils
{
public:
    QSodiumPasswordUtils() = delete;
    ~QSodiumPasswordUtils() = delete;

    [[nodiscard]] static QString hashPasswordForStorage(const QSecureMem &password) noexcept;
    [[nodiscard]] static bool verifyPasswordAgainstStorage(const QSecureMem &password, const QString &storedHash) noexcept;
    [[nodiscard]] static bool deriveKeyFromPassword(QSecureMem &outDerivedKey,
                                                    const QSecureMem &password,
                                                    const QByteArray &salt) noexcept;
};