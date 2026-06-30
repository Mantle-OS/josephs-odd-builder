#ifndef QAIUSERSESSION_H
#define QAIUSERSESSION_H
#include <QObject>
#include <QHash>

#include <qsecuremem.h>

#include <aisession/session_vault.hpp>

using namespace job::serializer::generated;

class QAiUserSession : public QObject
{
    Q_OBJECT

public:
    explicit QAiUserSession(QObject *parent = nullptr);
    ~QAiUserSession() noexcept override;

    bool loadFromVault(const AiSessionVault &vault,
                       const QSecureMem &vaultKey);

    void clear() noexcept;

    QSecureMem *credential(uint32_t provider, const QString &name);
    QSecureMem *key(uint32_t kind, const QString &keyId = {});

    const AiSessionVault &vaultData() const noexcept;

private:
    AiSessionVault m_vaultData;
    QSecureMem *m_vaultKey = nullptr;

    // maybe first pass:
    QHash<QString, QSecureMem *> m_credentials;
    QHash<QString, QSecureMem *> m_keys;
};

#endif // QAIUSERSESSION_H
