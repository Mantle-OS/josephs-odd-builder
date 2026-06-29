#ifndef HUGGINGFACEUSERMANAGER_H
#define HUGGINGFACEUSERMANAGER_H

#include <QObject>
#include <QQmlEngine>

#include <pointer-macros.h>
#include <objectmodel.h>

#include <qsodium.h>


#include "huggingfaceuser.h"


class HuggingFaceUserManager : public QObject
{
    Q_OBJECT

    QP_PTR_RO(HuggingFaceUser, anonymousUser)   // owned
    QP_PTR_RO(HuggingFaceUser, currentUser)     // owned in the model
    QP_PTR_RO(ObjectListModel<HuggingFaceUser>, users)
    QP_RO(bool, keyInEnv, false)
    QML_ELEMENT
    QML_UNCREATABLE("Please use HuggingFaceHub.userManager" )
public:
    explicit HuggingFaceUserManager(QObject *parent = nullptr) :
        m_anonymousUser{new HuggingFaceUser{this}},
        m_currentUser{nullptr},
        m_users{new ObjectListModel<HuggingFaceUser>{this, "display", "uid"}},
        QObject{parent}
    {
        m_anonymousUser->set_uid("anonymous");
        m_anonymousUser->set_authDisplayName("anonymous");
        m_anonymousUser->set_isLoggedIn(true);
        m_users->append(m_anonymousUser);
        m_currentUser = m_users->getByUid("anonymous");
        QString currentToken = qEnvironmentVariable("HF_TOKEN", "NONE");
        if(currentToken == "NONE"){
            set_keyInEnv(false);
        }else{
            set_keyInEnv(true);
        }
    }

    ~HuggingFaceUserManager()
    {
        if(!m_users->isEmpty()){

            for(auto *user : m_users->toList()){
                if(user->get_isLoggedIn()){
                    user->set_isLoggedIn(false);
                }
            }
            m_users->clear();
        }
        if(m_users){
            delete m_users;
            m_users = nullptr;
        }
    }

    void login(HuggingFaceApi *api, const QString &userPassword)
    {
        if (!m_currentUser) return;

        // Bypass check: If the user is logging in anonymously, skip decryption
        if (m_currentUser->get_uid() == "anonymous") {
            api->set_anonymous(true);
            api->set_token("NONE");

            api->login().then([this](QJsonObject obj) {
                Q_EMIT loggedIn(true);
            });
            return;
        }

        // Extract local storage crypt parameters from the active profile
        QByteArray const cipherBin = QByteArray::fromBase64(m_currentUser->get_encryptedTokenB64().toLatin1());
        QByteArray const nonceBin  = QByteArray::fromBase64(m_currentUser->get_cryptoNonceB64().toLatin1());
        QByteArray const saltBin   = QByteArray::fromBase64(m_currentUser->get_cryptoSaltB64().toLatin1());

        if (cipherBin.isEmpty() || nonceBin.isEmpty() || saltBin.isEmpty()) {
            qWarning() << "[UserManager] Cryptographic parameters are missing or unconfigured.";
            Q_EMIT loggedIn(false);
            return;
        }

        // Derive the master session key inside kernel-locked RAM spaces
        QSecureMem masterSessionKey;
        if (!QSodiumPasswordUtils::deriveKeyFromPassword(masterSessionKey, userPassword, saltBin)) {
            qCritical() << "[UserManager] Master session derivation key failed.";
            Q_EMIT loggedIn(false);
            return;
        }

        // 4. Decrypt the persistent token into isolated binary rows
        QSecureMem cleartextToken;
        if (!QSodium::decryptConfig(cipherBin, masterSessionKey, nonceBin, cleartextToken)) {
            qCritical() << "[UserManager] Authentication failed. Invalid master token signature.";
            Q_EMIT loggedIn(false);
            return;
        }

        // 5. Inject the transient cleartext token directly into the base ApiClient context string
        QString transientToken = QString::fromUtf8(reinterpret_cast<const char*>(cleartextToken.data()), cleartextToken.size());

        api->set_anonymous(false);
        api->set_token(transientToken);

        // CRITICAL: Immediately wipe the stack variables to shrink the leak window
        transientToken.fill('\0');
        cleartextToken.free();
        masterSessionKey.free();

        // 6. Fire the online "whoami-v2" validation pipeline pass
        api->login().then([this, api](QJsonObject obj) {
                        // Eviction Pass: Securely scrub the token string out of the ApiClient context the instant the roundtrip completes!
                        api->set_token("NONE");

                        if (obj.contains("error") || !obj.contains("id")) {
                            qWarning() << "[UserManager] Online verification rejected by backend hub API routes.";
                            m_currentUser->set_isLoggedIn(false);
                            Q_EMIT loggedIn(false);
                            return;
                        }

                        // Hydrate profile data models from verified server payload info
                        m_currentUser->fromJson(obj);
                        m_currentUser->set_isLoggedIn(true);

                        set_currentUser(m_currentUser);
                        Q_EMIT loggedIn(true);
                    }).onFailed([this, api]() {
                // Eviction Fallback: Ensure token is purged even on network exceptions or timeout faults
                api->set_token("NONE");
                m_currentUser->set_isLoggedIn(false);
                Q_EMIT loggedIn(false);
            });
    }
    void useAnonymous()
    {
        if(m_currentUser->get_uid() != "anonymous"){
            if(m_currentUser->get_isLoggedIn())
                m_currentUser->set_isLoggedIn(false);
                //
        }
        set_currentUser(m_anonymousUser);
        Q_EMIT loggedIn(true);
    }

Q_SIGNALS:
    void loggedIn(bool);

};

#endif // HUGGINGFACEUSERMANAGER_H
