#include "qaisessionworker.h"
#include <qsodium.h>
#include <qsodiumpasswordutils.h>
#include <qsodiumsecretbox.h>
#include <qzstd.h>
#include <qzstdio.h>
#include <qaiutils.h>
#include <msgpack.hpp>
#include <sodium.h>
#include <QBuffer>
#include <QFile>
#include <QDebug>

using namespace job::serializer::generated;

bool QAiSessionWorker::compileAndLock(AiSessionVault &vault,
                                      const QSecureMem &password,
                                      const QByteArray &salt,
                                      const QString &destinationPath) noexcept
{
    QSecureMem derivedVaultKey;

    if (!QSodiumPasswordUtils::deriveKeyFromPassword(derivedVaultKey, password, salt)) {
        qCritical() << "[-] VaultWorker: Key derivation processing for compilation targets exploded.";
        return false;
    }

    msgpack::sbuffer serializationBuffer;
    msgpack::packer<msgpack::sbuffer> packer(&serializationBuffer);
    vault.pack_msgpack(packer);

    QByteArray rawMessagePackBytes(serializationBuffer.data(), static_cast<int>(serializationBuffer.size()));

    QByteArray compressedBytes;
    QBuffer compressedBuffer(&compressedBytes);
    compressedBuffer.open(QIODevice::WriteOnly);

    QZstdIO zstdWriter(&compressedBuffer);
    zstdWriter.setCompressionLevel(9); // High efficiency tuning pass for small structured assets
    if (!zstdWriter.open(QIODevice::WriteOnly)) {
        qCritical() << "[-] VaultWorker: QZstd wrapper engine compression initialization failed.";
        return false;
    }

    zstdWriter.write(rawMessagePackBytes.constData(), rawMessagePackBytes.size());
    zstdWriter.close();
    compressedBuffer.close();

    QByteArray cipherText;
    QByteArray nonce;
    if (!QSodium::encryptConfig(compressedBytes, derivedVaultKey, cipherText, nonce)) {
        qCritical() << "[-] VaultWorker: Symmetric encryption execution rejected the archive.";
        return false;
    }

    QFile outVaultFile(destinationPath);
    if (!outVaultFile.open(QIODevice::WriteOnly)) {
        qCritical() << "[-] VaultWorker: Unable to open vault path target for output stream updates:" << destinationPath;
        return false;
    }

    // [4 bytes nonce size][Nonce Bytes][Ciphertext Payload Array]
    uint32_t const nonceSize = static_cast<uint32_t>(nonce.size());
    outVaultFile.write(reinterpret_cast<const char*>(&nonceSize), sizeof(nonceSize));
    outVaultFile.write(nonce.constData(), nonce.size());
    outVaultFile.write(cipherText.constData(), cipherText.size());
    outVaultFile.flush();
    outVaultFile.close();

    qDebug() << "[+] VaultWorker: Encrypted archive compiled successfully onto disk channel.";
    return true;
}

bool QAiSessionWorker::unlockAndParse(AiSessionVault &outVault,
                                      const QSecureMem &password,
                                      const QByteArray &salt,
                                      const QString &sourceVaultPath) noexcept
{
    QFile inVaultFile(sourceVaultPath);
    if (!inVaultFile.open(QIODevice::ReadOnly)) {
        qCritical() << "[-] VaultWorker: Failed to read target file path configuration:" << sourceVaultPath;
        return false;
    }

    uint32_t nonceSize = 0;
    if (inVaultFile.read(reinterpret_cast<char*>(&nonceSize), sizeof(nonceSize)) != sizeof(nonceSize)) {
        return false;
    }

    QByteArray const nonce = inVaultFile.read(static_cast<qint64>(nonceSize));
    QByteArray const cipherText = inVaultFile.readAll();
    inVaultFile.close();

    QSecureMem derivedVaultKey;

    if (!QSodiumPasswordUtils::deriveKeyFromPassword(derivedVaultKey, password, salt))
        return false;

    QSecureMem decryptedCompressedPayload;
    if (!QSodium::decryptConfig(cipherText, derivedVaultKey, nonce, decryptedCompressedPayload)) {
        qWarning() << "[-] VaultWorker: Cryptographic signature MAC validation check failed. Invalid password.";
        return false;
    }

    QByteArray compressedBytes(reinterpret_cast<const char*>(decryptedCompressedPayload.data()),
                               static_cast<int>(decryptedCompressedPayload.size()));

    QBuffer compressedInputBuffer(&compressedBytes);
    compressedInputBuffer.open(QIODevice::ReadOnly);

    QZstdIO zstdReader(&compressedInputBuffer);
    if (!zstdReader.open(QIODevice::ReadOnly))
        return false;


    QByteArray uncompressedMsgPackBytes;
    char buffer[4096];
    while (!zstdReader.atEnd()) {
        qint64 const readBytes = zstdReader.read(buffer, sizeof(buffer));
        if (readBytes <= 0)
            break;
        uncompressedMsgPackBytes.append(buffer, static_cast<int>(readBytes));
    }
    zstdReader.close();
    compressedInputBuffer.close();

    try {
        msgpack::object_handle oh = msgpack::unpack(uncompressedMsgPackBytes.constData(),
                                                    uncompressedMsgPackBytes.size());
        msgpack::object const obj = oh.get();
        outVault.unpack_msgpack(obj);
    }
    catch (const std::exception &e) {
        qCritical() << "[-] VaultWorker: Binary structure unpacker step encountered corruption error ->" << e.what();
        return false;
    }

    return true;
}