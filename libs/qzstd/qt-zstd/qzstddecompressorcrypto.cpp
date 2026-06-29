#include "qzstddecompressorcrypto.h"
#include "qzstdio.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QDebug>
#include <sodium/crypto_secretbox.h>

QZstdDecompressorCrypto::QZstdDecompressorCrypto(QObject *parent) :
    QZstdDecompressor{parent}
{

}

QSecureMem QZstdDecompressorCrypto::decryptionKey() const
{
    return m_decryptionKey;
}

void QZstdDecompressorCrypto::setDecryptionKey(const QSecureMem &key)
{
    if (m_decryptionKey.data() != key.data()) {
        m_decryptionKey = key;
        Q_EMIT decryptionKeyChanged();
    }
}

void QZstdDecompressorCrypto::setDecryptionKeyB64(const QString &base64Key)
{
    QSecureMem decryptedKey;
    if (decryptedKey.fromBase64(base64Key)) {
        setDecryptionKey(decryptedKey);
    } else {
        // I hate my life backup ......
        // Fallback to checking raw text characters if Base64 decoding fails
        if (decryptedKey.allocate(crypto_secretbox_KEYBYTES)) {
            QByteArray rawBytes = base64Key.toUtf8().left(crypto_secretbox_KEYBYTES);
            std::memcpy(decryptedKey.data(), rawBytes.constData(), rawBytes.size());
            setDecryptionKey(decryptedKey);
        }
    }
}


bool QZstdDecompressorCrypto::execute()
{
    if (input().isEmpty() || output().isEmpty()) {
        setErrorString(QStringLiteral("Input or output pathways are completely unconfigured."));
        return false;
    }

    if (m_decryptionKey.isEmpty()) {
        setErrorString(QStringLiteral("Cryptographic pipeline missing secure decryption key context."));
        return false;
    }

    QFileInfo const srcInfo(input());
    if (!srcInfo.exists() || srcInfo.isDir()) {
        setErrorString(QStringLiteral("Source input archive file does not exist or is a directory path."));
        return false;
    }

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
    in.setVersion(QZstdOptions::headerVersion());

    QString magicTag;
    in >> magicTag;
    zstdDevice.close();
    probeFile.close();

    if (magicTag == QZstdOptions::magicDirString()) {
        return decompressFolder();
    } else if (magicTag == QZstdOptions::magicFileString()) {
        return decompressFile();
    }else{
        // BAIL ?
        setErrorString(QStringLiteral("No header in the peek of the execute"));
        return false;
    }

    setErrorString(QStringLiteral("The file target is unrecognized or lacks cryptographic header identifiers."));
    return false;
}

bool QZstdDecompressorCrypto::decompressFolder()
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

    setTotal(static_cast<int>(qMin<qint64>(srcFile.size(), INT_MAX)));
    setCurrent(0);

    QDataStream in(&zstdDevice);
    in.setVersion(QZstdOptions::headerVersion());

    QString magicTag;
    quint64 totalFiles = 0;

    in >> magicTag;

    // Double check header here ?
    in >> totalFiles;

    QDir baseDir(output());
    if (!baseDir.exists() && !baseDir.mkpath(baseDir.absolutePath())) {
        setErrorString(QStringLiteral("Failed to allocate extraction target folder destination."));
        zstdDevice.close();
        return false;
    }

    QSodiumSecretBox box;

    for (quint64 i = 0; i < totalFiles; ++i) {
        QString relativePath;
        quint64 fileLength = 0;

        in >> relativePath;
        in >> fileLength;

        QString const targetFilePath = baseDir.absoluteFilePath(relativePath);
        QFileInfo const targetInfo(targetFilePath);
        QDir const parentDir = targetInfo.absoluteDir();

        if (!parentDir.exists() && !parentDir.mkpath(parentDir.absolutePath())) {
            setErrorString(QStringLiteral("Failed to compose child layout directory structures."));
            zstdDevice.close();
            return false;
        }

        QFile targetFile(targetFilePath);
        if (!targetFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            setErrorString(targetFile.errorString());
            zstdDevice.close();
            return false;
        }

        quint64 bytesWrittenForFile = 0;
        while (bytesWrittenForFile < fileLength) {
            QByteArray chunkNonce;
            QByteArray encryptedChunk;

            in >> chunkNonce;
            in >> encryptedChunk;

            if (in.status() != QDataStream::Ok || encryptedChunk.isEmpty()) {
                setErrorString(QStringLiteral("Archive format payload stream broken or unexpected EOF reached."));
                targetFile.close();
                zstdDevice.close();
                return false;
            }

            QSecureMem plainChunk;
            if (!box.decrypt(encryptedChunk, m_decryptionKey, chunkNonce, plainChunk)) {
                setErrorString(QStringLiteral("Folder block decryption failed. MAC validation rejected the frame."));
                targetFile.close();
                zstdDevice.close();
                return false;
            }

            if (targetFile.write(reinterpret_cast<const char*>(plainChunk.data()), plainChunk.size()) != static_cast<qint64>(plainChunk.size())) {
                setErrorString(targetFile.errorString());
                targetFile.close();
                zstdDevice.close();
                return false;
            }

            // Securely advance uncompressed file progress tracking thresholds
            bytesWrittenForFile += plainChunk.size();
        }
        targetFile.close();
        setCurrent(static_cast<int>(qMin<qint64>(srcFile.pos(), INT_MAX)));
    }

    zstdDevice.close();
    Q_EMIT finished();
    return true;
}

bool QZstdDecompressorCrypto::decompressFile()
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

    setTotal(static_cast<int>(qMin<qint64>(srcFile.size(), INT_MAX)));
    setCurrent(0);

    QDataStream in(&zstdDevice);
    in.setVersion(QZstdOptions::headerVersion());

    QString magicTag;
    in >> magicTag; // Consume the "JOBZCRYPFILE1" tag

    QSodiumSecretBox box;

    while (!in.atEnd()) {
        QByteArray chunkNonce;
        QByteArray encryptedChunk;

        in >> chunkNonce;
        if (chunkNonce.isEmpty() && in.atEnd()) break;
        in >> encryptedChunk;

        QSecureMem plainChunk;
        if (!box.decrypt(encryptedChunk, m_decryptionKey, chunkNonce, plainChunk)) {
            setErrorString(QStringLiteral("File payload decryption failed. MAC checking validation mismatch."));
            dstFile.close();
            zstdDevice.close();
            return false;
        }

        if (dstFile.write(reinterpret_cast<const char*>(plainChunk.data()), plainChunk.size()) != static_cast<qint64>(plainChunk.size())) {
            setErrorString(dstFile.errorString());
            dstFile.close();
            zstdDevice.close();
            return false;
        }

        setCurrent(static_cast<int>(qMin<qint64>(srcFile.pos(), INT_MAX)));
    }

    zstdDevice.close();
    dstFile.close();

    Q_EMIT finished();
    return true;
}