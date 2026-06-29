#ifndef QMLSODIUMPASSWORDUTILS_H
#define QMLSODIUMPASSWORDUTILS_H

#include <QObject>
#include <QString>
#include <qqmlregistration.h>
#include <property-macros.h>

class QmlSodiumPasswordUtils : public QObject
{
    Q_OBJECT
    QP_RW(QString, password, "")
    QML_ELEMENT
public:
    explicit QmlSodiumPasswordUtils(QObject *parent = nullptr);
    ~QmlSodiumPasswordUtils() override = default;
    Q_INVOKABLE QString hashForStorage() const noexcept;
    Q_INVOKABLE bool verifyAgainstStorage(const QString &storedHash) const noexcept;
};

#endif // QMLSODIUMPASSWORDUTILS_H
