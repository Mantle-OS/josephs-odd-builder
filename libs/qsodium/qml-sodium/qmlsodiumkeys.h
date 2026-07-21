#pragma once
#include <QObject>
#include <QStandardPaths>

#include <QQmlEngine>

#include <property-macros.h>
#include <qaiutils.h>

#include <qsodiumkeys.h>

#include "qmlsodium_export.h"
class QMLSODIUM_EXPORT QmlSodiumKeys : public QObject
{
    Q_OBJECT

    QP_RW(QString, keyDir,          QStandardPaths::writableLocation(QStandardPaths::TempLocation)  )
    QP_RW(QString, publicKeyFile,   ""                                                              )
    QP_RW(QString, privateKeyFile,  ""                                                              )
    QP_RW(QString, publicKeyBase64, ""                                                              )

    QML_ELEMENT
public:
    enum class KeyType {
        Exchange = static_cast<int>(QSodiumKeys::KeyType::Exchange),
        Sign     = static_cast<int>(QSodiumKeys::KeyType::Sign)
    };
    Q_ENUM(KeyType)

    explicit QmlSodiumKeys(QObject *parent = nullptr);

    ~QmlSodiumKeys() override;

    Q_INVOKABLE bool create(QmlSodiumKeys::KeyType type) noexcept;
    Q_INVOKABLE bool validSet() noexcept;

    Q_INVOKABLE bool loadKeysFromDisk() noexcept;
    Q_INVOKABLE bool saveKeysToDisk() noexcept;

private:
    QSodiumKeys *m_keys = nullptr; // owned

    QString getFullPath(const QString &fileName) const noexcept;
};
