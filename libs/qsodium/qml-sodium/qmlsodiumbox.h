#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

#include <property-macros.h>

#include <qsodiumpasswordutils.h>

#include "qmlsecuremem.h"
#include "qmlsodium_export.h"

class QMLSODIUM_EXPORT QmlSodiumBox : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QmlSecureMem *password READ get_password NOTIFY passwordChanged)
    QP_RW(QString,           salt,       "")
    QP_RW(QString,           cipherText, "")
    QP_RW(QString,           nonce,      "")
    QML_ELEMENT

public:
    explicit QmlSodiumBox(QObject *parent = nullptr);
    ~QmlSodiumBox() override;

    // main method that copys the SecureMem this is called from Qml
    QmlSecureMem *get_password() noexcept;

    // pass the pointer from the secure text input to here and copy it up
    Q_INVOKABLE bool setPassword(QmlSecureMem *source) noexcept;

    Q_INVOKABLE bool encryptString(const QString &plainText);
    Q_INVOKABLE QString decryptToString();
    Q_INVOKABLE void generateNewSalt();

Q_SIGNALS:
    void passwordChanged(QmlSecureMem *passwordMemory);

private:
    bool deriveKey(QSecureMem &derivedKey) noexcept;
    void set_password(QmlSecureMem *source) noexcept;
    bool copyPasswordMemoryFrom(QmlSecureMem *source) noexcept;

    QmlSecureMem *m_password = nullptr;
};

