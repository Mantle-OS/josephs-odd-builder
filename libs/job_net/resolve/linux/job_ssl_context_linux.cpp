#include "resolve/job_ssl_context.h"
#include "resolve/job_ssl_error.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <mutex>
#include <vector>
#include <cstring>
#include <algorithm>
#include <filesystem>

#include <job_logger.h>

namespace job::net {

struct JobSslContext::Impl
{
    SSL_CTX *ctx{nullptr};
    SslMode mode{SslMode::Client};
    VerifyMode verifyMode{VerifyMode::None};
    CertVersion certVersion{CertVersion::Cert_V1_2_OR_LATER};
    std::vector<std::string> alpnProtocols;
    std::vector<unsigned char> alpnWireBuf; // Wire-format cache for ALPN negotiation
    JobSslError error;

    // --- Private Static Internal Helpers ---
    static void ensureOpenSSL()
    {
        static std::once_flag s_flag;
        std::call_once(s_flag, []() {
            OPENSSL_init_ssl(0, nullptr);
            OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr);
        });
    }

    // Pulls and formats errors directly from the thread-local OpenSSL queue
    void recordOpenSslError(const std::string &contextMsg)
    {
        unsigned long errCode = ::ERR_get_error();
        if (errCode != 0) {
            char buf[256]{};
            ::ERR_error_string_n(errCode, buf, sizeof(buf));
            std::string fullMsg = contextMsg.empty() ? buf : (contextMsg + ": " + buf);
            error.recordError(JobSslError::SslErrNo::HandshakeFailed, fullMsg);
        } else {
            error.recordError(JobSslError::SslErrNo::InternalError, contextMsg.empty() ? "Unknown OpenSSL Error" : contextMsg);
        }
    }

    static bool checkFileExists(const std::string &path)
    {
        std::error_code ec;
        return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
    }

    static int pemPasswordCallback(char *buf, int size, int /*rwflag*/, void *userdata)
    {
        if (!userdata || size <= 0)
            return 0;

        auto *pass = static_cast<const std::string *>(userdata);
        if (pass->empty())
            return 0;

        int copyLen = static_cast<int>(pass->size());
        if (copyLen >= size)
            copyLen = size - 1;

        std::memcpy(buf, pass->data(), copyLen);
        buf[copyLen] = '\0';
        return copyLen;
    }

    static int alpnServerSelectCallback(SSL * /*ssl*/,
                                        const unsigned char **out,
                                        unsigned char *outlen,
                                        const unsigned char *in,
                                        unsigned int inlen,
                                        void *arg)
    {
        auto *serverProtocolsWire = static_cast<const std::vector<unsigned char> *>(arg);
        if (!serverProtocolsWire || serverProtocolsWire->empty())
            return SSL_TLSEXT_ERR_NOACK;

        int status = ::SSL_select_next_proto(
            const_cast<unsigned char **>(out), outlen,
            serverProtocolsWire->data(), static_cast<unsigned int>(serverProtocolsWire->size()),
            in, inlen
            );

        if (status != OPENSSL_NPN_NEGOTIATED)
            return SSL_TLSEXT_ERR_NOACK;

        return SSL_TLSEXT_ERR_OK;
    }
};

JobSslContext::JobSslContext(SslMode mode):
    m_impl(std::make_unique<Impl>())
{
    Impl::ensureOpenSSL();

    ::ERR_clear_error();
    m_impl->mode = mode;
    const SSL_METHOD *method = (mode == SslMode::Client)
                                   ? ::TLS_client_method()
                                   : ::TLS_server_method();

    m_impl->ctx = ::SSL_CTX_new(method);
    if (!m_impl->ctx) {
        m_impl->recordOpenSslError("Failed to create SSL_CTX");
        JOB_LOG_ERROR("[JobSslContext] Failed to create SSL_CTX: {}", m_impl->error.lastErrorString());
        return;
    }

    // Apply baseline default settings
    setProtocol(CertVersion::Cert_V1_2_OR_LATER);
    setVerifyMode(VerifyMode::None);
}

JobSslContext::~JobSslContext()
{
    if (m_impl && m_impl->ctx) {
        ::SSL_CTX_free(m_impl->ctx);
        m_impl->ctx = nullptr;
    }
}

JobSslContext::JobSslContext(JobSslContext &&other) noexcept = default;

JobSslContext &JobSslContext::operator=(JobSslContext &&other) noexcept = default;

bool JobSslContext::setProtocol(CertVersion version)
{
    if (!isValid())
        return false;

    ::ERR_clear_error();
    m_impl->certVersion = version;

    int minVer = 0;
    int maxVer = 0;

    switch (version) {
    case CertVersion::Cert_V1_0:
        minVer = maxVer = TLS1_VERSION;
        break;
    case CertVersion::Cert_V1_1:
        minVer = maxVer = TLS1_1_VERSION;
        break;
    case CertVersion::Cert_V1_2:
        minVer = maxVer = TLS1_2_VERSION;
        break;
    case CertVersion::Cert_V1_3:
        minVer = maxVer = TLS1_3_VERSION;
        break;

    case CertVersion::Cert_V1_0_OR_LATER:
        minVer = TLS1_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_1_OR_LATER:
        minVer = TLS1_1_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_2_OR_LATER:
    case CertVersion::Cert_SECURE_PROTOCOLS:
        minVer = TLS1_2_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_3_Or_Later:
        minVer = TLS1_3_VERSION; maxVer = 0;
        break;

    case CertVersion::Cert_ALL:
        minVer = 0; maxVer = 0;
        break;

    default:
        break;
    }

    if (minVer != 0 && ::SSL_CTX_set_min_proto_version(m_impl->ctx, minVer) <= 0) {
        m_impl->recordOpenSslError("Failed to set minimum TLS proto version");
        return false;
    }

    if (maxVer != 0 && ::SSL_CTX_set_max_proto_version(m_impl->ctx, maxVer) <= 0) {
        m_impl->recordOpenSslError("Failed to set maximum TLS proto version");
        return false;
    }

    return true;
}

void JobSslContext::setOption(CertOptions option, bool on)
{
    if (!isValid())
        return;

    uint64_t optMask = 0;

    switch (option) {
    case CertOptions::NoFragments:
        optMask = SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
        break;
    case CertOptions::NoSessionTickets:
        optMask = SSL_OP_NO_TICKET;
        break;
    case CertOptions::NoCompression:
        optMask = SSL_OP_NO_COMPRESSION;
        break;
    case CertOptions::NoLegacyRegen:
        optMask = SSL_OP_NO_RENEGOTIATION;
        break;
    case CertOptions::NoSessionPersistence:
        optMask = SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION;
        break;
    default:
        return;
    }

    if (on)
        ::SSL_CTX_set_options(m_impl->ctx, optMask);
    else
        ::SSL_CTX_clear_options(m_impl->ctx, optMask);
}

bool JobSslContext::loadSystemCertificates()
{
    if (!isValid())
        return false;

    ::ERR_clear_error();
    if (::SSL_CTX_set_default_verify_paths(m_impl->ctx) <= 0) {
        m_impl->recordOpenSslError("Failed to load system default CA paths");
        JOB_LOG_ERROR("[JobSslContext] Failed to load system CA paths: {}", m_impl->error.lastErrorString());
        return false;
    }
    return true;
}

bool JobSslContext::loadCaCertificateFile(const std::string &path)
{
    if (!isValid())
        return false;

    if (!Impl::checkFileExists(path)) {
        m_impl->error.recordError(JobSslError::SslErrNo::Syscall, "CA bundle file does not exist: " + path);
        JOB_LOG_ERROR("[JobSslContext] CA file does not exist at '{}'", path);
        return false;
    }

    ::ERR_clear_error();
    if (::SSL_CTX_load_verify_locations(m_impl->ctx, path.c_str(), nullptr) <= 0) {
        m_impl->recordOpenSslError("Failed to load CA bundle file");
        JOB_LOG_ERROR("[JobSslContext] Failed to load CA bundle '{}': {}", path, m_impl->error.lastErrorString());
        return false;
    }
    return true;
}

bool JobSslContext::loadCertificateFile(const std::string &path, EncodingType format)
{
    if (!isValid())
        return false;

    if (!Impl::checkFileExists(path)) {
        m_impl->error.recordError(JobSslError::SslErrNo::Syscall, "Certificate file does not exist: " + path);
        JOB_LOG_ERROR("[JobSslContext] Certificate file does not exist at '{}'", path);
        return false;
    }

    int fileType = (format == EncodingType::PEM) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;

    ::ERR_clear_error();
    if (format == EncodingType::PEM && ::SSL_CTX_use_certificate_chain_file(m_impl->ctx, path.c_str()) > 0) {
        return true;
    }

    ::ERR_clear_error();
    if (::SSL_CTX_use_certificate_file(m_impl->ctx, path.c_str(), fileType) <= 0) {
        m_impl->recordOpenSslError("Failed to load certificate file");
        JOB_LOG_ERROR("[JobSslContext] Failed to load certificate '{}': {}", path, m_impl->error.lastErrorString());
        return false;
    }
    return true;
}

bool JobSslContext::loadPrivateKeyFile(const std::string &path, EncodingType format, const std::string &passphrase)
{
    if (!isValid())
        return false;

    if (!Impl::checkFileExists(path)) {
        m_impl->error.recordError(JobSslError::SslErrNo::Syscall, "Private key file does not exist: " + path);
        JOB_LOG_ERROR("[JobSslContext] Private key file does not exist at '{}'", path);
        return false;
    }

    int fileType = (format == EncodingType::PEM) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;

    if (!passphrase.empty()) {
        ::SSL_CTX_set_default_passwd_cb(m_impl->ctx, Impl::pemPasswordCallback);
        ::SSL_CTX_set_default_passwd_cb_userdata(m_impl->ctx, const_cast<std::string *>(&passphrase));
    }

    ::ERR_clear_error();
    int rc = ::SSL_CTX_use_PrivateKey_file(m_impl->ctx, path.c_str(), fileType);

    if (!passphrase.empty()) {
        ::SSL_CTX_set_default_passwd_cb(m_impl->ctx, nullptr);
        ::SSL_CTX_set_default_passwd_cb_userdata(m_impl->ctx, nullptr);
    }

    if (rc <= 0) {
        m_impl->recordOpenSslError("Failed to load private key file");
        JOB_LOG_ERROR("[JobSslContext] Failed to load private key '{}': {}", path, m_impl->error.lastErrorString());
        return false;
    }

    if (!::SSL_CTX_check_private_key(m_impl->ctx)) {
        m_impl->error.recordError(JobSslError::SslErrNo::CertificateVerifyFailed, "Private key does not match certificate public key");
        JOB_LOG_ERROR("[JobSslContext] Private key validation failed for '{}'", path);
        return false;
    }

    return true;
}

bool JobSslContext::loadIdentityFile(const std::string & /*path*/, const std::string & /*passphrase*/)
{
    // Not yet implemented on Linux: loadCertificateFile()+loadPrivateKeyFile() already
    // cover the cert+key case here via separate PEM files, which is OpenSSL's native
    // idiom (unlike Schannel, which requires a single PFX identity). If PKCS#12 support
    // is wanted later for parity with Windows, OpenSSL's own PKCS12 API (d2i_PKCS12,
    // PKCS12_parse) is the path — deliberately not stubbed in silently here so a caller
    // porting Windows-shaped code doesn't get a quiet no-op.
    JOB_LOG_ERROR("[JobSslContext] loadIdentityFile() is not implemented on Linux — "
                  "use loadCertificateFile() + loadPrivateKeyFile() instead");
    return false;
}

void JobSslContext::setVerifyMode(VerifyMode mode)
{
    if (!isValid())
        return;

    m_impl->verifyMode = mode;
    int sslMode = SSL_VERIFY_NONE;

    switch (mode) {
    case VerifyMode::Peer:
        sslMode = SSL_VERIFY_PEER;
        break;
    case VerifyMode::RequirePeer:
        sslMode = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        break;
    case VerifyMode::None:
    default:
        sslMode = SSL_VERIFY_NONE;
        break;
    }

    ::SSL_CTX_set_verify(m_impl->ctx, sslMode, nullptr);
}
// HEADER
JobSslContext::VerifyMode JobSslContext::verifyMode() const noexcept
{
    return m_impl ? m_impl->verifyMode : VerifyMode::None;
}
// IMPLY
void JobSslContext::setAlpnProtocols(const std::vector<std::string> &protocols)
{
    if (!isValid())
        return;

    m_impl->alpnProtocols = protocols;
    m_impl->alpnWireBuf.clear();

    for (const auto &proto : protocols) {
        if (proto.empty() || proto.size() > 255)
            continue;
        m_impl->alpnWireBuf.push_back(static_cast<unsigned char>(proto.size()));
        m_impl->alpnWireBuf.insert(m_impl->alpnWireBuf.end(), proto.begin(), proto.end());
    }

    if (m_impl->alpnWireBuf.empty())
        return;

    if (m_impl->mode == SslMode::Client)
        ::SSL_CTX_set_alpn_protos(m_impl->ctx, m_impl->alpnWireBuf.data(), static_cast<unsigned int>(m_impl->alpnWireBuf.size()));
    else
        ::SSL_CTX_set_alpn_select_cb(m_impl->ctx, Impl::alpnServerSelectCallback, &m_impl->alpnWireBuf);
}

std::vector<std::string> JobSslContext::alpnProtocols() const
{
    return m_impl ? m_impl->alpnProtocols : std::vector<std::string>{};
}

JobSslContext::SslMode JobSslContext::mode() const noexcept
{
    return m_impl ? m_impl->mode : SslMode::Client;
}

bool JobSslContext::isValid() const noexcept
{
    return m_impl && m_impl->ctx != nullptr;
}

JobSslError::SslErrNo JobSslContext::lastError() const noexcept
{
    return m_impl ? m_impl->error.lastError() : JobSslError::SslErrNo::None;
}

std::string JobSslContext::lastErrorString() const
{
    return m_impl ? m_impl->error.lastErrorString() : "No error";
}

bool JobSslContext::ensureCredentials()
{
    return isValid();
}

void *JobSslContext::nativeHandle() const
{
    return m_impl ? static_cast<void *>(m_impl->ctx) : nullptr;
}

} // namespace job::net