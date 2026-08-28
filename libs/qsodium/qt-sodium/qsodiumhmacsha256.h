#pragma once

#include <QByteArray>

#include <qsecuremem.h>

#include "qsodium_export.h"

class QSODIUM_EXPORT QSodiumHmacSha256
{
public:
    QSodiumHmacSha256() = delete;
    ~QSodiumHmacSha256() = delete;

    QSodiumHmacSha256(const QSodiumHmacSha256 &) = delete;
    QSodiumHmacSha256 &operator=(const QSodiumHmacSha256 &) = delete;
    QSodiumHmacSha256(QSodiumHmacSha256 &&) = delete;
    QSodiumHmacSha256 &operator=(QSodiumHmacSha256 &&) = delete;

    [[nodiscard]] static QByteArray compute(const QByteArray &data,
                                            const QSecureMem &key) noexcept;

    [[nodiscard]] static QSecureMem generateKey() noexcept;

    [[nodiscard]] static bool verify(const QByteArray &mac,
                                     const QByteArray &data,
                                     const QSecureMem &key) noexcept;
};