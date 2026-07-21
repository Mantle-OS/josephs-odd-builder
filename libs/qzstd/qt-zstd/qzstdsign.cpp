#include "qzstdsign.h"
#include <filesystem>
#include <QObject>

QZstdSign::QZstdSign() :
    m_opts{new QZstdOptions{}},
    m_ownsOpts(true)
{
    setupOptionConnections();
}

QZstdSign::~QZstdSign()
{
    disconnectOptionConnections();
    if(m_ownsOpts && m_opts)
        delete m_opts;

    m_opts = nullptr;
}

QString QZstdSign::publicKeyFile() const noexcept
{
    return QString::fromStdString(job::zstd::JobZstdSign::publicKeyFile().string());
}

bool QZstdSign::setPublicKeyFile(const QString &publicKeyFile) noexcept
{
    syncOptions();
    std::filesystem::path p(publicKeyFile.toStdString());
    return job::zstd::JobZstdSign::setPublicKeyFile(p);
}

QString QZstdSign::privateKeyFile() const noexcept
{
    return QString::fromStdString(job::zstd::JobZstdSign::privateKeyFile().string());
}

bool QZstdSign::setPrivateKeyFile(const QString &privateKeyFile) noexcept
{
    syncOptions();
    std::filesystem::path p(privateKeyFile.toStdString());
    return job::zstd::JobZstdSign::setPrivateKeyFile(p);
}

bool QZstdSign::signFile(const QString &inPath, const QString &outPath, bool overwrite)
{
    syncOptions();
    std::filesystem::path in(inPath.toStdString());
    std::filesystem::path out(outPath.toStdString());
    return job::zstd::JobZstdSign::signFile(in, out, overwrite);
}

bool QZstdSign::verifyFile(const QString &filePath, const QString &signatureBase64)
{
    syncOptions();
    std::filesystem::path p(filePath.toStdString());
    std::string sig = signatureBase64.toStdString();
    return job::zstd::JobZstdSign::verifyFile(p, sig);
}

void QZstdSign::syncOptions(){
    setInput(m_opts->get_input().toStdString());
    setOutput(m_opts->get_output().toStdString());
    // These do not matter but whatever
    setCompressionLevel(m_opts->get_compressionLevel());
    setPreserveEmptyDirectories(m_opts->get_preserveEmptyDirectories());
    setPreserveSymlinks(m_opts->get_preserveSymlinks());
    setRecursiveDirectories(m_opts->get_recursiveDirectories());
}

void QZstdSign::disconnectOptionConnections() noexcept
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

void QZstdSign::setupOptionConnections() noexcept
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
