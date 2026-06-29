#ifndef HUGGINGFACECACHE_H
#define HUGGINGFACECACHE_H

#include <QObject>
#include <QQmlEngine>
#include <QFile>
#include <QDir>

#include <objectmodel.h>
#include <pointer-macros.h>

#include "huggingfacepackagemanifest.h"

#include <zstd.h>
#include <zstd_errors.h>

class HuggingFaceCache : public QObject
{
    Q_OBJECT
    QP_PTR_RO(ObjectListModel<HuggingFacePackageManifest>, cache)
    QP_RO(QString, basePath, QAiUtils::packagesDir)
    QP_RW(int, compression_level, 3)
    QML_ELEMENT

public:
    explicit HuggingFaceCache(QObject *parent = nullptr);

    bool readFromFile()
    {
        QString filePath = QString("%1/%2").arg(m_basePath, "Packages.zst");
        if (!QFile::exists(filePath)) {
            qInfo() << "HuggingFaceCache: No local cache file found at" << filePath;
            return false;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "HuggingFaceCache: Failed to open file for reading:" << filePath;
            return false;
        }
        QByteArray compressedData = file.readAll();
        file.close();

        if (compressedData.isEmpty()) return false;

        unsigned long long const uncompressedSize = ZSTD_getFrameContentSize(compressedData.constData(), compressedData.size());
        if (uncompressedSize == ZSTD_CONTENTSIZE_ERROR) {
            qWarning() << "HuggingFaceCache: ZSTD data is not a valid compressed frame.";
            return false;
        } else if (uncompressedSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            qWarning() << "HuggingFaceCache: Original uncompressed size cannot be determined.";
            return false;
        }

        QByteArray uncompressedData;
        uncompressedData.resize(static_cast<qsizetype>(uncompressedSize));

        // Perform decompression
        size_t const decompressResult = ZSTD_decompress(
            uncompressedData.data(),       // Destination buffer
            uncompressedSize,              // Max capacity
            compressedData.constData(),    // Source compressed bytes
            compressedData.size()          // Source size
            );

        if (ZSTD_isError(decompressResult)) {
            qWarning() << "HuggingFaceCache: ZSTD Decompression Fault:" << ZSTD_getErrorName(decompressResult);
            return false;
        }

        QJsonParseError parseError;
        QJsonDocument jdoc = QJsonDocument::fromJson(uncompressedData, &parseError);
        if (jdoc.isNull()) {
            qWarning() << "HuggingFaceCache: Failed to parse decompressed JSON structure:" << parseError.errorString();
            return false;
        }

        m_cache->clear();
        if (jdoc.isArray()) {
            QJsonArray rootArray = jdoc.array();
            for (const QJsonValue &val : rootArray) {
                if (val.isObject()) {
                    auto *manifest = new HuggingFacePackageManifest(this);
                    manifest->fromJson(val.toObject());
                    m_cache->append(manifest);
                }
            }
        }

        qDebug() << "HuggingFaceCache: Successfully loaded" << m_cache->count() << "cached packages from disk.";
        return true;

    }
    bool writeCache(){
        if (!QDir().mkpath(m_basePath)) {
            qWarning() << "HuggingFaceCache: Failed to guarantee base path directory tree:" << m_basePath;
            return false;
        }

        QJsonArray rootArray;
        for (auto *manifest : m_cache->toList()) {
            if (manifest) {
                rootArray.append(manifest->toJson());
            }
        }

        QJsonDocument jdoc;
        jdoc.setArray(rootArray);
        QByteArray uncompressedData = jdoc.toJson(QJsonDocument::Compact);

        if (uncompressedData.isEmpty())
            return false;

        size_t const maxCompressedSize = ZSTD_compressBound(uncompressedData.size());
        QByteArray compressedData;
        compressedData.resize(static_cast<qsizetype>(maxCompressedSize));

        size_t const compressResult = ZSTD_compress(
            compressedData.data(),         // Destination buffer
            maxCompressedSize,             // Destination buffer capacity bounds
            uncompressedData.constData(),  // Source uncompressed JSON string bytes
            uncompressedData.size(),       // Source size
            get_compression_level()         // Compression level preset
            );

        if (ZSTD_isError(compressResult)) {
            qWarning() << "HuggingFaceCache: ZSTD Compression Fault:" << ZSTD_getErrorName(compressResult);
            return false;
        }

        compressedData.resize(static_cast<qsizetype>(compressResult));

        QString filePath = QString("%1/%2").arg(m_basePath, "Packages.zst");
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qWarning() << "HuggingFaceCache: Failed to map file write permissions to:" << filePath;
            return false;
        }

        qint64 const bytesWritten = file.write(compressedData);
        file.close();
        file.flush();

        if (bytesWritten != compressedData.size()) {
            qWarning() << "HuggingFaceCache: Partial write anomaly encountered while writing cache to disk.";
            return false;
        }

        qDebug() << "HuggingFaceCache: Cached payload optimized and written to disk successfully ("
                 << uncompressedData.size() << "bytes packed down to" << compressResult << "bytes).";
        return true;
    }

    bool updatePackage(HuggingFacePackageManifest *package){
        bool ret = false;
        if(m_cache->contains(package)){
            QString inUid = package->get_uid();
            HuggingFacePackageManifest *item = m_cache->getByUid(inUid);
            if(item){
                item->update(package);
                ret = true;
            }
        }
        return ret;
    }

private:
    // This list is gathered from the docs in stable-diff.cpp
    QStringList m_knownRepos{};
};

#endif // HUGGINGFACECACHE_H
