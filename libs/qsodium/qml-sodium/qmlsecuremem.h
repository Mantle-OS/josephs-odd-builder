#pragma once

#include <QObject>

#include <qqmlregistration.h>

#include <qsecuremem.h>

#include "qmlsodium_export.h"
class QMLSODIUM_EXPORT QmlSecureMem : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit QmlSecureMem(QObject *parent = nullptr);

    ~QmlSecureMem() override;

    [[nodiscard]] QSecureMem *internalBuffer() noexcept;
    [[nodiscard]] const QSecureMem *internalBuffer() const noexcept;

    [[nodiscard]] bool copyFromSecureMem(const QSecureMem &source) noexcept;
    [[nodiscard]] QSecureMem *mem() const noexcept;

private:
    QSecureMem *m_mem = nullptr;
};