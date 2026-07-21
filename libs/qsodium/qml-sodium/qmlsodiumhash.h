#pragma once

#include <QObject>

#include <QQmlEngine>

#include <property-macros.h>

#include <qsodiumhash.h>

#include "qmlsodium_export.h"
class QMLSODIUM_EXPORT QmlSodiumHash: public QObject {
    Q_OBJECT
    QP_RW(QString, filePath, "")
    QP_RO(QString, lastHash, "")

    QML_ELEMENT
    QML_SINGLETON
public:
    explicit QmlSodiumHash(QObject *parent = nullptr);
    ~QmlSodiumHash() = default;
    Q_INVOKABLE QString hashBuffer(const QString &data) noexcept;
    Q_INVOKABLE QString hashFile() noexcept;
};
