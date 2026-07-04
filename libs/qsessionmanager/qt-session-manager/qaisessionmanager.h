#ifndef QAISESSIONMANAGER_H
#define QAISESSIONMANAGER_H

#include <QObject>
#include <QFuture>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QList>
#include <QtConcurrent>

// Utils
#include <qaiutils.h>
#include <property-macros.h>

// genreated scheam
#include <aisession/session_vault.hpp>

// Sodium side
#include <qsecuremem.h>

// Local to this lib
#include "qaisessionvault.h"
#include "qaiusersession.h"

namespace jsergen = job::serializer::generated;
class QAiSessionManager : public QObject
{
    Q_OBJECT
    QP_RO(bool, hasUsers, false)

public:
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

protected:
    QP_RW(QAiSessionState, sessionState, QAiSessionManager::LoggedOut)

public:
    explicit QAiSessionManager(QObject *parent = nullptr);
    ~QAiSessionManager() noexcept override;


    QAiSessionManager(const QAiSessionManager &) = delete;
    QAiSessionManager &operator=(const QAiSessionManager &) = delete;

    [[nodiscard]] QAiUserSession *currentSession() const noexcept;

    // Threaded execution states via QtConcurrent workers
    [[nodiscard]] QFuture<bool> signup(const QString &userId,
                                       const QString &displayName,
                                       const QSecureMem &password);

    // Visualizing the inner manager linkage pass
    QFuture<bool> login(const QString &userId, const QSecureMem &password);

    [[nodiscard]] QFuture<bool> logout();

public Q_SLOTS:
    bool loadUsers();

Q_SIGNALS:
    void foundUser(const QJsonObject &userObj);
    void initializationFailed(const QString &reason);

private:
    bool appendUserToJson(const QString &uid, const QString &name, const QString &icon);
    void initializeEmptyUserIndex();

    QAiUserSession        *m_currentSession{nullptr};
    QAiSessionVault       *m_vault{nullptr};
    QJsonObject            m_initList{};

    QList<QAiUserSession*> m_cachedUsers;
};

#endif // QAISESSIONMANAGER_H