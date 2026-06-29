#ifndef HUGGINGFACEUSER_H
#define HUGGINGFACEUSER_H

#include <QObject>
#include <QQmlEngine>

#include <property-macros.h>

#include "huggingfaceapi.h"

class HuggingFaceUser : public QObject
{
    Q_OBJECT
    // auth json obj
    QP_RO(QString,      authDisplayName,        ""      )
    QP_RO(QString,      authRoles,              ""      )
    QP_RO(QString,      authCreated,          ""        )

    QP_RO(QString,      authExpiresAt,          ""      )

    QP_RO(bool,         authCanReadGatedRepos, false    )

    QP_RO(QString,      uid,                    ""      )
    QP_RO(QString,      uname,                  ""      )
    QP_RO(QString,      fullName,               ""      )
    QP_RO(QString,      email,                  ""      )
    QP_RO(QString,      avatarUrl,              ""      )
    QP_RO(bool,         emailVerified,          false   )
    QP_RO(bool,         isLoggedIn,             false   )


    // NEW: Locked local crypto envelope parameters (Safe to serialize out as text configs)
    QP_RW(QString, encryptedTokenB64, "")
    QP_RW(QString, cryptoNonceB64,    "")
    QP_RW(QString, cryptoSaltB64,     "")

    QML_ELEMENT
public:
    explicit HuggingFaceUser(QObject *parent = nullptr);
    ~HuggingFaceUser() = default;

    void fromJson(const QJsonObject obj)
    {
        if(obj.contains("id") && obj.contains("auth")){
            QJsonObject authObj = obj["auth"].toObject();
            if(authObj.contains("accessToken")){
                QJsonObject accTokObj = authObj["accessToken"].toObject();
                if(accTokObj.contains("displayName"))
                    set_authDisplayName(accTokObj["displayName"].toString());

                if(accTokObj.contains("role"))
                    set_authRoles(accTokObj["role"].toString());

                if(accTokObj.contains("createdAt"))
                    set_authCreated(accTokObj["createdAt"].toString());

            }
            if(authObj.contains("expiresAt"))
                set_authExpiresAt(authObj["expiresAt"].toString());

            set_uid(obj["id"].toString());

            if(obj.contains("name"))
                set_uname(obj["name"].toString());

            if(obj.contains("fullname"))
                set_fullName(obj["fullname"].toString());

            if(obj.contains("email"))
                set_email(obj["email"].toString());

            if(obj.contains("avatarUrl"))
                set_avatarUrl(obj["avatarUrl"].toString());

            if(obj.contains("emailVerified"))
                set_emailVerified( obj["emailVerified"].toBool());

            // Pull encrypted credentials from local config mirror slots
            if (obj.contains("encryptedTokenB64")) set_encryptedTokenB64(obj["encryptedTokenB64"].toString());
            if (obj.contains("cryptoNonceB64"))    set_cryptoNonceB64(obj["cryptoNonceB64"].toString());
            if (obj.contains("cryptoSaltB64"))     set_cryptoSaltB64(obj["cryptoSaltB64"].toString());
        }
    }

    QJsonObject toJson()
    {
        QJsonObject ret{};
        QJsonObject authObj = ret["auth"].toObject();

        QJsonObject accTokObj =  authObj["accessToken"].toObject();
        accTokObj["displayName"] = get_authDisplayName();
        accTokObj["role"]= get_authRoles();
        accTokObj["createdAt"] = get_authCreated();
        authObj["accessToken"] = accTokObj;
        authObj["expiresAt"] = get_authExpiresAt();
        ret["auth"] = authObj;

        ret["id"] = get_uid();
        ret["name"] = get_uname();
        ret["fullname"] = get_fullName();
        ret["email"] = get_email();
        ret["avatarUrl"] = get_avatarUrl();
        ret["emailVerified"] = get_emailVerified();

        ret["encryptedTokenB64"] = get_encryptedTokenB64();
        ret["cryptoNonceB64"] = get_cryptoNonceB64();
        ret["cryptoSaltB64"] = get_cryptoSaltB64();
        return ret;
    }

};

#endif // HUGGINGFACEUSER_H
