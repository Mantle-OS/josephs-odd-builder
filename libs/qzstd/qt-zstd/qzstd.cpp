#include "qzstd.h"


QZstd::QZstd(QObject *parent) :
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

QZstd::~QZstd()
{
    if(m_zstd){
        delete m_zstd;
        m_zstd = nullptr;
    }
}

void QZstd::compress()
{
    if(m_zstd)
        m_zstd->compress();
}

void QZstd::decompress()
{
    if(m_zstd)
        m_zstd->decompress();
}

void QZstd::compress(bool sign, bool encrypt)
{
    if(m_zstd)
        m_zstd->compress(sign, encrypt);
}

void QZstd::decompress(bool verify, bool decrypt)
{
    if(m_zstd)
        m_zstd->decompress(verify, decrypt);
}

QString QZstd::publicKeyFile() const noexcept
{
    if(m_zstd)
        return QString::fromStdString(m_zstd->publicKeyFile());
    return {};
}

bool QZstd::setPublicKeyFile(const QString &pubKey) noexcept{
    if(!m_zstd)
        return false;
    std::filesystem::path p(pubKey.toStdString());
    return m_zstd->setPublicKeyFile(p);
}

QString QZstd::privateKeyFile() const noexcept
{
    if(m_zstd)
        return QString::fromStdString(m_zstd->privateKeyFile().string());
    return {};
}

bool QZstd::setPrivateKeyFile(const QString &privKey) noexcept
{
    if(!m_zstd)
        return false;

    std::filesystem::path p(privKey.toStdString());
    return m_zstd->setPrivateKeyFile(p);
}

void QZstd::setPrivateKey(const QSecureMem &key) noexcept
{
    if(!m_zstd)
        return;

    if(m_privateKey == key)
        return;

    m_privateKey = key;
    m_zstd->setPrivateKey(m_privateKey);
    Q_EMIT privateKeyChanged();

}

const QSecureMem &QZstd::privateKey() const noexcept
{
    return m_privateKey;
}

