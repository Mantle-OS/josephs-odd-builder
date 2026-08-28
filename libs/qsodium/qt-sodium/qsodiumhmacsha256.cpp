#include "qsodiumhmacsha256.h"

#include <algorithm>

#include <job_hmac_sha256.h>

QByteArray QSodiumHmacSha256::compute(const QByteArray &data,
                                      const QSecureMem &key) noexcept
{
    job::crypto::JobHmacSha256::Mac const mac =
        job::crypto::JobHmacSha256::compute(
            data.constData(),
            static_cast<std::size_t>(data.size()),
            key
            );

    return QByteArray(
        reinterpret_cast<const char *>(mac.data()),
        static_cast<int>(mac.size())
        );
}

QSecureMem QSodiumHmacSha256::generateKey() noexcept
{
    return QSecureMem{job::crypto::JobHmacSha256::generateKey()};
}

bool QSodiumHmacSha256::verify(const QByteArray &mac,
                               const QByteArray &data,
                               const QSecureMem &key) noexcept
{
    if (mac.size() != static_cast<int>(job::crypto::JobHmacSha256::kMacSize))
        return false;

    job::crypto::JobHmacSha256::Mac nativeMac{};

    std::copy_n(
        reinterpret_cast<const unsigned char *>(mac.constData()),
        nativeMac.size(),
        nativeMac.begin()
        );

    return job::crypto::JobHmacSha256::verify(
        nativeMac,
        data.constData(),
        static_cast<std::size_t>(data.size()),
        key
        );
}