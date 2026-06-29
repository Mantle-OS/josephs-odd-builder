#ifndef QMLSODIUMBOX_H
#define QMLSODIUMBOX_H

#include <QObject>
#include <QString>

#include <property-macros.h>

#include <qqmlregistration.h>

class QmlSodiumBox : public QObject
{
    Q_OBJECT
    QP_RW(QString, password,   "")
    QP_RW(QString, salt,       "")
    QP_RW(QString, cipherText, "")
    QP_RW(QString, nonce,      "")
    QML_ELEMENT

public:
    explicit QmlSodiumBox(QObject *parent = nullptr);
    ~QmlSodiumBox() override = default;
    Q_INVOKABLE bool encryptString(const QString &plainText);
    Q_INVOKABLE QString decryptToString();
    Q_INVOKABLE void generateNewSalt();
};

#endif // QMLSODIUMBOX_H
