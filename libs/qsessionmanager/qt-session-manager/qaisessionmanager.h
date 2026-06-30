#ifndef QAISESSIONMANAGER_H
#define QAISESSIONMANAGER_H

#include <QObject>
#include <QFuture>
#include <QJsonObject>
#include <QJsonArray>

#include <qaiutils.h>
#include <property-macros.h>

#include "qaiusersession.h"
#include "qaisessionvault.h"
class QAiSessionManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAiSessionState sessionState READ sessionState NOTIFY sessionStateChanged FINAL)
    QP_RO(bool, hasUsers, false)
public:
    explicit QAiSessionManager(QObject *parent = nullptr) :
          QObject{parent}
    {
        if( !QAiUtils::createDefaultDirs() ){
            // LOG
        }

        if(!QAiUtils::fileExists(QAiUtils::userJson)){
            setSessionState(QAiSessionState::NoUsers);
        }else{
            // Okay we want to setup the object so taht the qml pl;ugin will get the QJsonObject back.
            // setHasUsers(true);
            loadUsers();

        }

    }

    ~QAiSessionManager() noexcept override;
    enum QAiSessionState {
        NoUsers,
        ScanningUsers,
        LoggedOut,
        SigningUp,
        Unlocking,
        LoggedIn,
        Locking,
        Error
    };
    Q_ENUM(QAiSessionState)




    bool loadUsers(){
        setSessionState(QAiSessionState::ScanningUsers);
        auto txt = QAiUtils::readTextFile(QAiUtils::userJson);
        // QJsonApiClient
        QJsonParseError jerr;
        QJsonDocument doc = QJsonDocument::fromJson(txt.toLatin1(), &jerr);
        if(doc.isNull()){
           // yikes
        } else {
            int nUsers = 0;
            QJsonObject obj = doc.object();
            if(!obj.isEmpty() && obj.contains("users")){
                QJsonArray jarry = obj["users"].toArray();
                if(!jarry.isEmpty()){
                    for(auto userObjVal : jarry){
                        QJsonObject userObj = userObjVal.toObject();
                        if(!userObj.isEmpty()){
                            if(userObj.contains("uid") && userObj.contains("icon")){
                                nUsers++;
                                // Okay we emit the json object so that the QmlPlugin gets it in fills its MVC
                                Q_EMIT foundUser(userObj);
                            }
                        }else{
                            //YIKES
                        }
                    }
                } else{
                    // YIKES
                }
            }else{
                //yikes
            }

            if(nUsers > 0){
                set_hasUsers(true);
                m_initList = obj;
            }else{
                set_hasUsers(false);
                //YIKES
            }


        }
        return m_hasUsers;
    }

    bool appendUserToJson(const QString &uid, const QString &name, const QString &icon, bool write = true){
        if(!m_initList.isEmpty() && m_initList.contains("users")){
            QJsonArray jarry = m_initList["users"].toArray();
            int jarrySize = jarry.size();
            if(!jarry.isEmpty()){
                QJsonObject userObj;
                userObj["uid"] = uid;
                userObj["name"] = name;
                userObj["icon"] = icon;
                jarry.append(QJsonValue(userObj));
            }

            if(jarrySize == (jarry.size() - 1)){
                if(write){
                    QJsonDocument jdoc;
                    jdoc.setObject(m_initList);
                    if(QAiUtils::writeTextFile(QAiUtils::userJson, jdoc.toJson(QJsonDocument::Compact)))
                        return true;
                }else{
                    return true;
                }
            }
        }
        return false;
    }



    QFuture<bool> signup(const QString &userId,
                         const QString &displayName,
                         const QSecureMem &password);

    QFuture<bool> login(const QString &userId,
                        const QSecureMem &password);

    QFuture<bool> logout();

    QAiUserSession *currentSession() const noexcept
    {
        return m_currentSession;
    }

    QAiSessionState sessionState() const
    {
        return m_sessionState;
    }

    void setSessionState(QAiSessionState newSessionState)
    {
        if (m_sessionState != newSessionState){
            m_sessionState = newSessionState;
            Q_EMIT sessionStateChanged();
        }
    }

Q_SIGNALS:
    void sessionStateChanged();
    void foundUser(QJsonObject);

private:
    QList<QAiUserSession*> m_users;
    QAiUserSession *m_currentSession = nullptr;
    QAiSessionVault *m_vault = nullptr;

    QAiSessionState m_sessionState = QAiSessionState::LoggedOut;
    QJsonObject m_initList{}; // example {[{"name" : "johnDoe", "icon" : "/usr/share/icons/default.png" }. {}]}

};

#endif // QAISESSIONMANAGER_H
