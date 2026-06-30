#ifndef QAISESSIONVAULT_H
#define QAISESSIONVAULT_H

#include <QObject>

#include <qsecuremem.h>

#include <aisession/session_vault.hpp>
using namespace job::serializer::generated;
class QAiSessionVault : public QObject
{
    Q_OBJECT

public:

    explicit QAiSessionVault(QObject *parent = nullptr);

    bool createVault(const QString &vaultPath,
                     const AiSessionVault &vault,
                     const QSecureMem &vaultKey,
                     QString *errorString = nullptr);

    bool openVault(const QString &vaultPath,
                   const QSecureMem &vaultKey,
                   AiSessionVault *outVault,
                   QString *errorString = nullptr);

    bool saveVaultAtomic(const QString &vaultPath,
                         const AiSessionVault &vault,
                         const QSecureMem &vaultKey,
                         QString *errorString = nullptr);
};

#endif // QAISESSIONVAULT_H
