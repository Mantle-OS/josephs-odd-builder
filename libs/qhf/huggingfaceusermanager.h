#ifndef HUGGINGFACEUSERMANAGER_H
#define HUGGINGFACEUSERMANAGER_H

#include <QObject>
#include <QQmlEngine>
#include <pointer-macros.h>
#include <objectmodel.h>
#include <qsecuremem.h>
#include <qaiusersession.h>
#include <qaitypes.h>

#include "huggingfaceuser.h"
#include "huggingfaceapi.h"

class HuggingFaceUserManager : public QObject
{
    Q_OBJECT

    QP_PTR_RO(HuggingFaceUser, anonymousUser)   // owned
    QP_PTR_RO(HuggingFaceUser, currentUser)     // tracking model reference
    QP_PTR_RO(ObjectListModel<HuggingFaceUser>, users)
    QP_RO(bool, keyInEnv, false)

    QML_ELEMENT
    QML_UNCREATABLE("Please use HuggingFaceHub.userManager")

public:
    explicit HuggingFaceUserManager(QObject *parent = nullptr);
    ~HuggingFaceUserManager() override;

    void syncFromGlobalSession(const QAiUserSession *globalSession) noexcept;
    void login(HuggingFaceApi *api, const QString &accountName);



    void useAnonymous();

Q_SIGNALS:
    void loggedIn(bool success);
};

#endif // HUGGINGFACEUSERMANAGER_H