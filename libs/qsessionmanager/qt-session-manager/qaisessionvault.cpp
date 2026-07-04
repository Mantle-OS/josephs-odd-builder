#include "qaisessionvault.h"
#include <sodium.h>
#include <QDebug>

using namespace job::serializer::generated;

QAiSessionVault::QAiSessionVault(QObject *parent) :
    QObject{parent}
{
}

QAiSessionVault::~QAiSessionVault() noexcept
{
    clear();
}

void QAiSessionVault::clear() noexcept
{
    m_secureKeys.clear();
    m_secureSecrets.clear();

    m_metaKeys.clear();
    m_metaCredentials.clear();
    m_metaProviders.clear();

    m_userId.clear();
    m_displayName.clear();
    m_createdAt.clear();
    m_updatedAt.clear();
}

bool QAiSessionVault::populateAndScrub(AiSessionVault &transientVault) noexcept
{
    clear();

    m_schemaVersion = transientVault.schema_version;
    m_userId        = QString::fromStdString(transientVault.user_id);
    m_displayName   = QString::fromStdString(transientVault.display_name);
    m_createdAt     = QString::fromStdString(transientVault.created_at);
    m_updatedAt     = QString::fromStdString(transientVault.updated_at);

    // Process Keys while retaining structural identities
    for (auto &keyItem : transientVault.keys) {
        if (!keyItem.secret_key.empty() && keyItem.enabled) {
            // Use composite key masking to block functional overlaps
            QString const compositeKey = QString("%1:%2").arg(keyItem.kind).arg(QString::fromStdString(keyItem.key_id));

            QSecureMem secureKeyContainer(keyItem.secret_key.size());
            secureKeyContainer.copyFrom(keyItem.secret_key.data(), keyItem.secret_key.size());
            m_secureKeys.insert(compositeKey, secureKeyContainer);

            // Scrub physical process RAM footprints instantly
            sodium_memzero(keyItem.secret_key.data(), keyItem.secret_key.size());
        }

        // Retain the metadata item layout intact, missing only the raw volatile secret bytes
        keyItem.secret_key.clear();
        m_metaKeys.append(keyItem);
    }

    // Process Credentials securely
    for (auto &credItem : transientVault.credentials) {
        if (!credItem.secret.empty() && credItem.enabled) {
            QString const cid = QString::fromStdString(credItem.credential_id);

            QSecureMem secureSecretContainer(credItem.secret.size());
            secureSecretContainer.copyFrom(credItem.secret.data(), credItem.secret.size());
            m_secureSecrets.insert(cid, secureSecretContainer);

            sodium_memzero(credItem.secret.data(), credItem.secret.size());
        }

        credItem.secret.clear();
        m_metaCredentials.append(credItem);
    }

    // 3. Mirror operational providers metadata cleanly
    for (const auto &provItem : transientVault.providers) {
        m_metaProviders.append(provItem);
    }

    qDebug() << "[+] SessionVault: Safely unboxed identity parameters without metadata loss.";
    return true;
}

AiSessionVault QAiSessionVault::exportToGenerated() const
{
    AiSessionVault tv;
    tv.schema_version = m_schemaVersion;
    tv.user_id        = m_userId.toStdString();
    tv.display_name   = m_displayName.toStdString();
    tv.created_at     = m_createdAt.toStdString();
    tv.updated_at     = m_updatedAt.toStdString();

    // Rehydrate keys list cleanly using zero-copy iterator lookups
    for (const auto &metaKey : m_metaKeys) {
        AiSessionKey k = metaKey;
        QString const compositeKey = QString("%1:%2").arg(k.kind).arg(QString::fromStdString(k.key_id));

        auto it = m_secureKeys.constFind(compositeKey);
        if (it != m_secureKeys.constEnd()) {
            const QSecureMem &sec = it.value();
            k.secret_key.assign(sec.data(), sec.data() + sec.size());
        }
        tv.keys.push_back(k);
    }

    // Rehydrate credentials list cleanly using zero-copy iterator lookups
    for (const auto &metaCred : m_metaCredentials) {
        AiSessionCredential c = metaCred;
        QString const cid = QString::fromStdString(c.credential_id);

        auto it = m_secureSecrets.constFind(cid);
        if (it != m_secureSecrets.constEnd()) {
            const QSecureMem &sec = it.value();
            c.secret.assign(sec.data(), sec.data() + sec.size());
        }
        tv.credentials.push_back(c);
    }

    for (const auto &metaProv : m_metaProviders)
        tv.providers.push_back(metaProv);

    return tv;
}

QString QAiSessionVault::getProviderCredentialId(QAi::Provider providerType,
                                                 const QString &accountName) const noexcept
{
    const uint32_t pType = static_cast<uint32_t>(providerType);
    const QString requested = accountName.trimmed().toLower();

    QString firstEnabledId;
    QString defaultId;

    for (const auto &prov : m_metaProviders) {
        if (prov.provider != pType || !prov.enabled)
            continue;

        const QString currentAccount = QString::fromStdString(prov.account_name).toLower();
        const QString credentialId = QString::fromStdString(prov.credential_id);

        if (currentAccount == requested)
            return credentialId;

        if (firstEnabledId.isEmpty())
            firstEnabledId = credentialId;

        if (currentAccount == QStringLiteral("default"))
            defaultId = credentialId;
    }

    return !defaultId.isEmpty() ? defaultId : firstEnabledId;
}

QSecureMem QAiSessionVault::getSecretKey(QAi::KeyKind kind, const QString &keyId) const noexcept
{
    QString const compositeKey = QString("%1:%2").arg(static_cast<uint32_t>(kind)).arg(keyId);
    return m_secureKeys.value(compositeKey);
}

QSecureMem QAiSessionVault::getCredentialSecret(const QString &credentialId) const noexcept
{
    return m_secureSecrets.value(credentialId);
}




