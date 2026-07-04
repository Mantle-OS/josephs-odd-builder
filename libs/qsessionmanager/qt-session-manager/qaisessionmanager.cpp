#include "qaisessionmanager.h"
#include <QJsonParseError>
#include <QDebug>

#include <sodium.h>
#include <qextrarandom.h>

#include "qaisessionworker.h"
QAiSessionManager::QAiSessionManager(QObject *parent) :
    QObject{parent}
{
    // Make sure layout environments are generated on boot
    if (!QAiUtils::createDefaultDirs()) {
        qCritical() << "[-] SessionManager: System failed to provision XDG target storage channels.";
        set_sessionState(QAiSessionState::Error);
        return;
    }

    if (!QAiUtils::fileExists(QAiUtils::userJson)) {
        initializeEmptyUserIndex();
        set_sessionState(QAiSessionState::NoUsers);
    } else {
        loadUsers();
    }
}

QAiSessionManager::~QAiSessionManager() noexcept
{
    qDeleteAll(m_cachedUsers);
    m_cachedUsers.clear();
}

void QAiSessionManager::initializeEmptyUserIndex()
{
    m_initList = QJsonObject{{"users", QJsonArray{}}};
    set_hasUsers(false);
}

bool QAiSessionManager::loadUsers()
{
    set_sessionState(QAiSessionState::ScanningUsers);
    const QString txt = QAiUtils::readTextFile(QAiUtils::userJson);

    if (txt.isEmpty()) {
        initializeEmptyUserIndex();
        set_sessionState(QAiSessionState::NoUsers);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(txt.toUtf8(), &parseError);

    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "[-] SessionManager: Index parsing exploded ->" << parseError.errorString();
        set_sessionState(QAiSessionState::Error);
        return false;
    }

    m_initList = doc.object();

    // Safety check: if "users" array doesn't exist, build it out natively
    if (!m_initList.contains("users")) {
        m_initList["users"] = QJsonArray{};
    }

    const QJsonArray usersArray = m_initList["users"].toArray();
    int validUserCount = 0;

    for (const auto &userVal : usersArray) {
        const QJsonObject userObj = userVal.toObject();
        if (!userObj.isEmpty()) {
            if (userObj.contains("uid") && userObj.contains("icon")) {
                validUserCount++;
                // Broadcast to QML plugin to instantly populate its MVC model view
                Q_EMIT foundUser(userObj);
            }
        } else {
            qWarning() << "[-] SessionManager: Encountered empty or corrupted JSON user block.";
        }
    }

    const bool activeUsersFound = (validUserCount > 0);
    set_hasUsers(activeUsersFound);
    set_sessionState(activeUsersFound ? QAiSessionState::LoggedOut : QAiSessionState::NoUsers);

    return get_hasUsers();
}

bool QAiSessionManager::appendUserToJson(const QString &uid, const QString &name, const QString &icon)
{
    // Ensure index maps are structurally initialized before injecting elements
    if (m_initList.isEmpty() || !m_initList.contains("users")) {
        m_initList = QJsonObject{{"users", QJsonArray{}}};
    }

    QJsonArray usersArray = m_initList["users"].toArray();
    const int originalSize = usersArray.size();

    // Enforce duplicate protection checks against registered UIDs
    for (const auto &userVal : usersArray) {
        if (userVal.toObject()["uid"].toString() == uid) {
            qWarning() << "[-] SessionManager: Registration dropped. User ID already active:" << uid;
            return false;
        }
    }

    QJsonObject newUser;
    newUser["uid"]  = uid;
    newUser["name"] = name;
    newUser["icon"] = icon;
    usersArray.append(newUser);

    m_initList["users"] = usersArray;

    // Check that array element counters shifted predictably before committing writes
    if (usersArray.size() == (originalSize + 1)) {
        const QJsonDocument doc(m_initList);
        const bool writeSuccess = QAiUtils::writeTextFile(QAiUtils::userJson, doc.toJson(QJsonDocument::Compact));

        if (writeSuccess) {
            set_hasUsers(true);
            return true;
        }
    }

    return false;
}

QAiUserSession* QAiSessionManager::currentSession() const noexcept
{
    return m_currentSession;
}

QFuture<bool> QAiSessionManager::login(const QString &userId, const QSecureMem &password)
{
    set_sessionState(Unlocking);

    // 1. Resolve user data payload profile variables out of m_initList mapping index
    QJsonArray usersArray = m_initList["users"].toArray();
    QJsonObject targetUser;
    bool found = false;

    for (const auto &userVal : usersArray) {
        QJsonObject uObj = userVal.toObject();
        if (uObj["uid"].toString() == userId) {
            targetUser = uObj;
            found = true;
            break;
        }
    }

    if (!found) {
        qWarning() << "[-] SessionManager: Login failed. User not registered in index:" << userId;
        set_sessionState(QAiSessionState::Error);
        return QtConcurrent::run([]() { return false; });
    }

    // Recover path destinations out of index mappings
    QString const vaultPath = targetUser["vault_path"].toString();

    // Convert base64 salt from index back to binary bytes
    QByteArray const salt = QByteArray::fromBase64(targetUser["kdf_salt"].toString().toUtf8());

    // 2. Offload resource-intensive decryption workloads to background workers
    return QtConcurrent::run([this, password, salt, vaultPath]() {
        jsergen::AiSessionVault transientVaultData;

        if (!QAiSessionWorker::unlockAndParse(transientVaultData, password, salt, vaultPath)) {
            QMetaObject::invokeMethod(this, [this]() { set_sessionState(QAiSessionState::Error); });
            return false;
        }

        // Build active session structure inside the thread allocation step
        auto *newSession = new QAiUserSession{};
        if (!newSession->loadFromVault(transientVaultData, password)) {
            delete newSession;
            QMetaObject::invokeMethod(this, [this]() { set_sessionState(QAiSessionState::Error); });
            return false;
        }

        // Marshall ownership back to the primary thread loop context safely
        QMetaObject::invokeMethod(this, [this, newSession]() {
            if (m_currentSession) {
                m_currentSession->clear();
                m_currentSession->deleteLater();
            }
            m_currentSession = newSession;
            m_currentSession->setParent(this);
            set_sessionState(QAiSessionState::LoggedIn);
        });

        return true;
    });
}

QFuture<bool> QAiSessionManager::signup(const QString &userId, const QString &displayName, const QSecureMem &password)
{
    set_sessionState(SigningUp);

    // Generate path locations and hardware salt frames for the fresh identity profile
    QString const individualVaultName = QString("vault_%1.pkg").arg(userId);
    QString const targetVaultPath = QString("%1/%2").arg(QAiUtils::userDir).arg(individualVaultName);

    // Generate fresh crypto-random salt parameters out of your libsodium helper pass
    QByteArray const generatedSalt = QExtraRandom::randomSalt();

    return QtConcurrent::run([this, userId, displayName, password, generatedSalt, targetVaultPath]() {
        // Construct the initial fresh empty vault payload layer
        jsergen::AiSessionVault cleanVault;
        cleanVault.schema_version = 1;
        cleanVault.user_id = userId.toStdString();
        cleanVault.display_name = displayName.toStdString();

        // Lock it onto the storage filesystem
        if (!QAiSessionWorker::compileAndLock(cleanVault, password, generatedSalt, targetVaultPath)) {
            QMetaObject::invokeMethod(this, [this]() { set_sessionState(QAiSessionState::Error); });
            return false;
        }

        // Rehydrate changes back into primary runtime maps
        QMetaObject::invokeMethod(this, [this, userId, displayName, generatedSalt, targetVaultPath]() {
            QString const saltB64 = QString::fromUtf8(generatedSalt.toBase64());

            if (appendUserToJson(userId, displayName, "default_icon.png")) {
                // Find and update the newly appended user with salt and path mappings
                QJsonArray usersArray = m_initList["users"].toArray();
                for (int i = 0; i < usersArray.size(); ++i) {
                    QJsonObject uObj = usersArray[i].toObject();
                    if (uObj["uid"].toString() == userId) {
                        uObj["vault_path"] = targetVaultPath;
                        uObj["kdf_salt"] = saltB64;
                        usersArray[i] = uObj;
                        break;
                    }
                }
                m_initList["users"] = usersArray;

                // Flush out to disk storage
                QJsonDocument doc(m_initList);
                QAiUtils::writeTextFile(QAiUtils::userJson, doc.toJson(QJsonDocument::Compact));

                loadUsers(); // Refresh listings and cycle back to LoggedOut layout
            } else {
                set_sessionState(QAiSessionState::Error);
            }
        });

        return true;
    });
}

QFuture<bool> QAiSessionManager::logout()
{
    set_sessionState(Locking);

    return QtConcurrent::run([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            if (m_currentSession) {
                m_currentSession->clear();
                m_currentSession->deleteLater();
                m_currentSession = nullptr;
            }
            set_sessionState(QAiSessionState::LoggedOut);
        });
        return true;
    });
}