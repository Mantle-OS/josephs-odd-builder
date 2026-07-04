#ifndef QMLSODIUMHASH_H
#define QMLSODIUMHASH_H

#include <QObject>
#include <QQmlEngine>

#include <property-macros.h>

#include <qsodiumhash.h>

class QmlSodiumHash: public QObject
{
    Q_OBJECT
    QP_RW(QString, filePath, "")
    QP_RO(QString, lastHash, "")

    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QmlSodiumHash(QObject *parent = nullptr) :
        QObject{parent}
    {
    }

    ~QmlSodiumHash() = default;

    Q_INVOKABLE QString hashBuffer(const QString &data)
    {
        QByteArray const rawHash = QSodiumHash::hashBuffer(data.toUtf8());
        QString const hexHash = QString::fromLatin1(rawHash.toHex());

        set_lastHash(hexHash);
        return hexHash;
    }

    Q_INVOKABLE QString hashFile(){
        if (get_filePath().isEmpty()) {
            return {};
        }

        QByteArray const rawHash = QSodiumHash::hashFile(get_filePath());
        QString const hexHash = QString::fromLatin1(rawHash.toHex());

        set_lastHash(hexHash);
        return hexHash;
    }
};

#endif // QMLSODIUMHASH_H
