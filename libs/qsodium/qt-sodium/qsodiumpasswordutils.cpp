#include "qsodiumpasswordutils.h"

#include <string>
#include <vector>

QString QSodiumPasswordUtils::hashPasswordForStorage(const QSecureMem &password) noexcept
{
    std::string out;
    if (!job::crypto::JobPasswordUtils::hashPasswordForStorage(password, out))
        return {};

    return QString::fromStdString(out);
}


bool QSodiumPasswordUtils::verifyPasswordAgainstStorage(const QSecureMem &password,
                                                        const QString &storedHash) noexcept
{
    return job::crypto::JobPasswordUtils::verifyPasswordAgainstStorage(
        password,
        storedHash.toStdString()
        );
}

bool QSodiumPasswordUtils::deriveKeyFromPassword(QSecureMem &outDerivedKey,
                                                 const QSecureMem &password,
                                                 const QByteArray &salt) noexcept
{
    // Map QByteArray over to a pure vector layout for standard bridge conversion
    std::vector<unsigned char> nativeSalt(salt.constData(), salt.constData() + salt.size());

    // Pass everything directly to the C++ core engine layer
    return job::crypto::JobPasswordUtils::deriveKeyFromPassword(outDerivedKey, password, nativeSalt);
}