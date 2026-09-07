#pragma once

#include <cstddef>

#include <QByteArray>
#include <QString>

#include <job_sha.h>

#include "qsodium_export.h"

class QSODIUM_EXPORT QSodiumSha
{
public:
    QSodiumSha() = delete;
    ~QSodiumSha() = delete;

    QSodiumSha(const QSodiumSha &) = delete;
    QSodiumSha &operator=(const QSodiumSha &) = delete;
    QSodiumSha(QSodiumSha &&) = delete;
    QSodiumSha &operator=(QSodiumSha &&) = delete;

    [[nodiscard]] static QByteArray hashBuffer(job::crypto::JobShaType type,
                                               const QByteArray &data) noexcept;

    [[nodiscard]] static QByteArray hashBufferHex(job::crypto::JobShaType type,
                                                  const QByteArray &data) noexcept;

    [[nodiscard]] static QString hashStringHex(job::crypto::JobShaType type,
                                               const QString &data) noexcept;

    [[nodiscard]] static constexpr bool supports(job::crypto::JobShaType type) noexcept
    {
        return job::crypto::JobSha::supports(type);
    }

    [[nodiscard]] static constexpr bool requiresKey(job::crypto::JobShaType type) noexcept
    {
        return job::crypto::JobSha::requiresKey(type);
    }

    [[nodiscard]] static constexpr std::size_t hashSize(job::crypto::JobShaType type) noexcept
    {
        return job::crypto::JobSha::hashSize(type);
    }
};