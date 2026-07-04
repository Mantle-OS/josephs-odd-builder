#include "qsodiumcryptosign.h"

#include <string>

bool QSodiumCryptoSign::signFile(const QString &filePath, QString &outSignatureBase64) noexcept
{
    std::string nativeSig;
    if (!job::crypto::JobCryptoSign::signFile(filePath.toStdString(), nativeSig))
        return false;

    outSignatureBase64 = QString::fromStdString(nativeSig);
    return true;
}

bool QSodiumCryptoSign::verifyFile(const QString &filePath, const QString &signatureBase64) noexcept
{
    return job::crypto::JobCryptoSign::verifyFile(filePath.toStdString(), signatureBase64.toStdString());
}