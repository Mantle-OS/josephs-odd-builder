#ifndef QAISESSIONWORKER_H
#define QAISESSIONWORKER_H

#include <QString>
#include <qsecuremem.h>
#include <aisession/session_vault.hpp>
#include <aisession/session_user.hpp>

class QAiSessionWorker
{
public:
    [[nodiscard]] static bool compileAndLock(job::serializer::generated::AiSessionVault &vault,
                                             const QSecureMem &password,
                                             const QByteArray &salt,
                                             const QString &destinationPath) noexcept;

    [[nodiscard]] static bool unlockAndParse(job::serializer::generated::AiSessionVault &outVault,
                                             const QSecureMem &password,
                                             const QByteArray &salt,
                                             const QString &sourceVaultPath) noexcept;
};

#endif // QAISESSIONWORKER_H