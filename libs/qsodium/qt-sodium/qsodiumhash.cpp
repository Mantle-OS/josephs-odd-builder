#include "qsodiumhash.h"

#include <vector>
#include <filesystem>

#include <QFileInfo>

#include <job_hash.h>

QByteArray QSodiumHash::hashBuffer(const QByteArray &data, std::size_t hashSize, const QByteArray &key) noexcept
{
    std::vector<unsigned char> const nativeData(
        data.constData(),
        data.constData() + data.size()
    );

    const unsigned char *rawKey = key.isEmpty() ?
                                      nullptr :
                                      reinterpret_cast<const unsigned char*>(key.constData());

    std::size_t rawKeyLen = 0;
    if(rawKey)
        rawKeyLen = static_cast<std::size_t>(key.size());

    std::vector<unsigned char> const nativeHash =
        job::crypto::JobHash::hashBuffer(
            nativeData,
            hashSize,
            rawKey,
            rawKeyLen
            );

    return QByteArray(reinterpret_cast<const char*>(nativeHash.data()),
                      static_cast<int>(nativeHash.size()));
}

QByteArray QSodiumHash::hashFile(const QString &filePath, std::size_t hashSize, const QByteArray &key) noexcept
{

    const unsigned char *rawKey = key.isEmpty() ?
                                      nullptr :
                                      reinterpret_cast<const unsigned char*>(key.constData());

    std::size_t rawKeyLen = 0;
    if(rawKey)
        rawKeyLen = static_cast<std::size_t>(key.size());

    std::filesystem::path const nativePath = QFileInfo(filePath).filesystemFilePath();

    std::vector<unsigned char> const nativeHash =
        job::crypto::JobHash::hashFile(
            nativePath.string(),
            hashSize,
            rawKey,
            rawKeyLen
            );

    return QByteArray(reinterpret_cast<const char*>(nativeHash.data()),
                      static_cast<int>(nativeHash.size()));
}