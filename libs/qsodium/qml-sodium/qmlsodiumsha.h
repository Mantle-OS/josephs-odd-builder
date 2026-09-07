#pragma once

#include <cstddef>

#include <QObject>
#include <QString>

#include <qqmlregistration.h>

#include <pointer-macros.h>
#include <property-macros.h>

#include <qmlstringlist.h>
#include <qsodiumsha.h>

#include "qmlsodium_export.h"

class QMLSODIUM_EXPORT SodiumSha : public QObject
{
    Q_OBJECT
    QP_RO(QString,           lastSha,     "unknown" )
    QP_RO(QString,           errorString, "unknown" )
    QP_PTR_RO(QmlStringList, shaTypes               )
    Q_PROPERTY(ShaType currentType READ currentType WRITE setCurrentType NOTIFY currentTypeChanged FINAL)
    QML_ELEMENT
    QML_SINGLETON

public:
    enum class ShaType
    {
        Sha224 = 0,
        Sha256,
        Sha384,
        Sha512,
        Sha3_224,
        Sha3_256,
        Sha3_384,
        Sha3_512,
        ShaGeneric,
        ShaBlake2b
    };
    Q_ENUM(ShaType)

    explicit SodiumSha(QObject *parent = nullptr);
    ~SodiumSha() override = default;

    SodiumSha(const SodiumSha &) = delete;
    SodiumSha &operator=(const SodiumSha &) = delete;
    SodiumSha(SodiumSha &&) = delete;
    SodiumSha &operator=(SodiumSha &&) = delete;

    [[nodiscard]] ShaType currentType() const noexcept;

public Q_SLOTS:
    QString computeHex(const QString &data) noexcept;
    QString computeHex(const QString &data, ShaType type) noexcept;
    void setCurrentType(ShaType newCurrentType) noexcept;

Q_SIGNALS:
    void finished(ShaType type, const QString &sha);
    void currentTypeChanged(ShaType type);

private:
    ShaType m_currentType{ShaType::ShaGeneric};
    job::crypto::JobShaType m_jobType{job::crypto::JobShaType::ShaGeneric};
};