#include "qsodiumcryptosign.h"

#include <string>
#include <filesystem>

bool QSodiumCryptoSign::signFile(const QString &filePath, QString &outSignatureBase64) noexcept
{
    std::string nativeSig;
    std::filesystem::path const nativePath = QFileInfo(filePath).filesystemFilePath();

    if (!job::crypto::JobCryptoSign::signFile(nativePath.string(), nativeSig))
        return false;

    outSignatureBase64 = QString::fromStdString(nativeSig);
    return true;
}

bool QSodiumCryptoSign::verifyFile(const QString &filePath, const QString &signatureBase64) noexcept
{
    std::filesystem::path const nativePath = QFileInfo(filePath).filesystemFilePath();
    return job::crypto::JobCryptoSign::verifyFile(nativePath.string(), signatureBase64.toStdString());
}

QString QSodiumCryptoSign::pubKey() const noexcept
{
    return QString::fromStdString(publicKey());
}

bool QSodiumCryptoSign::signAssociatedFile(const QString &filePath, QString &outSignatureBase64) noexcept
{
    std::string nativeSig;
    std::filesystem::path const nativePath = QFileInfo(filePath).filesystemFilePath();
    if (!job::crypto::JobCryptoSign::signAssociatedFile(nativePath.string(), nativeSig))
        return false;

    outSignatureBase64 = QString::fromStdString(nativeSig);
    return true;
}

bool QSodiumCryptoSign::loadKeys(const QString &pubName, const QString &priName) noexcept
{
    std::filesystem::path const pubPath = QFileInfo(pubName).filesystemFilePath();
    std::filesystem::path const priPath = QFileInfo(priName).filesystemFilePath();
    return loadKeysFromDisk(pubPath, priPath);
}

void QSodiumCryptoSign::setPublicKey(const QString &pubKey)
{
    job::crypto::JobCryptoSign::setPublicKey(pubKey.toStdString());
}

void QSodiumCryptoSign::addKeyDirectory(const QDir &dir)
{
    if(dir.exists()){
        std::filesystem::path const nativePath = QFileInfo(dir.absolutePath()).filesystemFilePath();
        job::crypto::JobCryptoSign::addKeyDirectory(nativePath);
    }
}

void QSodiumCryptoSign::addKeyDirectory(const QString &dir)
{
    QDir d(dir);
    addKeyDirectory(d);
}