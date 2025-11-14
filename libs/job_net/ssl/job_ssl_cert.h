#pragma once

#include <filesystem>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace job::net::ssl {

class JobSslCert {
public:


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






private:





};
}
// CHECKPOINT: v1
