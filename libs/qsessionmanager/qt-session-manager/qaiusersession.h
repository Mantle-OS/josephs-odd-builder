#ifndef QAIUSERSESSION_H
#define QAIUSERSESSION_H

#include <QObject>
#include <QHash>
#include <QString>

#include <qsecuremem.h>
#include <aisession/session_vault.hpp>
#include "qaitypes.h"
class QAiUserSession : public QObject
{
    Q_OBJECT

public:
    explicit QAiUserSession(QObject *parent = nullptr);
    ~QAiUserSession() noexcept override;

    QAiUserSession(const QAiUserSession &) = delete;
    QAiUserSession &operator=(const QAiUserSession &) = delete;

    bool loadFromVault(job::serializer::generated::AiSessionVault &vault,
                       const QSecureMem &vaultKey);

    void clear() noexcept;

    [[nodiscard]] bool isActive() const noexcept
    {
        return !m_vaultData.user_id.empty() && (!m_keys.isEmpty() || !m_credentials.isEmpty());
    }

    // Returns a copy of the secure container handle (retaining the protected page lock)
    [[nodiscard]] QSecureMem credential(QAi::Provider provider, const QString &name) const noexcept;
    [[nodiscard]] QSecureMem key(QAi::KeyKind kind, const QString &keyId = {}) const noexcept;

    [[nodiscard]] const job::serializer::generated::AiSessionVault &vaultData() const noexcept;

private:
    job::serializer::generated::AiSessionVault m_vaultData;
    QSecureMem                                 m_vaultKey;

    QHash<QString, QSecureMem>                 m_credentials; // key: provider_name (e.g. "0_default")
    QHash<QString, QSecureMem>                 m_keys;        // key: kind_keyId (e.g. "1_master-dev")
};

#endif // QAIUSERSESSION_H