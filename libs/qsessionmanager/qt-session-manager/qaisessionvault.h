#ifndef QAISESSIONVAULT_H
#define QAISESSIONVAULT_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QList>

#include <qsecuremem.h>
#include <qaitypes.h>

// Bring in the full generated layout definitions so we can mirror their structural metadata
#include <aisession/session_key.hpp>
#include <aisession/session_credential.hpp>
#include <aisession/session_provider.hpp>
#include <aisession/session_vault.hpp>

class QAiSessionVault : public QObject
{
    Q_OBJECT

public:
    explicit QAiSessionVault(QObject *parent = nullptr);
    ~QAiSessionVault() noexcept override;

    QAiSessionVault(const QAiSessionVault &) = delete;
    QAiSessionVault &operator=(const QAiSessionVault &) = delete;


    bool populateAndScrub(job::serializer::generated::AiSessionVault &transientVault) noexcept;

    [[nodiscard]] job::serializer::generated::AiSessionVault exportToGenerated() const;

    [[nodiscard]] QString userId() const noexcept { return m_userId; }
    [[nodiscard]] QString displayName() const noexcept { return m_displayName; }

    // Enhanced lookups tracking type safety namespaces explicitly
    [[nodiscard]] QSecureMem getSecretKey(QAi::KeyKind kind, const QString &keyId) const noexcept;
    [[nodiscard]] QSecureMem getCredentialSecret(const QString &credentialId) const noexcept;

    [[nodiscard]] QString getProviderCredentialId(QAi::Provider providerType,
                                                  const QString &accountName = "default") const noexcept;
    void clear() noexcept;

private:
    QString m_userId;
    QString m_displayName;
    uint32_t m_schemaVersion{1};
    QString m_createdAt;
    QString m_updatedAt;

    // Cryptographically isolated storage tables
    QHash<QString, QSecureMem> m_secureKeys;       // Key layout: "kind:key_id" -> private key page lock
    QHash<QString, QSecureMem> m_secureSecrets;    // Key layout: "credential_id" -> token page lock

    // Full, structural, non-secret metadata mirrors to prevent down-cycle data loss
    QList<job::serializer::generated::AiSessionKey>        m_metaKeys;
    QList<job::serializer::generated::AiSessionCredential> m_metaCredentials;
    QList<job::serializer::generated::AiSessionProvider>   m_metaProviders;
};

#endif // QAISESSIONVAULT_H