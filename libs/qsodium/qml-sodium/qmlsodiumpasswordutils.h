#ifndef QMLSODIUMPASSWORDUTILS_H
#define QMLSODIUMPASSWORDUTILS_H

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

#include <qmlsecuremem.h>
class QmlSodiumPasswordUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QmlSecureMem *password READ password NOTIFY passwordChanged FINAL)
    QML_ELEMENT

public:
    explicit QmlSodiumPasswordUtils(QObject *parent = nullptr);
    ~QmlSodiumPasswordUtils() override;

    QmlSecureMem *password() const noexcept;

    Q_INVOKABLE bool setPassword(QmlSecureMem *source) noexcept;
    Q_INVOKABLE void clearPassword() noexcept;

    Q_INVOKABLE QString hashForStorage() const noexcept;
    Q_INVOKABLE bool verifyAgainstStorage(const QString &storedHash) const noexcept;

Q_SIGNALS:
    void passwordChanged(QmlSecureMem *password);

private:
    QmlSecureMem *m_password = nullptr;
};

#endif // QMLSODIUMPASSWORDUTILS_H