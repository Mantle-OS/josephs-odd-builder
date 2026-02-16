#pragma once

#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace job::net::ssl {

class JobSslCert {
public:
    using Ptr = std::shared_ptr<JobSslCert>;

    enum class SchemeType : uint8_t {
        Key,
        Socket,
        Cert,
        Elliptic,
        Hellman,
        Dtls,
        DtlsCookie
    };
    enum class EncodingType: uint8_t {
        PEM,
        DER
    };
    enum class CertAlgo{
        DSA,
        RSA,
        EC,
        DH,
        Opaque
    };
    enum class CertType{
        Private,
        Public
    };
    enum class CertOptions: uint8_t {
        NoFragments                 = 0x01,
        NoSessionTickets            = 0x02,
        NoCompression               = 0x03,
        NoServerNameIndication      = 0x04,
        NoLegacyRegen               = 0x05,
        NoSessionSharding           = 0x06,
        NoSessionPersistence        = 0x07,
        NoServerCipher              = 0x08
    };

    enum class CertVersion{
        Cert_V1_0                   = 0,
        Cert_V1_1                   = 1,
        Cert_V1_2                   = 2,
        Cert_V1_3                   = 3,

        Cert_DTLS_V1_0              = 10,
        Cert_DTLS_V1_2              = 11,

        Cert_V1_0_OR_LATER          = 20,
        Cert_V1_1_OR_LATER          = 21,
        Cert_V1_2_OR_LATER          = 22,
        Cert_V1_3_Or_Later          = 23,

        Cert_DTLS_V1_0_OR_LATER     = 33,
        Cert_DTLS_V1_2_OR_LATER     = 34,


        Cert_ALL                    = 40,
        Cert_SECURE_PROTOCOLS       = 41,
        Cert_UNKNOWN = 255,
    };



    // JobSslCert(std::filesystem::path *path, EncodingType format = EncodingType::PEM)
    // JobSslCert(const char *data = {}, EncodingType format = EncodingType::PEM)
    // JobSslCert(const JobSslCert &other)
    // JobSslCert(JobSslCert &&other)

    // Constructors
    JobSslCert();
    // Load Client/Server Identity from files
    JobSslCert(const std::filesystem::path &certPath,
               const std::filesystem::path &keyPath,
               EncodingType format = EncodingType::PEM);

    ~JobSslCert();

    // Move semantics
    JobSslCert(JobSslCert &&other) noexcept;
    JobSslCert &operator=(JobSslCert &&other) noexcept;

    // Delete copy (SSL_CTX has internal ref counting but let's keep it unique for now)
    JobSslCert(const JobSslCert &) = delete;
    JobSslCert &operator=(const JobSslCert &) = delete;

    // Configuration API
    bool setProtocol(CertVersion version);
    void setOption(CertOptions option, bool on);

    // Loading Identity
    bool loadCertificate(const std::string &path, EncodingType format = EncodingType::PEM);
    bool loadPrivateKey(const std::string &path, EncodingType format = EncodingType::PEM);
    bool loadCaBundle(const std::string &path); // For verifying the server

    // Validation
    bool isValid() const;
    std::string getError() const;

    // Accessor for the Socket to use
    SSL_CTX *nativeHandle() const { return m_ctx; }

private:
    void initOpenSSL();
    int mapVersion(CertVersion v) const;

    SSL_CTX *m_ctx{nullptr};
    std::string m_lastError;
};
}
// CHECKPOINT: v1
