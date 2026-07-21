#pragma once

#include <QObject>
#include <QDebug>
#include <qqmlregistration.h>

#include <qmlsecuremem.h>
#include <qmlsodiumkeys.h>
#include <qzstd.h>

#include "qmlzstd_export.h"

class QMLZSTD_EXPORT QmlZstd : public QZstd
{
    Q_OBJECT
    Q_PROPERTY(QmlSodiumKeys    *signingKeys    READ signingKeys CONSTANT)
    Q_PROPERTY(QmlSecureMem     *encryptionKey  READ get_encryptionKey NOTIFY encryptionKeyChanged)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QmlZstd(QObject *parent = nullptr);
    ~QmlZstd() override;

    QmlSodiumKeys *signingKeys() const noexcept;
    QmlSecureMem *get_encryptionKey() noexcept;
    Q_INVOKABLE bool setEncryptionKey(QmlSecureMem *source);

Q_SIGNALS:
    void encryptionKeyChanged();

private:
    QmlSodiumKeys *m_signingKeys = nullptr;   // owned
    QmlSecureMem *m_encryptionKey = nullptr;  // owned
};
