// job_zstd.cpp
#include "job_zstd.h"

#include <fstream>
#include <sstream>

namespace job::zstd {

JobZstd::JobZstd()
    : m_compress(new JobZstdCompressor())
    , m_decompress(new JobZstdDecompressor())
    , m_compressCrypto(new JobZstdCompressorCrypto())
    , m_decompressCrypto(new JobZstdDecompressorCrypto())
    , m_signer(new JobZstdSign())
{
}

JobZstd::~JobZstd()
{
    // No real cancellation exists here std::async gives no cooperative
    // way to interrupt work already in flight, same as Qt's own
    // cancel() couldn't actually stop a plain running function
    // either. This just waits for whatever's already running to finish
    // before the owned objects get destroyed out from under it.
    if (m_compressFuture.valid())
        m_compressFuture.wait();

    if (m_decompressFuture.valid())
        m_decompressFuture.wait();

    if (m_compressCryptoFuture.valid())
        m_compressCryptoFuture.wait();

    if (m_decompressCryptoFuture.valid())
        m_decompressCryptoFuture.wait();

    delete m_compress;
    delete m_decompress;
    delete m_compressCrypto;
    delete m_decompressCrypto;
    delete m_signer;
}

void JobZstd::setupTaskConnections(JobZstdOptions *task, std::future<bool> *watcher)
{
    static_cast<void>(task);
    static_cast<void>(watcher);
    // Intentionally empty for this pass -- no live progress relay, and
    // error-string propagation happens explicitly inline in each pipeline
    // lambda below rather than through a generic callback wiring here.
    // Kept as a named seam in case a future pass wants to centralize that.
}

bool JobZstd::futureIsRunning(const std::future<bool> &f) noexcept
{
    if (!f.valid())
        return false;

    return f.wait_for(std::chrono::seconds(0)) != std::future_status::ready;
}

bool JobZstd::isRunning() const noexcept
{
    return futureIsRunning(m_compressFuture)
        || futureIsRunning(m_decompressFuture)
        || futureIsRunning(m_compressCryptoFuture)
        || futureIsRunning(m_decompressCryptoFuture);
}

std::filesystem::path JobZstd::publicKeyFile() const noexcept
{
    return m_publicKeyFile;
}

bool JobZstd::setPublicKeyFile(const std::filesystem::path &pubKey) noexcept
{
    if (!m_signer->setPublicKeyFile(pubKey))
        return false;

    m_publicKeyFile = pubKey;
    return true;
}

std::filesystem::path JobZstd::privateKeyFile() const noexcept
{
    return m_privateKeyFile;
}

bool JobZstd::setPrivateKeyFile(const std::filesystem::path &privKey) noexcept
{
    if (!m_signer->setPrivateKeyFile(privKey))
        return false;

    m_privateKeyFile = privKey;
    return true;
}

job::crypto::JobSecureMem JobZstd::getPrivateKey() const noexcept
{
    return m_privateKey;
}

void JobZstd::setPrivateKey(const job::crypto::JobSecureMem &key) noexcept
{
    m_privateKey = key;
}

job::crypto::JobSecureMem JobZstd::getSignKey() const noexcept
{
    return m_signKey;
}

void JobZstd::setSignKey(const job::crypto::JobSecureMem &key) noexcept
{
    m_signKey = key;
}

void JobZstd::compress()
{
    if (futureIsRunning(m_compressFuture))
        return;

    setErrorString("");
    setupTaskConnections(m_compress, &m_compressFuture);

    m_compress->setInput(input());
    m_compress->setOutput(output());
    m_compress->setCompressionLevel(compressionLevel());
    m_compress->setPreserveEmptyDirectories(preserveEmptyDirectories());
    m_compress->setPreserveSymlinks(preserveSymlinks());
    m_compress->setRecursiveDirectories(recursiveDirectories());

    m_compressFuture = std::async(std::launch::async, [this]() {
        bool const ok = m_compress->execute();

        if (!ok)
            setErrorString(m_compress->errorString());

        notifyFinished();
        return ok;
    });
}

void JobZstd::decompress()
{
    if (futureIsRunning(m_decompressFuture))
        return;

    setErrorString("");
    setupTaskConnections(m_decompress, &m_decompressFuture);

    m_decompress->setInput(input());
    m_decompress->setOutput(output());
    m_decompress->setPreserveSymlinks(preserveSymlinks());

    m_decompressFuture = std::async(std::launch::async, [this]() {
        bool const ok = m_decompress->execute();

        if (!ok)
            setErrorString(m_decompress->errorString());

        notifyFinished();
        return ok;
    });
}

void JobZstd::compress(bool sign, bool encrypt)
{
    if (futureIsRunning(m_compressCryptoFuture))
        return;

    setErrorString("");
    setupTaskConnections(m_compressCrypto, &m_compressCryptoFuture);

    std::string const finalDestination = output();
    std::string const stageOutput = sign ? (finalDestination + ".tmp") : finalDestination;
    job::crypto::JobSecureMem const encKey = getPrivateKey();

    std::string const compIn = input();
    int const level = compressionLevel();
    bool const preserveEmpty = preserveEmptyDirectories();
    bool const preserveLinks = preserveSymlinks();
    bool const recursive = recursiveDirectories();

    m_compressCryptoFuture = std::async(std::launch::async,
        [this, sign, encrypt, compIn, stageOutput, finalDestination, level, preserveEmpty, preserveLinks, recursive, encKey]() {
        bool ok = false;

        if (encrypt) {
            m_compressCrypto->setInput(compIn);
            m_compressCrypto->setOutput(stageOutput);
            m_compressCrypto->setCompressionLevel(level);
            m_compressCrypto->setPreserveEmptyDirectories(preserveEmpty);
            m_compressCrypto->setPreserveSymlinks(preserveLinks);
            m_compressCrypto->setRecursiveDirectories(recursive);
            m_compressCrypto->setEncryptionKey(encKey);

            ok = m_compressCrypto->execute();
            if (!ok)
                setErrorString(m_compressCrypto->errorString());
        } else {
            m_compress->setInput(compIn);
            m_compress->setOutput(stageOutput);
            m_compress->setCompressionLevel(level);
            m_compress->setPreserveEmptyDirectories(preserveEmpty);
            m_compress->setPreserveSymlinks(preserveLinks);
            m_compress->setRecursiveDirectories(recursive);

            ok = m_compress->execute();
            if (!ok)
                setErrorString(m_compress->errorString());
        }

        if (ok && sign) {
            ok = m_signer->signFile(stageOutput, finalDestination + ".sig");

            if (!ok) {
                setErrorString("Crypto signing runtime failure during compress pipeline.");
            } else {
                std::error_code renameEc;
                std::filesystem::rename(stageOutput, finalDestination, renameEc);

                if (renameEc) {
                    ok = false;
                    setErrorString("Pipeline failed to promote signed asset package.");
                }
            }
        }

        notifyFinished();
        return ok;
    });
}

void JobZstd::decompress(bool verify, bool decrypt)
{
    if (futureIsRunning(m_decompressCryptoFuture))
        return;

    setErrorString("");
    setupTaskConnections(m_decompressCrypto, &m_decompressCryptoFuture);

    std::string const source = input();
    std::string const dest = output();
    bool const preserveLinks = preserveSymlinks();
    job::crypto::JobSecureMem const decKey = getPrivateKey();

    m_decompressCryptoFuture = std::async(std::launch::async, [this, verify, decrypt, source, dest, preserveLinks, decKey]() {
        bool ok = true;

        if (verify) {
            std::ifstream sigFile(source + ".sig", std::ios::binary);
            std::ostringstream sigContent;
            sigContent << sigFile.rdbuf();

            ok = m_signer->verifyFile(source, sigContent.str());

            if (!ok)
                setErrorString("Signature verification rejected the asset.");
        }

        if (ok) {
            if (decrypt) {
                m_decompressCrypto->setInput(source);
                m_decompressCrypto->setOutput(dest);
                m_decompressCrypto->setPreserveSymlinks(preserveLinks);
                m_decompressCrypto->setDecryptionKey(decKey);

                ok = m_decompressCrypto->execute();
                if (!ok)
                    setErrorString(m_decompressCrypto->errorString());
            } else {
                m_decompress->setInput(source);
                m_decompress->setOutput(dest);
                m_decompress->setPreserveSymlinks(preserveLinks);

                ok = m_decompress->execute();
                if (!ok)
                    setErrorString(m_decompress->errorString());
            }
        }

        notifyFinished();
        return ok;
    });
}

} // namespace job::zstd