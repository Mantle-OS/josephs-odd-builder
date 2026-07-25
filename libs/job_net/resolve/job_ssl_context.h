#pragma once
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "job_ssl_error.h"
#include "jobnet_export.h"
namespace job::net {
class JOBNET_EXPORT JobSslContext {
public:
    using Ptr = std::shared_ptr<JobSslContext>;
    enum class SslMode : uint8_t {
        Client = 0,
        Server
    };
    enum class SchemeType : uint8_t {
        Key,
        Socket,
        Cert,
        Elliptic,
        Hellman,
        Dtls,
        DtlsCookie
    };
    enum class EncodingType : uint8_t {
        PEM,
        DER
    };
    enum class CertAlgo : uint8_t {
        DSA,
        RSA,
        EC,
        DH,
        Opaque
    };
    enum class CertType : uint8_t {
        Private,
        Public
    };
    enum class CertOptions : uint8_t {
        NoFragments             = 0x01,
        NoSessionTickets        = 0x02,
        NoCompression           = 0x03,
        NoServerNameIndication  = 0x04,
        NoLegacyRegen           = 0x05,
        NoSessionSharding       = 0x06,
        NoSessionPersistence    = 0x07,
        NoServerCipher          = 0x08
    };
    enum class CertVersion : uint8_t {
        Cert_V1_0               = 0,
        Cert_V1_1               = 1,
        Cert_V1_2               = 2,
        Cert_V1_3               = 3,
        Cert_DTLS_V1_0          = 10,
        Cert_DTLS_V1_2          = 11,
        Cert_V1_0_OR_LATER      = 20,
        Cert_V1_1_OR_LATER      = 21,
        Cert_V1_2_OR_LATER      = 22,
        Cert_V1_3_Or_Later      = 23,
        Cert_DTLS_V1_0_OR_LATER = 33,
        Cert_DTLS_V1_2_OR_LATER = 34,
        Cert_ALL                = 40,
        Cert_SECURE_PROTOCOLS   = 41,
        Cert_UNKNOWN            = 255
    };
    enum class VerifyMode : uint8_t {
        None = 0,
        Peer,
        RequirePeer
    };
    explicit JobSslContext(SslMode mode = SslMode::Client);
    ~JobSslContext();
    JobSslContext(const JobSslContext &) = delete;
    JobSslContext &operator=(const JobSslContext &) = delete;
    JobSslContext(JobSslContext &&) noexcept;
    JobSslContext &operator=(JobSslContext &&) noexcept;
    bool setProtocol(CertVersion version);
    void setOption(CertOptions option, bool on);
    void setVerifyMode(VerifyMode mode);
    VerifyMode verifyMode() const noexcept;
    bool loadSystemCertificates();
    bool loadCaCertificateFile(const std::string &path);
    bool loadCertificateFile(const std::string &path, EncodingType format = EncodingType::PEM);
    bool loadPrivateKeyFile(const std::string &path, EncodingType format = EncodingType::PEM, const std::string &passphrase = {});

    // Loads a certificate + private key as one PKCS#12/PFX identity. This is Schannel's
    // native identity model on Windows.
    bool loadIdentityFile(const std::string &path, const std::string &passphrase = {});

    void setAlpnProtocols(const std::vector<std::string> &protocols);
    [[nodiscard]] std::vector<std::string> alpnProtocols() const;
    [[nodiscard]] SslMode mode() const noexcept;
    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] JobSslError::SslErrNo lastError() const noexcept;
    [[nodiscard]] std::string lastErrorString() const;
    [[nodiscard]] bool ensureCredentials();
    [[nodiscard]] void *nativeHandle() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace job::net