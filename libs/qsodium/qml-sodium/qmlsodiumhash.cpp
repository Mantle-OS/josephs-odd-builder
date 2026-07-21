#include "qmlsodiumhash.h"

QmlSodiumHash::QmlSodiumHash(QObject *parent) :
    QObject{parent}
{
}

QString QmlSodiumHash::hashBuffer(const QString &data) noexcept
{
    QByteArray const rawHash = QSodiumHash::hashBuffer(data.toUtf8());
    QString const hexHash = QString::fromLatin1(rawHash.toHex());

    set_lastHash(hexHash);
    return hexHash;
}

QString QmlSodiumHash::hashFile() noexcept
{
    if (get_filePath().isEmpty())
        return {};

    QByteArray const rawHash = QSodiumHash::hashFile(get_filePath());
    QString const hexHash = QString::fromLatin1(rawHash.toHex());

    set_lastHash(hexHash);
    return hexHash;
}
