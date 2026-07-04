#include "qaiusersession.h"
#include <sodium.h>
#include <QDebug>

using namespace job::serializer::generated;

QAiUserSession::QAiUserSession(QObject *parent)
    : QObject{parent}
{
}

QAiUserSession::~QAiUserSession() noexcept
{
    clear();
}

void QAiUserSession::clear() noexcept
{
    m_credentials.clear();
    m_keys.clear();
    m_vaultKey.free();

    m_vaultData.user_id.clear();
    m_vaultData.display_name.clear();
    m_vaultData.keys.clear();
    m_vaultData.credentials.clear();
    m_vaultData.providers.clear();
}

bool QAiUserSession::loadFromVault(AiSessionVault &vault, const QSecureMem &vaultKey)
{
    clear();

    m_vaultData = vault;
    m_vaultKey = vaultKey; // Deep copy maintains lock parameters ???

    for (auto &keyItem : vault.keys) {
        if (!keyItem.enabled)
            continue;

        QString const compositeKey = QString("%1_%2").arg(keyItem.kind).arg(QString::fromStdString(keyItem.key_id));

        QSecureMem secKey(keyItem.secret_key.size());
        secKey.copyFrom(keyItem.secret_key.data(), keyItem.secret_key.size());
        m_keys.insert(compositeKey, secKey);

        // Scrub source buffer footprint out of standard RAM
        sodium_memzero(keyItem.secret_key.data(), keyItem.secret_key.size());
        keyItem.secret_key.clear();
    }

    for (auto &credItem : vault.credentials) {
        if (!credItem.enabled)
            continue;

        QString const compositeKey = QString("%1_%2").arg(credItem.provider).arg(QString::fromStdString(credItem.name));

        QSecureMem secCred(credItem.secret.size());
        secCred.copyFrom(credItem.secret.data(), credItem.secret.size());
        m_credentials.insert(compositeKey, secCred);

        sodium_memzero(credItem.secret.data(), credItem.secret.size());
        credItem.secret.clear();
    }

    qDebug() << "[+] UserSession: Loaded operational session state for UID:" << QString::fromStdString(m_vaultData.user_id);
    return true;
}

// QSecureMem QAiUserSession::credential(QAi::Provider provider, const QString &name) const noexcept
// {
//     // Let the vault do the heavy lifting of matching or falling back
//     const QString credentialId = m_vaultData.getProviderCredentialId(provider, name);
//     if (credentialId.isEmpty()) {
//         qWarning() << "[-] UserSession: Failed to resolve credential routing for provider:"
//                    << static_cast<uint32_t>(provider) << "profile:" << name;
//         return QSecureMem{};
//     }

//     // Direct zero-copy iterator extraction out of our page-locked hash map
//     auto it = m_credentials.constFind(QString("%1_%2").arg(static_cast<uint32_t>(provider)).arg(credentialId));
//     if (it != m_credentials.constEnd()) {
//         return it.value();
//     }

//     return QSecureMem{};
// }

QSecureMem QAiUserSession::credential(QAi::Provider provider, const QString &name) const noexcept
{
    const uint32_t providerType = static_cast<uint32_t>(provider);
    QString targetCredentialId;

    // If an explicit profile name is provided, search the unpacked providers list for a match
    if (!name.isEmpty() && name.toLower() != "default") {
        for (const auto &provItem : m_vaultData.providers) {
            if (provItem.provider == providerType && QString::fromStdString(provItem.account_name) == name) {
                if (provItem.enabled) {
                    targetCredentialId = QString::fromStdString(provItem.credential_id);
                    break;
                }
            }
        }
    }

    // Fallback: If no explicit match was found, resolve via the default routing configuration
    if (targetCredentialId.isEmpty()) {
        for (const auto &provItem : m_vaultData.providers) {
            if (provItem.provider == providerType) {
                // If we find the explicit "default" account name, or simply the first enabled instance
                if (provItem.enabled && (targetCredentialId.isEmpty() || QString::fromStdString(provItem.account_name).toLower() == "default")) {
                    targetCredentialId = QString::fromStdString(provItem.credential_id);
                    if (QString::fromStdString(provItem.account_name).toLower() == "default") {
                        break; // Exact fallback match located
                    }
                }
            }
        }
    }

    if (targetCredentialId.isEmpty()) {
        qWarning() << "[-] UserSession: Failed to resolve credential routing for provider:" << provider << "profile:" << name;
        return QSecureMem{};
    }

    // The key format matches how we packed it in loadFromVault: "%1_%2" -> (provider_credentialId)
    QString const compositeKey = QString("%1_%2").arg(providerType).arg(targetCredentialId);
    return m_credentials.value(compositeKey);
}

QSecureMem QAiUserSession::key(QAi::KeyKind kind, const QString &keyId) const noexcept
{
    const QString compositeKey = QString("%1_%2").arg(static_cast<uint32_t>(kind)).arg(keyId);
    return m_keys.value(compositeKey);
}

const AiSessionVault& QAiUserSession::vaultData() const noexcept
{
    return m_vaultData;
}