#include "qsodiumsha.h"

#include <string>
#include <string_view>

#include <job_sha.h>

QByteArray QSodiumSha::hashBuffer(job::crypto::JobShaType type,
                                  const QByteArray &data) noexcept
{
    job::crypto::JobSha::Hash const hash = job::crypto::JobSha::compute(type, data.constData(), static_cast<std::size_t>(data.size()));

    if (hash.empty())
        return {};

    return QByteArray(reinterpret_cast<const char *>(hash.data()),
                      static_cast<qsizetype>(hash.size()));
}

QByteArray QSodiumSha::hashBufferHex(job::crypto::JobShaType type, const QByteArray &data) noexcept
{
    std::string const hash = job::crypto::JobSha::computeHex(type, data.constData(), static_cast<std::size_t>(data.size()));

    if (hash.empty())
        return {};

    return QByteArray(hash.data(), static_cast<qsizetype>(hash.size()));
}

QString QSodiumSha::hashStringHex(job::crypto::JobShaType type, const QString &data) noexcept
{
    QByteArray const utf8 = data.toUtf8();
    std::string const hash = job::crypto::JobSha::computeHex(type, utf8.constData(), static_cast<std::size_t>(utf8.size()));

    if (hash.empty())
        return {};

    return QString::fromLatin1(hash.data(), static_cast<qsizetype>(hash.size()));
}