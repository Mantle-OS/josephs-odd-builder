#include "qzstdcompressor.h"
#include "qzstdio.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QDataStream>

QZstdCompressor::QZstdCompressor(QObject *parent):
    QZstdOptions{parent}
{

}

bool QZstdCompressor::execute()
{
    if (input().isEmpty() || output().isEmpty()) {
        setErrorString(QStringLiteral("Input or output pathways are unconfigured."));
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

bool QZstdCompressor::compressFolder()
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
    out.setVersion(headerVersion());

    // FIXME HOW OR WHY DOES NOT MATCH
    // Commit magic wrapper ID headers sequentially
    // QZstdOptions::magicDirString(); =  JOBZCRYPDIR1
    out << magicDirString(); //QStringLiteral("JOBZDIR1"); // Maybe I should store one for encrypted and one for not ?
    out << quint64(files.size());

    QDir const root(input());
    char buffer[65536];
    qint64 done = 0;

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
            qint64 const n = src.read(buffer, sizeof(buffer));
            if (n < 0) {
                setErrorString(src.errorString());
                zstd.close();
                return false;
            }
            if (n == 0) break;

            if (out.writeRawData(buffer, static_cast<int>(n)) != n) {
                setErrorString(QStringLiteral("Stream serialization write interruption occurred."));
                zstd.close();
                return false;
            }

            done += n;
            setCurrent(static_cast<int>(qMin<qint64>(done, INT_MAX)));
        }
    }

    zstd.close();
    Q_EMIT finished();
    return true;
}

bool QZstdCompressor::compressFile()
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

    // QZstdIO set the header and datastream to keep it up to snuff with the rest of the tools

    setTotal(static_cast<int>(qMin<qint64>(src.size(), INT_MAX)));
    setCurrent(0);

    char buffer[65536];
    qint64 done = 0;

    while (!src.atEnd()) {
        qint64 const n = src.read(buffer, sizeof(buffer));
        if (n < 0) {
            setErrorString(src.errorString());
            zstd.close();
            return false;
        }
        if (n == 0) break;

        if (zstd.write(buffer, n) != n) {
            setErrorString(zstd.errorString());
            zstd.close();
            return false;
        }

        done += n;
        setCurrent(static_cast<int>(qMin<qint64>(done, INT_MAX)));
    }

    zstd.close();
    Q_EMIT finished();
    return true;
}