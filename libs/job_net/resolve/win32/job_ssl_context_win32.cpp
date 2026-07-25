#include "resolve/job_ssl_context.h"
#include "resolve/job_ssl_error.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincrypt.h>
#include <ncrypt.h>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <security.h>
#include <schannel.h>

#include <vector>
#include <string>
#include <cstring>
#include <job_logger.h>

namespace job::net {

struct JobSslContext::Impl {
    CredHandle credHandle{};
    bool hasCredHandle{false};
    bool isDirty{true}; // Indicates credentials handle needs reacquisition

    HCERTSTORE certStore{nullptr};
    PCCERT_CONTEXT certContext{nullptr};

    SslMode mode{SslMode::Client};
    VerifyMode verifyMode{VerifyMode::None};
    CertVersion certVersion{CertVersion::Cert_V1_2_OR_LATER};
    std::vector<std::string> alpnProtocols;

    JobSslError error;

    ~Impl()
    {
        cleanup();
    }

    void cleanup() noexcept
    {
        if (certContext) {
            CertFreeCertificateContext(certContext);
            certContext = nullptr;
        }
        if (certStore) {
            CertCloseStore(certStore, 0);
            certStore = nullptr;
        }
        if (hasCredHandle) {
            FreeCredentialsHandle(&credHandle);
            hasCredHandle = false;
        }
    }

    // Helper to read files safely and map Win32 errors to JobSslError
    static bool readFileToBuffer(const std::string &path, std::vector<BYTE> &buffer, JobSslError &err)
    {
        HANDLE hFile = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD dwErr = ::GetLastError();
            err.recordError(JobSslError::SslErrNo::Syscall, "Failed to open file: " + path);
            JOB_LOG_ERROR("[JobSslContext] CreateFileA failed for '{}': Win32 Error {}", path, dwErr);
            return false;
        }

        DWORD fileSize = ::GetFileSize(hFile, nullptr);
        if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
            ::CloseHandle(hFile);
            err.recordError(JobSslError::SslErrNo::Syscall, "Invalid or empty file size: " + path);
            return false;
        }

        buffer.resize(fileSize);
        DWORD bytesRead = 0;
        BOOL success = ::ReadFile(hFile, buffer.data(), fileSize, &bytesRead, nullptr);
        ::CloseHandle(hFile);

        if (!success || bytesRead != fileSize) {
            err.recordError(JobSslError::SslErrNo::Syscall, "Incomplete read for file: " + path);
            JOB_LOG_ERROR("[JobSslContext] ReadFile failed or incomplete for '{}'", path);
            return false;
        }

        return true;
    }

    // Dynamic Acquisition of Schannel CredHandle
    bool rebuildCredentialsHandle()
    {
        SCH_CREDENTIALS creds{};
        creds.dwVersion = SCH_CREDENTIALS_VERSION;
        creds.dwFlags = SCH_USE_STRONG_CRYPTO;

        if (mode == SslMode::Client)
            creds.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

        // VerifyMode::None means "don't validate the peer's certificate at all" --
        // Schannel's default behavior is automatic chain validation, so disabling
        // that requires SCH_CRED_MANUAL_CRED_VALIDATION here. The caller (ssl_socket.h)
        // must then simply skip calling its own manual verification step after the
        // handshake, which reproduces OpenSSL's SSL_VERIFY_NONE behavior. Peer/RequirePeer
        // both leave Schannel's automatic validation on (no flag needed, it's the default).
        // NOTE: RequirePeer's mutual-auth requirement (asking for + requiring a client cert
        // on the server side) is a separate knob -- ASC_REQ_MUTUAL_AUTH at the
        // AcceptSecurityContext call site in ssl_socket.h, not something set here at
        // credential-acquisition time.
        if (verifyMode == VerifyMode::None)
            creds.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

        PCCERT_CONTEXT certificate = certContext;

        if (certificate) {
            creds.cCreds = 1;
            creds.paCred = &certificate;
        }

        TLS_PARAMETERS tlsParameters{};
        tlsParameters.cAlpnIds = 0;
        tlsParameters.rgstrAlpnIds = nullptr;
        tlsParameters.grbitDisabledProtocols =
            protocolMaskForMode();
        tlsParameters.cDisabledCrypto = 0;
        tlsParameters.pDisabledCrypto = nullptr;
        tlsParameters.dwFlags = 0;

        // Cert_ALL means use the system protocol defaults. In that case, TLS_PARAMETERS is not needed.
        if (certVersion != CertVersion::Cert_ALL) {
            creds.cTlsParameters = 1;
            creds.pTlsParameters = &tlsParameters;
        }

        CredHandle newHandle{};
        TimeStamp expiry{};

        const SECURITY_STATUS status =
            ::AcquireCredentialsHandleA(
                nullptr,
                const_cast<SEC_CHAR *>(UNISP_NAME_A),
                mode == SslMode::Client
                    ? SECPKG_CRED_OUTBOUND
                    : SECPKG_CRED_INBOUND,
                nullptr,
                &creds,
                nullptr,
                nullptr,
                &newHandle,
                &expiry
                );

        if (status != SEC_E_OK) {
            error.recordNativeError(
                static_cast<int>(status)
                );

            JOB_LOG_ERROR(
                "[JobSslContext] AcquireCredentialsHandleA "
                "failed: {}",
                error.lastErrorString()
                );

            return false;
        }

        if (hasCredHandle)
            ::FreeCredentialsHandle(&credHandle);

        credHandle = newHandle;
        hasCredHandle = true;
        isDirty = false;

        return true;
    }
    static bool parseCaBufferToStore(const std::vector<BYTE> &buffer, HCERTSTORE hTargetStore, JobSslError &err) {
        if (buffer.empty() || !hTargetStore)
            return false;

        DWORD dwEncoding = 0;
        DWORD dwContentType = 0;
        DWORD dwFormatType = 0;
        HCERTSTORE hMsgStore = nullptr;

        CERT_BLOB blob{};
        blob.cbData = static_cast<DWORD>(buffer.size());
        blob.pbData = const_cast<BYTE*>(buffer.data());

        BOOL queryResult = ::CryptQueryObject(
            CERT_QUERY_OBJECT_BLOB,
            &blob,
            CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED |
                CERT_QUERY_CONTENT_FLAG_PKCS7_UNSIGNED |
                CERT_QUERY_CONTENT_FLAG_CERT |
                CERT_QUERY_CONTENT_FLAG_SERIALIZED_STORE,
            CERT_QUERY_FORMAT_FLAG_ALL,
            0,
            &dwEncoding,
            &dwContentType,
            &dwFormatType,
            &hMsgStore,
            nullptr,
            nullptr
            );

        if (queryResult && hMsgStore) {
            PCCERT_CONTEXT pCert = nullptr;
            while ((pCert = ::CertEnumCertificatesInStore(hMsgStore, pCert)) != nullptr)
                ::CertAddCertificateContextToStore(hTargetStore, pCert, CERT_STORE_ADD_REPLACE_EXISTING, nullptr);
            ::CertCloseStore(hMsgStore, 0);
            return true;
        }

        const char *pStr = reinterpret_cast<const char *>(buffer.data());
        DWORD strLen = static_cast<DWORD>(buffer.size());
        DWORD certCount = 0;

        DWORD skipBytes = 0;
        while (skipBytes < strLen) {
            DWORD derLen = 0;
            DWORD skipOut = 0;

            if (!::CryptStringToBinaryA(
                    pStr + skipBytes, strLen - skipBytes,
                    CRYPT_STRING_BASE64HEADER,
                    nullptr, &derLen,
                    nullptr, nullptr)) {
                break;
            }

            std::vector<BYTE> derBuffer(derLen);
            if (::CryptStringToBinaryA(
                    pStr + skipBytes, strLen - skipBytes,
                    CRYPT_STRING_BASE64HEADER,
                    derBuffer.data(), &derLen,
                    &skipOut, nullptr)) {

                if (::CertAddEncodedCertificateToStore(
                        hTargetStore,
                        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
                        derBuffer.data(), derLen,
                        CERT_STORE_ADD_REPLACE_EXISTING,
                        nullptr)) {
                    certCount++;
                }
                skipBytes += skipOut;
            } else {
                break;
            }
        }

        if (certCount > 0)
            return true;

        err.recordError(JobSslError::SslErrNo::CertificateVerifyFailed, "Failed to parse CA certificate file in PEM, DER, or PKCS7 format");
        return false;
    }

    [[nodiscard]] DWORD protocolMaskForMode() const noexcept
    {
        const DWORD tls10 =
            mode == SslMode::Client
                ? SP_PROT_TLS1_0_CLIENT
                : SP_PROT_TLS1_0_SERVER;

        const DWORD tls11 =
            mode == SslMode::Client
                ? SP_PROT_TLS1_1_CLIENT
                : SP_PROT_TLS1_1_SERVER;

        const DWORD tls12 =
            mode == SslMode::Client
                ? SP_PROT_TLS1_2_CLIENT
                : SP_PROT_TLS1_2_SERVER;

        const DWORD tls13 =
            mode == SslMode::Client
                ? SP_PROT_TLS1_3_CLIENT
                : SP_PROT_TLS1_3_SERVER;

        const DWORD allTls =
            tls10 |
            tls11 |
            tls12 |
            tls13;

        DWORD enabledProtocols = 0;

        switch (certVersion) {
        case CertVersion::Cert_V1_0:
            enabledProtocols = tls10;
            break;

        case CertVersion::Cert_V1_1:
            enabledProtocols = tls11;
            break;

        case CertVersion::Cert_V1_2:
            enabledProtocols = tls12;
            break;

        case CertVersion::Cert_V1_3:
            enabledProtocols = tls13;
            break;

        case CertVersion::Cert_V1_0_OR_LATER:
            enabledProtocols =
                tls10 |
                tls11 |
                tls12 |
                tls13;
            break;

        case CertVersion::Cert_V1_1_OR_LATER:
            enabledProtocols =
                tls11 |
                tls12 |
                tls13;
            break;

        case CertVersion::Cert_V1_2_OR_LATER:
        case CertVersion::Cert_SECURE_PROTOCOLS:
            enabledProtocols =
                tls12 |
                tls13;
            break;

        case CertVersion::Cert_V1_3_Or_Later:
            enabledProtocols = tls13;
            break;

        case CertVersion::Cert_ALL:
            // A disabled mask of zero delegates protocol selection to the Windows Schannel system policy.
            return 0;

        default:
            return 0;
        }
        return allTls & ~enabledProtocols;
    }
};

JobSslContext::JobSslContext(SslMode mode):
    m_impl(std::make_unique<Impl>())
{
    m_impl->mode = mode;
    m_impl->isDirty = true;
}

JobSslContext::~JobSslContext() = default;

JobSslContext::JobSslContext(JobSslContext &&other) noexcept = default;

JobSslContext &JobSslContext::operator=(JobSslContext &&other) noexcept = default;

bool JobSslContext::setProtocol(CertVersion version)
{
    if (!m_impl)
        return false;

    if (m_impl->certVersion == version)
        return true;

    m_impl->certVersion = version;
    m_impl->isDirty = true;

    return true;
}

void JobSslContext::setOption(CertOptions /*option*/, bool /*on*/)
{
    JOB_LOG_WARN("[JobSslContext] setOption is a no-op on Windows. Schannel manages session caching, "
                 "fragmentation, and compression automatically at the OS level.");
}

bool JobSslContext::loadSystemCertificates()
{
    if (!m_impl)
        return false;

    HCERTSTORE hStore = ::CertOpenSystemStoreA(0, "ROOT");
    if (!hStore) {
        DWORD dwErr = ::GetLastError();
        m_impl->error.recordError(JobSslError::SslErrNo::Syscall, "Failed to open Windows ROOT store");
        JOB_LOG_ERROR("[JobSslContext] Failed to open Windows System Certificate Store: Win32 Err {}", dwErr);
        return false;
    }

    if (m_impl->certStore)
        ::CertCloseStore(m_impl->certStore, 0);

    m_impl->certStore = hStore;
    return true;
}

bool JobSslContext::loadCaCertificateFile(const std::string &path) {
    if (!m_impl)
        return false;

    std::vector<BYTE> buffer;
    if (!Impl::readFileToBuffer(path, buffer, m_impl->error))
        return false;

    HCERTSTORE hNewStore = ::CertOpenStore(
        CERT_STORE_PROV_MEMORY,
        0,
        0,
        0,
        nullptr
        );

    if (!hNewStore) {
        DWORD dwErr = ::GetLastError();
        m_impl->error.recordError(JobSslError::SslErrNo::InternalError, "Failed to create in-memory certificate store");
        JOB_LOG_ERROR("[JobSslContext] CertOpenStore(MEMORY) failed: Win32 Err {}", dwErr);
        return false;
    }

    if (!Impl::parseCaBufferToStore(buffer, hNewStore, m_impl->error)) {
        ::CertCloseStore(hNewStore, 0);
        JOB_LOG_ERROR("[JobSslContext] Failed to parse CA certificate bundle from '{}'", path);
        return false;
    }

    if (m_impl->certStore)
        ::CertCloseStore(m_impl->certStore, 0);

    m_impl->certStore = hNewStore;
    return true;
}

bool JobSslContext::loadCertificateFile(const std::string &path, EncodingType format)
{
    if (!m_impl)
        return false;

    std::vector<BYTE> buffer;
    if (!Impl::readFileToBuffer(path, buffer, m_impl->error))
        return false;

    const BYTE *pDerData = buffer.data();
    DWORD derSize = static_cast<DWORD>(buffer.size());
    std::vector<BYTE> derBuffer;

    if (format == EncodingType::PEM) {
        const char *pPemStr = reinterpret_cast<const char *>(buffer.data());
        DWORD strLen = static_cast<DWORD>(buffer.size());
        DWORD decodedLen = 0;

        if (::CryptStringToBinaryA(
                pPemStr, strLen,
                CRYPT_STRING_BASE64HEADER,
                nullptr, &decodedLen,
                nullptr, nullptr)) {

            derBuffer.resize(decodedLen);
            if (::CryptStringToBinaryA(
                    pPemStr, strLen,
                    CRYPT_STRING_BASE64HEADER,
                    derBuffer.data(), &decodedLen,
                    nullptr, nullptr)) {

                pDerData = derBuffer.data();
                derSize = decodedLen;
            } else {
                m_impl->error.recordError(JobSslError::SslErrNo::CertificateVerifyFailed, "Failed to decode Base64 PEM certificate string");
                JOB_LOG_ERROR("[JobSslContext] CryptStringToBinaryA decode failed for '{}'", path);
                return false;
            }
        } else {
            JOB_LOG_WARN("[JobSslContext] PEM header parsing failed for '{}', attempting raw DER parse fallback", path);
        }
    }

    PCCERT_CONTEXT pCert = ::CertCreateCertificateContext(
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pDerData,
        derSize
        );

    if (!pCert) {
        DWORD dwErr = ::GetLastError();
        m_impl->error.recordError(JobSslError::SslErrNo::CertificateVerifyFailed, "Failed to parse certificate context from DER payload");
        JOB_LOG_ERROR("[JobSslContext] CertCreateCertificateContext failed for '{}': Win32 Err {}", path, dwErr);
        return false;
    }

    if (m_impl->certContext)
        ::CertFreeCertificateContext(m_impl->certContext);

    m_impl->certContext = pCert;
    m_impl->isDirty = true; // Local identity certificate changed -> CredHandle is now stale
    return true;
}

bool JobSslContext::loadPrivateKeyFile(const std::string &path, EncodingType /*format*/, const std::string &/*passphrase*/)
{
    JOB_LOG_WARN("[JobSslContext] loadPrivateKeyFile('{}') called on Windows Schannel. "
                 "Standalone private key loading is not supported; Schannel requires private keys to be bound "
                 "directly to the certificate context (PCCERT_CONTEXT) or imported via PFX/Cert Store. "
                 "Use loadIdentityFile() instead.", path);

    if (m_impl) {
        m_impl->error.recordError(
            JobSslError::SslErrNo::OperationNotSupported,
            "Standalone private key file loading is not supported under Windows Schannel. Use loadIdentityFile()."
            );
    }

    return false;
}

bool JobSslContext::loadIdentityFile(const std::string &path, const std::string &passphrase)
{
    if (!m_impl)
        return false;

    std::vector<BYTE> buffer;
    if (!Impl::readFileToBuffer(path, buffer, m_impl->error))
        return false;

    CRYPT_DATA_BLOB pfxBlob{};
    pfxBlob.cbData = static_cast<DWORD>(buffer.size());
    pfxBlob.pbData = buffer.data();

    // PFXImportCertStore wants the password as a wide string, unlike most of the
    // ANSI ("A") APIs used elsewhere in this file.
    std::wstring widePassphrase;
    if (!passphrase.empty()) {
        int wideLen = ::MultiByteToWideChar(CP_UTF8, 0, passphrase.c_str(), -1, nullptr, 0);
        if (wideLen > 0) {
            widePassphrase.resize(static_cast<size_t>(wideLen) - 1); // exclude the null terminator MultiByteToWideChar counts
            ::MultiByteToWideChar(CP_UTF8, 0, passphrase.c_str(), -1, widePassphrase.data(), wideLen);
        }
    }

    // CRYPT_EXPORTABLE: the imported key must be usable by AcquireCredentialsHandleA
    // afterward, not locked to a non-exportable container.
    // PKCS12_NO_PERSIST_KEY: keeps the imported key ephemeral (process memory only)
    // rather than writing it into the user's persisted key store on disk. Requires a
    // CNG-capable provider. Aavailable since Vista, so should be safe for any target
    // JOB cares about, but not yet exercised on real Windows hardware.
    const DWORD importFlags = CRYPT_EXPORTABLE | PKCS12_NO_PERSIST_KEY;

    HCERTSTORE pfxStore = ::PFXImportCertStore(
        &pfxBlob,
        widePassphrase.empty() ? nullptr : widePassphrase.c_str(),
        importFlags
        );

    if (!pfxStore) {
        DWORD dwErr = ::GetLastError();
        m_impl->error.recordError(JobSslError::SslErrNo::CertificateVerifyFailed,
                                  "PFXImportCertStore failed for: " + path);
        JOB_LOG_ERROR("[JobSslContext] PFXImportCertStore failed for '{}': Win32 Err {}", path, dwErr);
        return false;
    }

    // A PFX can contain extra certs (e.g. chain members) without private keys attached --
    // walk the store to find the one that actually carries a usable private key.
    PCCERT_CONTEXT pIdentityCert = nullptr;
    PCCERT_CONTEXT pCurrent = nullptr;

    while ((pCurrent = ::CertEnumCertificatesInStore(pfxStore, pCurrent)) != nullptr) {
        HCRYPTPROV_OR_NCRYPT_KEY_HANDLE hKey = 0;
        DWORD keySpec = 0;
        BOOL callerFreesKey = FALSE;

        if (::CryptAcquireCertificatePrivateKey(
                pCurrent,
                CRYPT_ACQUIRE_CACHE_FLAG | CRYPT_ACQUIRE_COMPARE_KEY_FLAG,
                nullptr, &hKey, &keySpec, &callerFreesKey)) {
            pIdentityCert = ::CertDuplicateCertificateContext(pCurrent);
            if (callerFreesKey && keySpec == CERT_NCRYPT_KEY_SPEC)
                ::NCryptFreeObject(reinterpret_cast<NCRYPT_HANDLE>(hKey));
            ::CertFreeCertificateContext(pCurrent);
            break;
        }
    }

    ::CertCloseStore(pfxStore, 0);

    if (!pIdentityCert) {
        m_impl->error.recordError(JobSslError::SslErrNo::CertificateVerifyFailed,
                                  "PFX file contains no certificate with an associated private key: " + path);
        JOB_LOG_ERROR("[JobSslContext] No private-key-bearing certificate found in '{}'", path);
        return false;
    }

    if (m_impl->certContext)
        ::CertFreeCertificateContext(m_impl->certContext);

    m_impl->certContext = pIdentityCert;
    m_impl->isDirty = true; // Identity changed -> CredHandle must be reacquired
    return true;
}

void JobSslContext::setVerifyMode(VerifyMode mode)
{
    if (!m_impl)
        return;

    m_impl->verifyMode = mode;
    m_impl->isDirty = true; // verify mode affects SCH_CRED_MANUAL_CRED_VALIDATION at acquisition time
}

JobSslContext::VerifyMode JobSslContext::verifyMode() const noexcept
{
    return m_impl ? m_impl->verifyMode : VerifyMode::None;
}

void JobSslContext::setAlpnProtocols(const std::vector<std::string> &protocols)
{
    if (!m_impl)
        return;

    m_impl->alpnProtocols.clear();
    m_impl->alpnProtocols.reserve(protocols.size());

    for (const auto &protocol : protocols) {
        if (protocol.empty() || protocol.size() > 255)
            continue;
        m_impl->alpnProtocols.push_back(protocol);
    }
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
    return m_impl != nullptr;
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
    if (!m_impl)
        return false;

    if (!m_impl->isDirty && m_impl->hasCredHandle)
        return true;

    return m_impl->rebuildCredentialsHandle();
}

void *JobSslContext::nativeHandle() const
{
    if (!m_impl || m_impl->isDirty || !m_impl->hasCredHandle)
        return nullptr;

    return static_cast<void *>(&m_impl->credHandle);
}

} // namespace job::net