#include "qmlsodiumpasswordutils.h"

#include <qsodiumpasswordutils.h>
#include <QDebug>

QmlSodiumPasswordUtils::QmlSodiumPasswordUtils(QObject *parent)
    : QObject(parent)
{
}

QString QmlSodiumPasswordUtils::hashForStorage() const noexcept
{
    if (get_password().isEmpty()) {
        qWarning() << "[QmlSodiumPasswordUtils] Cannot generate storage hash from an empty password.";
        return {};
    }

    return QSodiumPasswordUtils::hashPasswordForStorage(get_password());
}

bool QmlSodiumPasswordUtils::verifyAgainstStorage(const QString &storedHash) const noexcept
{
    if (get_password().isEmpty() || storedHash.isEmpty()) {
        return false;
    }

    return QSodiumPasswordUtils::verifyPasswordAgainstStorage(get_password(), storedHash);
}