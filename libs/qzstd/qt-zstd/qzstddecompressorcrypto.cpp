#include "qzstddecompressorcrypto.h"

QZstdDecompressorCrypto::QZstdDecompressorCrypto() :
    m_opts{new QZstdOptions{}},
    m_ownsOpts(true)
{
    setupOptionConnections();
    setOnFinished([this]() {
        if (m_opts)
            Q_EMIT m_opts->finished();
    });
}

QZstdDecompressorCrypto::QZstdDecompressorCrypto(QZstdOptions *opts) :
    m_ownsOpts(false)
{
    setOnFinished(nullptr);
    setOptions(opts);
    setOnFinished([this]() {
        if (m_opts)
            Q_EMIT m_opts->finished();
    });
}

QZstdDecompressorCrypto::~QZstdDecompressorCrypto()
{
    disconnectOptionConnections();
    setOnFinished(nullptr);
    if(m_ownsOpts && m_opts)
        delete m_opts;
    m_opts = nullptr;
}

const QSecureMem &QZstdDecompressorCrypto::decryptionKey() const noexcept
{
    return m_decryptionKey;
}

void QZstdDecompressorCrypto::setDecryptionKey(const QSecureMem &key) noexcept
{
    if(m_decryptionKey == key)
        return;

    m_decryptionKey = key;
    job::zstd::JobZstdDecompressorCrypto::setDecryptionKey(m_decryptionKey);
}


QZstdOptions *QZstdDecompressorCrypto::options() const
{
    return m_opts;
}


void QZstdDecompressorCrypto::setOptions(QZstdOptions *other)
{
    if (!other || m_opts == other)
        return;

    // dissconnect the old shit
    disconnectOptionConnections();
    if(m_ownsOpts)
        delete m_opts;
    m_opts = other;

    // setup the new connections
    setupOptionConnections();

    setInput(m_opts->get_input().toStdString());
    setOutput(m_opts->get_output().toStdString());
    setCompressionLevel(m_opts->get_compressionLevel());
    setPreserveEmptyDirectories(m_opts->get_preserveEmptyDirectories());
    setPreserveSymlinks(m_opts->get_preserveSymlinks());
    setRecursiveDirectories(m_opts->get_recursiveDirectories());
}



bool QZstdDecompressorCrypto::decryptAndDecompress() noexcept
{
    if(!m_opts)
        return false;

    if (!hasKeys()) {
        setErrorString("Cryptographic pipeline missing a valid encryption key call setEncryptionKey() first.");
        *m_opts = *this;
        return false;
    }

    bool ok = execute();
    if (m_opts && *m_opts != *this)
        *m_opts = *this;

    return ok;
}

void QZstdDecompressorCrypto::disconnectOptionConnections() noexcept
{
    if(!m_opts)
        return;
    QObject::disconnect(m_opts, &QZstdOptions::inputChanged, m_opts, nullptr);
    QObject::disconnect(m_opts, &QZstdOptions::outputChanged, m_opts, nullptr);
    QObject::disconnect(m_opts, &QZstdOptions::compressionLevelChanged, m_opts, nullptr);
    QObject::disconnect(m_opts, &QZstdOptions::preserveEmptyDirectoriesChanged, m_opts, nullptr);
    QObject::disconnect(m_opts, &QZstdOptions::preserveSymlinksChanged, m_opts, nullptr);
    QObject::disconnect(m_opts, &QZstdOptions::recursiveDirectoriesChanged, m_opts, nullptr);
}

void QZstdDecompressorCrypto::setupOptionConnections() noexcept
{
    QObject::connect(m_opts, &QZstdOptions::inputChanged,
                     m_opts, [this](const QString &path) {
                         setInput(path.toStdString());
                     });

    QObject::connect(m_opts, &QZstdOptions::outputChanged,
                     m_opts, [this](const QString &path) {
                         setOutput(path.toStdString());
                     });

    QObject::connect(m_opts, &QZstdOptions::compressionLevelChanged,
                     m_opts, [this](int level) {
                         setCompressionLevel(level);
                     });

    QObject::connect(m_opts, &QZstdOptions::preserveEmptyDirectoriesChanged,
                     m_opts, [this](bool value) {
                         setPreserveEmptyDirectories(value);
                     });

    QObject::connect(m_opts, &QZstdOptions::preserveSymlinksChanged,
                     m_opts, [this](bool value) {
                         setPreserveSymlinks(value);
                     });

    QObject::connect(m_opts, &QZstdOptions::recursiveDirectoriesChanged,
                     m_opts, [this](bool value) {
                         setRecursiveDirectories(value);
                     });
}

