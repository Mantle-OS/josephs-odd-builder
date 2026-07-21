#pragma once
#include <QObject>
#include <QString>

#include <pointer-macros.h>

#include <qsecuremem.h>

#include <job_zstd.h>

#include "qzstdoptions.h"

#include "qzstd_export.h"

class QZSTD_EXPORT QZstd : public QZstdOptions
{
    Q_OBJECT

public:
    explicit QZstd(QObject *parent);
    ~QZstd();

    Q_INVOKABLE void compress();
    Q_INVOKABLE void decompress();
    Q_INVOKABLE void compress(bool sign, bool encrypt);
    Q_INVOKABLE void decompress(bool verify, bool decrypt);

    [[nodiscard]] QString publicKeyFile() const noexcept;
    [[nodiscard]] bool setPublicKeyFile([[maybe_unused]] const QString &pubKey) noexcept;
    [[nodiscard]] QString privateKeyFile() const noexcept;
    [[nodiscard]] bool setPrivateKeyFile([[maybe_unused]]const QString &privKey) noexcept;
    [[nodiscard]]const QSecureMem &privateKey() const noexcept;
    void setPrivateKey(const QSecureMem &key) noexcept;

Q_SIGNALS:
    void privateKeyChanged();

private:
    job::zstd::JobZstd      *m_zstd         = nullptr;
    QSecureMem              m_privateKey;
};
