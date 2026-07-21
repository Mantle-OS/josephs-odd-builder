#include "job_ssl_cert.h"
#include <job_logger.h>

namespace job::net::ssl {

// Static initialization helper
static void ensureOpenSSL() {
    static std::once_flag s_flag;
    std::call_once(s_flag, []() {
        OPENSSL_init_ssl(0, NULL);
    });
}

JobSslCert::JobSslCert()
{
    ensureOpenSSL();
    // Default to TLS_client_method which auto-negotiates the highest version
    const SSL_METHOD* method = TLS_client_method();
    m_ctx = SSL_CTX_new(method);

    if (!m_ctx) {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        m_lastError = buf;
        JOB_LOG_ERROR("[JobSslCert] Failed to create context: {}", m_lastError);
    } else {
        // Safe defaults
        setProtocol(CertVersion::Cert_V1_2_OR_LATER);
        // Load system CA certs by default so we can talk to Hugging Face
        SSL_CTX_set_default_verify_paths(m_ctx);
    }
}

JobSslCert::JobSslCert(const std::filesystem::path &certPath,
                       const std::filesystem::path &keyPath,
                       EncodingType format) :
    JobSslCert()
{
    if (!loadCertificate(certPath.string(), format))
        JOB_LOG_ERROR("[JobSslCert] Failed to load cert: {}", certPath.string());

    if (!loadPrivateKey(keyPath.string(), format))
        JOB_LOG_ERROR("[JobSslCert] Failed to load key: {}", keyPath.string());
}

JobSslCert::~JobSslCert()
{
    if (m_ctx)
        SSL_CTX_free(m_ctx);
}

JobSslCert::JobSslCert(JobSslCert &&other) noexcept
{
    m_ctx = other.m_ctx;
    other.m_ctx = nullptr;
    m_lastError = std::move(other.m_lastError);
}

JobSslCert &JobSslCert::operator=(JobSslCert &&other) noexcept
{
    if (this != &other) {
        if (m_ctx)
            SSL_CTX_free(m_ctx);

        m_ctx = other.m_ctx;
        other.m_ctx = nullptr;
        m_lastError = std::move(other.m_lastError);
    }
    return *this;
}

void JobSslCert::initOpenSSL()
{
    ensureOpenSSL();
}

bool JobSslCert::isValid() const
{
    return m_ctx != nullptr;
}

std::string JobSslCert::getError() const
{
    return m_lastError;
}

bool JobSslCert::setProtocol(CertVersion version)
{
    if (!m_ctx) return false;

    // OpenSSL 1.1+ uses _min_proto_version / _max_proto_version
    // We map your enum to these constants.

    int minVer = 0;
    int maxVer = 0;

    switch(version) {
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
        minVer  = TLS1_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_1_OR_LATER:
        minVer = TLS1_1_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_2_OR_LATER:
        minVer = TLS1_2_VERSION; maxVer = 0;
        break;
    case CertVersion::Cert_V1_3_Or_Later:
        minVer = TLS1_3_VERSION; maxVer = 0;
        break;

    case CertVersion::Cert_SECURE_PROTOCOLS:
        minVer = TLS1_2_VERSION;
        maxVer = 0;
        break;

    default: break;
    }

    if (minVer != 0)
        SSL_CTX_set_min_proto_version(m_ctx, minVer);

    if (maxVer != 0)
        SSL_CTX_set_max_proto_version(m_ctx, maxVer);

    return true;
}

void JobSslCert::setOption(CertOptions option, bool on)
{
    if (!m_ctx) return;

    long optMask = 0;

    switch(option) {
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
    case CertOptions::NoServerNameIndication:
    case CertOptions::NoSessionSharding:
    case CertOptions::NoServerCipher:
        return;

    default:
        return;
    }

    if (on) {
        SSL_CTX_set_options(m_ctx, optMask);
    } else {
        SSL_CTX_clear_options(m_ctx, optMask);
    }
}

bool JobSslCert::loadCertificate(const std::string &path, EncodingType format)
{
    if (!m_ctx)
        return false;

    int type = (format == EncodingType::PEM) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;

    if (SSL_CTX_use_certificate_file(m_ctx, path.c_str(), type) <= 0) {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        m_lastError = buf;
        return false;
    }
    return true;
}

bool JobSslCert::loadPrivateKey(const std::string &path, EncodingType format)
{
    if (!m_ctx)
        return false;

    int type = (format == EncodingType::PEM) ? SSL_FILETYPE_PEM : SSL_FILETYPE_ASN1;

    if (SSL_CTX_use_PrivateKey_file(m_ctx, path.c_str(), type) <= 0) {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        m_lastError = buf;
        return false;
    }

    if (!SSL_CTX_check_private_key(m_ctx)) {
        m_lastError = "Private key does not match the certificate public key";
        return false;
    }
    return true;
}

bool JobSslCert::loadCaBundle(const std::string &path)
{
    if (!m_ctx)
        return false;
    if (SSL_CTX_load_verify_locations(m_ctx, path.c_str(), nullptr) <= 0) {
        char buf[256];
        ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
        m_lastError = buf;
        return false;
    }
    return true;
}

} // namespace job::net::ssl
