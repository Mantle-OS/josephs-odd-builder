#include "qzstddecompressor.h"
#include "qzstdio.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>

QZstdDecompressor::QZstdDecompressor(QObject *parent):
    QZstdOptions{parent}
{

}

bool QZstdDecompressor::execute()
{
    if (input().isEmpty() || output().isEmpty()) {
        setErrorString(QStringLiteral("Input or output pathways are completely unconfigured."));
        return false;
    }

    QFileInfo const srcInfo(input());
    if (!srcInfo.exists() || srcInfo.isDir()) {
        setErrorString(QStringLiteral("Source input archive file does not exist or is a directory path."));
        return false;
    }

    // Sniff the file signature to determine the unpacking profile
    QFile probeFile(input());
    if (!probeFile.open(QIODevice::ReadOnly)) {
        setErrorString(probeFile.errorString());
        return false;
    }

    QZstdIO zstdDevice(&probeFile);
    if (!zstdDevice.open(QIODevice::ReadOnly)) {
        setErrorString(zstdDevice.errorString());
        return false;
    }

    QDataStream in(&zstdDevice);
    in.setVersion(headerVersion());

    QString magicTag;
    // Peek at the beginning of the stream layout via the deserializer block
    in >> magicTag;
    zstdDevice.close();
    probeFile.close();

    // Route based on format sniff detection
    // FIXME use the new QZstdIO::magicDirString()
    if (magicTag == QStringLiteral("JOBZDIR1"))
        return decompressFolder();

    return decompressFile();
}

bool QZstdDecompressor::decompressFolder()
{
    QFile srcFile(input());
    if (!srcFile.open(QIODevice::ReadOnly)) {
        setErrorString(srcFile.errorString());
        return false;
    }

    QZstdIO zstdDevice(&srcFile);
    if (!zstdDevice.open(QIODevice::ReadOnly)) {
        setErrorString(zstdDevice.errorString());
        return false;
    }

    // Map progress updates against compressed archive file sizing bounds
    setTotal(static_cast<int>(qMin<qint64>(srcFile.size(), INT_MAX)));
    setCurrent(0);

    QDataStream in(&zstdDevice);
    in.setVersion(headerVersion());

    QString magicTag;
    quint64 totalFiles = 0;

    // Discard magic header tag and extract total count payload metadata
    in >> magicTag;
    in >> totalFiles;

    // The output() parameter acts as our target destination extraction folder
    QDir baseDir(output());
    if (!baseDir.exists() && !baseDir.mkpath(baseDir.absolutePath())) {
        setErrorString(QStringLiteral("Failed to allocate extraction target folder destination path tree."));
        zstdDevice.close();
        return false;
    }

    char buffer[65536];

    for (quint64 i = 0; i < totalFiles; ++i) {
        QString relativePath;
        quint64 fileLength = 0;

        in >> relativePath;
        in >> fileLength;

        QString const targetFilePath = baseDir.absoluteFilePath(relativePath);
        QFileInfo const targetInfo(targetFilePath);
        QDir const parentDir = targetInfo.absoluteDir();

        // Regenerate child subdirectory trees if missing
        if (!parentDir.exists() && !parentDir.mkpath(parentDir.absolutePath())) {
            setErrorString(QStringLiteral("Failed to compose child package filesystem layout hierarchies."));
            zstdDevice.close();
            return false;
        }

        QFile targetFile(targetFilePath);
        if (!targetFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setErrorString(targetFile.errorString());
            zstdDevice.close();
            return false;
        }

        quint64 bytesRemaining = fileLength;
        while (bytesRemaining > 0) {
            qint64 const chunkToRead = static_cast<qint64>(qMin<quint64>(bytesRemaining, sizeof(buffer)));

            // Synchronized with QDataStream structure out of the compressor
            int const bytesRead = in.readRawData(buffer, static_cast<int>(chunkToRead));

            if (bytesRead <= 0) {
                setErrorString(QStringLiteral("Archive format payload stream broken or unexpected EOF reached."));
                targetFile.close();
                zstdDevice.close();
                return false;
            }

            if (targetFile.write(buffer, bytesRead) != bytesRead) {
                setErrorString(targetFile.errorString());
                targetFile.close();
                zstdDevice.close();
                return false;
            }

            bytesRemaining -= bytesRead;
        }

        targetFile.close();
        setCurrent(static_cast<int>(qMin<qint64>(srcFile.pos(), INT_MAX)));
    }

    zstdDevice.close();
    Q_EMIT finished();
    return true;
}

bool QZstdDecompressor::decompressFile()
{
    QFile srcFile(input());
    QFile dstFile(output());

    if (!srcFile.open(QIODevice::ReadOnly)) {
        setErrorString(srcFile.errorString());
        return false;
    }

    if (!dstFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setErrorString(dstFile.errorString());
        return false;
    }

    QZstdIO zstdDevice(&srcFile);
    if (!zstdDevice.open(QIODevice::ReadOnly)) {
        setErrorString(zstdDevice.errorString());
        return false;
    }

    // BACKLOG setup the header

    setTotal(static_cast<int>(qMin<qint64>(srcFile.size(), INT_MAX)));
    setCurrent(0);

    char buffer[65536];
    while (!zstdDevice.atEnd()) {
        qint64 const bytesRead = zstdDevice.read(buffer, sizeof(buffer));

        if (bytesRead < 0) {
            setErrorString(zstdDevice.errorString());
            zstdDevice.close();
            dstFile.close();
            return false;
        }

        if (bytesRead == 0)
            break;

        if (dstFile.write(buffer, bytesRead) != bytesRead) {
            setErrorString(dstFile.errorString());
            zstdDevice.close();
            dstFile.close();
            return false;
        }

        setCurrent(static_cast<int>(qMin<qint64>(srcFile.pos(), INT_MAX)));
    }

    zstdDevice.close();
    dstFile.close();

    Q_EMIT finished();
    return true;
}