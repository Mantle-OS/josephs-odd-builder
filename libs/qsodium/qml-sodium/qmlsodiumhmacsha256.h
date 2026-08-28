#pragma once

#include <QObject>
#include <QString>

#include <qqmlregistration.h>

#include <property-macros.h>

#include <qsodiumhmacsha256.h>

#include "qmlsecuremem.h"
#include "qmlsodium_export.h"

class QMLSODIUM_EXPORT SodiumHmacSha256 : public QObject
{
    Q_OBJECT
    QP_RO(QString,          mac,        "unknown")
    QP_RO(QString,          lastMac,    "unknown")
    QP_RO(bool,             valid,      false)
    QP_RO(bool,             hasKey,     false)
    Q_PROPERTY(QmlSecureMem *key READ key NOTIFY keyChanged FINAL)
    QML_ELEMENT

public:
    explicit SodiumHmacSha256(QObject *parent = nullptr);
    ~SodiumHmacSha256() override;

    SodiumHmacSha256(const SodiumHmacSha256 &) = delete;
    SodiumHmacSha256 &operator=(const SodiumHmacSha256 &) = delete;
    SodiumHmacSha256(SodiumHmacSha256 &&) = delete;
    SodiumHmacSha256 &operator=(SodiumHmacSha256 &&) = delete;

    [[nodiscard]] QmlSecureMem *key() const noexcept;

    Q_INVOKABLE bool setKey(QmlSecureMem *key) noexcept;
    Q_INVOKABLE void clearKey() noexcept;

    Q_INVOKABLE QString compute(const QString &data) noexcept;
    Q_INVOKABLE bool verify(const QString &data) noexcept;
    Q_INVOKABLE bool generateKey() noexcept;

Q_SIGNALS:
    void keyChanged(QmlSecureMem *key);

private:
    [[nodiscard]] bool hasKey() const noexcept;
    [[nodiscard]] bool isValid() noexcept;
    QmlSecureMem *m_key{nullptr}; // OWNED
};