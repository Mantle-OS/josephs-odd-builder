#include "huggingfaceusermanager.h"
#include <QDebug>

HuggingFaceUserManager::HuggingFaceUserManager(QObject *parent) :
    QObject{parent},
    m_anonymousUser{new HuggingFaceUser{this}},
    m_currentUser{nullptr},
    m_users{new ObjectListModel<HuggingFaceUser>{this, "display", "uid"}}
{
    m_anonymousUser->set_uid(QStringLiteral("anonymous"));
    m_anonymousUser->set_authDisplayName(QStringLiteral("anonymous"));
    m_anonymousUser->set_isLoggedIn(true);

    m_users->append(m_anonymousUser);
    m_currentUser = m_anonymousUser;
}

HuggingFaceUserManager::~HuggingFaceUserManager()
{
    m_users->clear();
    delete m_users;
}

void HuggingFaceUserManager::syncFromGlobalSession(const QAiUserSession *globalSession) noexcept
{
    if (!globalSession || !globalSession->isActive()) {
        useAnonymous();
        return;
    }

    // Flush existing accounts except for anonymous placeholder
    m_users->clear();
    m_users->append(m_anonymousUser);

    // Iterate through the structural non-secret vault definitions metadata mirrored from the global session
    const auto &vaultData = globalSession->vaultData();
    for (const auto &prov : vaultData.providers) {
        if (prov.provider == static_cast<uint32_t>(QAi::Provider::HuggingFace) && prov.enabled) {

            HuggingFaceUser *hfProfile = new HuggingFaceUser(this);
            QString const name = QString::fromStdString(prov.account_name);

            hfProfile->set_uid(QString::fromStdString(prov.credential_id));
            hfProfile->set_authDisplayName(name); // "corp", "personal", etc.
            hfProfile->set_isLoggedIn(false);

            m_users->append(hfProfile);
        }
    }
}

void HuggingFaceUserManager::login(HuggingFaceApi *api, const QString &accountName)
{
    if (!api) return;

    HuggingFaceUser *targetUser = nullptr;
    for (auto *user : m_users->toList()) {
        if (user->get_authDisplayName().toLower() == accountName.trimmed().toLower()) {
            targetUser = user;
            break;
        }
    }

    if (!targetUser || targetUser->get_uid() == "anonymous") {
        useAnonymous();
        api->setSecureSession(nullptr); // Clears interception hooks

        api->login().then([this]([[maybe_unused]] QJsonObject obj) {
            Q_EMIT loggedIn(true);
        });
        return;
    }

    // Pass configuration descriptors down to the interceptor tracking reference
    // This hooks up our zero-copy virtual prepareSecureHeaders method!
    api->setSecureSession(api->findChild<QAiUserSession*>(), targetUser->get_authDisplayName());

    // Execute whoami validation verification call
    api->login().then([this, targetUser](QJsonObject obj) {
                    if (obj.contains("error") || !obj.contains("id")) {
                        qWarning() << "[UserManager] Token validation rejected by Hugging Face API routes.";
                        targetUser->set_isLoggedIn(false);
                        Q_EMIT loggedIn(false);
                        return;
                    }

                    targetUser->fromJson(obj);
                    targetUser->set_isLoggedIn(true);
                    set_currentUser(targetUser);

                    Q_EMIT loggedIn(true);
                }).onFailed([this, targetUser]() {
            targetUser->set_isLoggedIn(false);
            Q_EMIT loggedIn(false);
        });
}

void HuggingFaceUserManager::useAnonymous()
{
    if (m_currentUser && m_currentUser->get_uid() != "anonymous") {
        m_currentUser->set_isLoggedIn(false);
    }
    set_currentUser(m_anonymousUser);
    m_anonymousUser->set_isLoggedIn(true);
    Q_EMIT loggedIn(true);
}