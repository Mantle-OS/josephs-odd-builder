#include "qzstdcompressorcrypto.h"
#include "qzstdio.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDataStream>

QZstdCompressorCrypto::QZstdCompressorCrypto(QObject *parent) :
    QZstdCompressor{parent}
{

}

QSecureMem QZstdCompressorCrypto::encryptionKey() const
{
    return m_encryptionKey;
}

void QZstdCompressorCrypto::setEncryptionKey(const QSecureMem &key)
{
    if (m_encryptionKey.data() != key.data()) {
        m_encryptionKey = key;
        Q_EMIT encryptionKeyChanged();
    }
}

void QZstdCompressorCrypto::setEncryptionKeyB64(const QString &base64Key)
{
    QSecureMem decryptedKey;
    if (decryptedKey.fromBase64(base64Key))
        setEncryptionKey(decryptedKey);
    else
        qWarning() << "[QZstdCompressorCrypto] Failed to decode base64 encryption key material.";
}

bool QZstdCompressorCrypto::execute()
{
    if (input().isEmpty() || output().isEmpty()) {
        setErrorString(QStringLiteral("Input or output pathways are unconfigured."));
        return false;
    }

    if (m_encryptionKey.isEmpty()) {
        setErrorString(QStringLiteral("Cryptographic pipeline missing secure encryption key context."));
        return false;
    }

    QFileInfo const fi(input());
    if (!fi.exists()) {
        setErrorString(QStringLiteral("Source input layout path does not exist."));
        return false;
    }

    // Ensure our parent destination directory structure exists cleanly
    QFileInfo const dstInfo(output());
    QDir const parentDir = dstInfo.absoluteDir();
    if (!parentDir.exists() && !parentDir.mkpath(parentDir.absolutePath())) {
        setErrorString(QStringLiteral("Failed to generate directory tree layout for destination container."));
        return false;
    }

    if (fi.isDir())
        return compressFolder();

    return compressFile();
}

bool QZstdCompressorCrypto::compressFolder()
{
    QFile dst(output());

    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setErrorString(dst.errorString());
        return false;
    }

    QZstdIO zstd(&dst);
    zstd.setCompressionLevel(compressionLevel());
    if (!zstd.open(QIODevice::WriteOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    QVector<QFileInfo> files;
    qint64 totalBytes = 0;

    QDirIterator it(input(), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        files.push_back(fi);
        totalBytes += fi.size();
    }

    setTotal(static_cast<int>(qMin<qint64>(totalBytes, INT_MAX)));
    setCurrent(0);

    QDataStream out(&zstd);
    out.setVersion(QZstdOptions::headerVersion());

    // JOBZCRYPDIR1
    out << QZstdOptions::magicDirString();
    out << quint64(files.size());

    QDir const root(input());
    char readBuffer[65536];
    qint64 done = 0;

    QSodiumSecretBox box;
    for (const QFileInfo &fi : files) {
        QString const relPath = root.relativeFilePath(fi.absoluteFilePath());

        QFile src(fi.absoluteFilePath());
        if (!src.open(QIODevice::ReadOnly)) {
            setErrorString(src.errorString());
            zstd.close();
            return false;
        }

        out << relPath;
        out << quint64(src.size());

        while (!src.atEnd()) {
            qint64 const n = src.read(readBuffer, sizeof(readBuffer));
            if (n < 0) {
                setErrorString(src.errorString());
                zstd.close();
                return false;
            }
            if (n == 0) break;

            QByteArray const plainChunk = QByteArray::fromRawData(readBuffer, static_cast<int>(n));
            QByteArray encryptedChunk;
            QByteArray chunkNonce;

            if (!box.encrypt(plainChunk, m_encryptionKey,
                             encryptedChunk, chunkNonce)) {
                setErrorString(QStringLiteral("Symmetric encryption wrapper operation failed inside streaming loop."));
                zstd.close();
                return false;
            }

            // Stream both components sequentially into the frame wrapper
            out << chunkNonce;
            out << encryptedChunk;

            done += n;
            setCurrent(static_cast<int>(qMin<qint64>(done, INT_MAX)));
        }
    }

    zstd.close();
    Q_EMIT finished();
    return true;
}

bool QZstdCompressorCrypto::compressFile()
{
    QFile src(input());
    QFile dst(output());

    if (!src.open(QIODevice::ReadOnly)) {
        setErrorString(src.errorString());
        return false;
    }

    if (!dst.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setErrorString(dst.errorString());
        return false;
    }

    QZstdIO zstd(&dst);
    zstd.setCompressionLevel(compressionLevel());

    if (!zstd.open(QIODevice::WriteOnly)) {
        setErrorString(zstd.errorString());
        return false;
    }

    setTotal(static_cast<int>(qMin<qint64>(src.size(), INT_MAX)));
    setCurrent(0);

    QDataStream out(&zstd);
    out.setVersion(QZstdOptions::headerVersion());

    // "JOBZCRYPFILE1"
    out << QZstdOptions::magicFileString();

    char readBuffer[65536];
    qint64 done = 0;
    QSodiumSecretBox box;

    while (!src.atEnd()) {
        qint64 const n = src.read(readBuffer, sizeof(readBuffer));
        if (n < 0) {
            setErrorString(src.errorString());
            zstd.close();
            return false;
        }
        if (n == 0) break;

        QByteArray const plainChunk = QByteArray::fromRawData(readBuffer, static_cast<int>(n));
        QByteArray encryptedChunk;
        QByteArray chunkNonce;

        if (!box.encrypt(plainChunk, m_encryptionKey, encryptedChunk, chunkNonce)) {
            setErrorString(QStringLiteral("Symmetric encryption operation failed during flat file processing."));
            zstd.close();
            return false;
        }

        out << chunkNonce;
        out << encryptedChunk;

        done += n;
        setCurrent(static_cast<int>(qMin<qint64>(done, INT_MAX)));
    }

    zstd.close();
    Q_EMIT finished();
    return true;
}