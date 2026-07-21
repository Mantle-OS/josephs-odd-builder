#include "qzstdcompressor.h"
#include <QObject>
QZstdCompressor::QZstdCompressor() :
    m_opts{new QZstdOptions{nullptr}},
    m_ownsOpts(true)
{
    disconnectOptionConnections();

    setupOptionConnections();
    setOnFinished([this]() {
        if (m_opts)
            Q_EMIT m_opts->finished();
    });
}

QZstdCompressor::QZstdCompressor(QZstdOptions *opts) :
    m_ownsOpts(false)
{
    setOnFinished(nullptr);
    setOptions(opts);
    setOnFinished([this]() {
        if (m_opts)
            Q_EMIT m_opts->finished();
    });
}

QZstdCompressor::~QZstdCompressor()
{
    disconnectOptionConnections();
    setOnFinished(nullptr);
    if (m_ownsOpts && m_opts)
        delete m_opts;

    m_opts = nullptr;
}

bool QZstdCompressor::compress()
{
    const bool ok = execute();

    if (m_opts && *m_opts != *this)
        *m_opts = *this;

    return ok;
}

QZstdOptions *QZstdCompressor::options() const
{
    return m_opts;
}

void QZstdCompressor::setOptions(QZstdOptions *other)
{
    if (!other || m_opts == other)
        return;

    disconnectOptionConnections();

    if(m_ownsOpts)
        delete m_opts;

    m_opts = other;
    m_ownsOpts = false;

    setupOptionConnections();

    setInput(m_opts->get_input().toStdString());
    setOutput(m_opts->get_output().toStdString());
    setCompressionLevel(m_opts->get_compressionLevel());
    setPreserveEmptyDirectories(m_opts->get_preserveEmptyDirectories());
    setPreserveSymlinks(m_opts->get_preserveSymlinks());
    setRecursiveDirectories(m_opts->get_recursiveDirectories());
}

void QZstdCompressor::disconnectOptionConnections() noexcept
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

void QZstdCompressor::setupOptionConnections() noexcept
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
