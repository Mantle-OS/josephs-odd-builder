#pragma once
#include <QString>

#include <qsecuremem.h>

#include <job_zstd_sign.h>

#include "qzstdoptions.h"
#include "qzstd_export.h"

class QZSTD_EXPORT QZstdSign : public job::zstd::JobZstdSign
{    
public:
    explicit QZstdSign();
    explicit QZstdSign(const QZstdSign &) = delete;
    QZstdSign(QZstdSign &&) noexcept = delete;
    QZstdSign &operator=(const QZstdSign &) = delete;
    QZstdSign &operator=(QZstdSign &&) noexcept = delete;
    ~QZstdSign();

    [[nodiscard]] QString publicKeyFile() const noexcept;
    [[nodiscard]] bool setPublicKeyFile(const QString &publicKeyFile) noexcept;

    [[nodiscard]] QString privateKeyFile() const noexcept;
    [[nodiscard]] bool setPrivateKeyFile(const QString &privateKeyFile) noexcept;

    [[nodiscard]] bool signFile(const QString &inPath, const QString &outPath, bool overwrite = true);
    [[nodiscard]] bool verifyFile(const QString  &filePath, const QString &signatureBase64);

private:
    void syncOptions();
    void disconnectOptionConnections() noexcept;
    void setupOptionConnections() noexcept;

    QZstdOptions    *m_opts     = nullptr;
    bool            m_ownsOpts  = true;
};