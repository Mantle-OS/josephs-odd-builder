#pragma once
#include <QObject>

#include <pointer-macros.h>

#include <qsecuremem.h>

#include <job_zstd.h>

#include "qzstdoptions.h"

class QZstd : public QZstdOptions
{
    Q_OBJECT

public:
    explicit QZstd(QObject *parent) :
        QZstdOptions{parent},
        m_zstd{new job::zstd::JobZstd{}}
    {
        connect(this, &QZstdOptions::inputChanged,
                this, [this](const QString &path) {
                    if(m_zstd)
                        m_zstd->setInput(path.toStdString());
                });

        connect(this, &QZstdOptions::outputChanged,
                this, [this](const QString &path) {
                    if(m_zstd)
                        m_zstd->setOutput(path.toStdString());
                });

        connect(this, &QZstdOptions::compressionLevelChanged,
                this, [this](int level) {
                    if(m_zstd)
                        m_zstd->setCompressionLevel(level);
                });

        connect(this, &QZstdOptions::preserveEmptyDirectoriesChanged,
                this, [this](bool value) {
                    if(m_zstd)
                        m_zstd->setPreserveEmptyDirectories(value);
                });

        connect(this, &QZstdOptions::preserveSymlinksChanged,
                this, [this](bool value) {
                    if(m_zstd)
                        m_zstd->setPreserveSymlinks(value);
                });

        connect(this, &QZstdOptions::recursiveDirectoriesChanged,
                this, [this](bool value) {
                    if(m_zstd)
                        m_zstd->setRecursiveDirectories(value);
                });

        m_zstd->setOnFinished([this]() {
            int const cur = m_zstd->current();
            int const tot = m_zstd->total();
            QString const err = QString::fromStdString(m_zstd->errorString());
            QMetaObject::invokeMethod(this, [this, cur, tot, err]() {
                set_current(cur);
                set_total(tot);
                set_errorString(err);
                Q_EMIT finished();
            }, Qt::QueuedConnection);
        });
    }
    ~QZstd()
    {
        if(m_zstd){
            delete m_zstd;
            m_zstd = nullptr;
        }
    }

    Q_INVOKABLE void compress()
    {
        if(m_zstd)
            m_zstd->compress();
    }
    Q_INVOKABLE void decompress()
    {
        if(m_zstd)
            m_zstd->decompress();
    }

    Q_INVOKABLE void compress(bool sign, bool encrypt)
    {
        if(m_zstd)
            m_zstd->compress(sign, encrypt);
    }
    Q_INVOKABLE void decompress(bool verify, bool decrypt)
    {
        if(m_zstd)
            m_zstd->decompress(verify, decrypt);
    }

    [[nodiscard]] QString publicKeyFile() const noexcept
    {
        if(m_zstd)
            return QString::fromStdString(m_zstd->publicKeyFile());
        return {};
    }
    [[nodiscard]] bool setPublicKeyFile([[maybe_unused]] const QString &pubKey) noexcept{
        if(!m_zstd)
            return false;
        std::filesystem::path p(pubKey.toStdString());
        return m_zstd->setPublicKeyFile(p);
    }

    [[nodiscard]] QString privateKeyFile() const noexcept
    {
        if(m_zstd)
            return QString::fromStdString(m_zstd->privateKeyFile().string());
        return {};
    }
    [[nodiscard]] bool setPrivateKeyFile([[maybe_unused]]const QString &privKey) noexcept
    {
        if(!m_zstd)
            return false;

        std::filesystem::path p(privKey.toStdString());
        return m_zstd->setPrivateKeyFile(p);
    }

    [[nodiscard]]const QSecureMem &privateKey() const noexcept
    {
        return m_privateKey;
    }

    void setPrivateKey(const QSecureMem &key) noexcept
    {
        if(!m_zstd)
            return;

        if(m_privateKey == key)
            return;

        m_privateKey = key;
        m_zstd->setPrivateKey(m_privateKey);
        Q_EMIT privateKeyChanged();

    }

Q_SIGNALS:
    void privateKeyChanged();

private:
    job::zstd::JobZstd      *m_zstd         = nullptr;
    QSecureMem              m_privateKey;
};
