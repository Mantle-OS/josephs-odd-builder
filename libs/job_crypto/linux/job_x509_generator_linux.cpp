#include "../job_x509_generator.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <openssl/asn1.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <job_logger.h>

    namespace job::crypto {

    namespace {

    using PKeyPtr = std::unique_ptr<EVP_PKEY, decltype(&::EVP_PKEY_free)>;
    using X509Ptr = std::unique_ptr<X509, decltype(&::X509_free)>;
    using Pkcs12Ptr = std::unique_ptr<PKCS12, decltype(&::PKCS12_free)>;
    using BignumPtr = std::unique_ptr<BIGNUM, decltype(&::BN_free)>;
    using Asn1IntegerPtr = std::unique_ptr<ASN1_INTEGER, decltype(&::ASN1_INTEGER_free)>;
    using GeneralNamesPtr = std::unique_ptr<GENERAL_NAMES, decltype(&::GENERAL_NAMES_free)>;

    struct FileCloser {
        void operator()(FILE *file) const noexcept
        {
            if (file)
                std::fclose(file);
        }
    };

    using FilePtr = std::unique_ptr<FILE, FileCloser>;

    [[nodiscard]] std::string lastOpenSslError()
    {
        const unsigned long error = ::ERR_get_error();

        if (error == 0)
            return "Unknown OpenSSL error";

        char buffer[256]{};
        ::ERR_error_string_n(error, buffer, sizeof(buffer));

        return buffer;
    }

    void fail(const std::string &message)
    {
        const std::string error = lastOpenSslError();
        JOB_LOG_ERROR("[JobX509Generator] {}: {}", message, error);
    }

    [[nodiscard]] FilePtr openOutputFile(const std::filesystem::path &path, mode_t permissions)
    {
        if (path.empty()) {
            JOB_LOG_ERROR("[JobX509Generator] Output path is empty");
            return {};
        }

        const int fd = ::open(
            path.c_str(),
            O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
            permissions
            );

        if (fd < 0) {
            JOB_LOG_ERROR("[JobX509Generator] Failed to open output file '{}': {}", path.string(), std::strerror(errno));
            return {};
        }

        if (::fchmod(fd, permissions) != 0) {
            JOB_LOG_ERROR("[JobX509Generator] Failed to set permissions on '{}': {}", path.string(), std::strerror(errno));
            ::close(fd);
            return {};
        }

        FILE *file = ::fdopen(fd, "wb");

        if (!file) {
            JOB_LOG_ERROR("[JobX509Generator] Failed to create file stream for '{}': {}", path.string(), std::strerror(errno));
            ::close(fd);
            return {};
        }

        return FilePtr(file);
    }

    [[nodiscard]] const EVP_MD *digestFor(JobSslOptions::Digest digest) noexcept
    {
        switch (digest) {
        case JobSslOptions::Digest::SHA256:
            return ::EVP_sha256();

        case JobSslOptions::Digest::SHA384:
            return ::EVP_sha384();

        case JobSslOptions::Digest::SHA512:
            return ::EVP_sha512();
        }

        return nullptr;
    }

    [[nodiscard]] const char *curveFor(JobSslOptions::EcCurve curve) noexcept
    {
        switch (curve) {
        case JobSslOptions::EcCurve::P256:
            return "P-256";

        case JobSslOptions::EcCurve::P384:
            return "P-384";

        case JobSslOptions::EcCurve::P521:
            return "P-521";
        }

        return nullptr;
    }

    [[nodiscard]] PKeyPtr generatePrivateKey(const JobSslOptions &opt)
    {
        ::ERR_clear_error();

        EVP_PKEY *key = nullptr;

        switch (opt.keyType()) {
        case JobSslOptions::KeyType::RSA:
            if (opt.rsaBits() < 2048) {
                JOB_LOG_ERROR("[JobX509Generator] RSA key size must be at least 2048 bits");
                return PKeyPtr(nullptr, ::EVP_PKEY_free);
            }

            key = ::EVP_PKEY_Q_keygen(
                nullptr,
                nullptr,
                "RSA",
                static_cast<size_t>(opt.rsaBits())
                );
            break;

        case JobSslOptions::KeyType::EC: {
            const char *curve = curveFor(opt.ecCurve());

            if (!curve) {
                JOB_LOG_ERROR("[JobX509Generator] Unsupported EC curve");
                return PKeyPtr(nullptr, ::EVP_PKEY_free);
            }

            key = ::EVP_PKEY_Q_keygen(
                nullptr,
                nullptr,
                "EC",
                curve
                );
            break;
        }
        }

        if (!key) {
            fail("Failed to generate private key");
            return PKeyPtr(nullptr, ::EVP_PKEY_free);
        }

        return PKeyPtr(key, ::EVP_PKEY_free);
    }

    [[nodiscard]] bool setRandomSerial(X509 *certificate)
    {
        std::array<unsigned char, 16> serialBytes{};

        if (::RAND_bytes(serialBytes.data(), static_cast<int>(serialBytes.size())) != 1) {
            fail("Failed to generate certificate serial number");
            return false;
        }

        serialBytes[0] &= 0x7f;

        const bool allZero = std::all_of(
            serialBytes.begin(),
            serialBytes.end(),
            [](unsigned char value) {
                return value == 0;
            }
            );

        if (allZero)
            serialBytes.back() = 1;

        BignumPtr serialNumber(
            ::BN_bin2bn(serialBytes.data(), static_cast<int>(serialBytes.size()), nullptr),
            ::BN_free
            );

        if (!serialNumber) {
            fail("Failed to create certificate serial number");
            return false;
        }

        Asn1IntegerPtr serial(
            ::BN_to_ASN1_INTEGER(serialNumber.get(), nullptr),
            ::ASN1_INTEGER_free
            );

        if (!serial) {
            fail("Failed to encode certificate serial number");
            return false;
        }

        if (::X509_set_serialNumber(certificate, serial.get()) != 1) {
            fail("Failed to set certificate serial number");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool addNameEntry(X509_NAME *name, const char *field, const std::string &value)
    {
        if (value.empty())
            return true;

        if (::X509_NAME_add_entry_by_txt(
                name,
                field,
                MBSTRING_UTF8,
                reinterpret_cast<const unsigned char *>(value.data()),
                static_cast<int>(value.size()),
                -1,
                0) != 1)
        {
            fail(std::string("Failed to set certificate subject field ") + field);
            return false;
        }

        return true;
    }

    [[nodiscard]] bool setSubject(X509 *certificate, const JobSslOptions &opt)
    {
        X509_NAME *subject = ::X509_get_subject_name(certificate);

        if (!subject) {
            fail("Failed to access certificate subject");
            return false;
        }

        if (!addNameEntry(subject, "C", opt.country()))
            return false;

        if (!addNameEntry(subject, "O", opt.organization()))
            return false;

        if (!addNameEntry(subject, "CN", opt.commonName()))
            return false;

        if (::X509_set_issuer_name(certificate, subject) != 1) {
            fail("Failed to set self-signed certificate issuer");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool addExtension(X509 *certificate, int nid, const char *value)
    {
        X509V3_CTX context{};
        ::X509V3_set_ctx(&context, certificate, certificate, nullptr, nullptr, 0);

        X509_EXTENSION *extension = ::X509V3_EXT_conf_nid(
            nullptr,
            &context,
            nid,
            const_cast<char *>(value)
            );

        if (!extension) {
            fail("Failed to create X.509 extension");
            return false;
        }

        const int result = ::X509_add_ext(certificate, extension, -1);
        ::X509_EXTENSION_free(extension);

        if (result != 1) {
            fail("Failed to add X.509 extension");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool addDnsName(GENERAL_NAMES *names, const std::string &dnsName)
    {
        if (dnsName.empty())
            return true;

        GENERAL_NAME *name = ::GENERAL_NAME_new();

        if (!name) {
            fail("Failed to allocate DNS alternative name");
            return false;
        }

        ASN1_IA5STRING *value = ::ASN1_IA5STRING_new();

        if (!value) {
            ::GENERAL_NAME_free(name);
            fail("Failed to allocate DNS alternative-name value");
            return false;
        }

        if (::ASN1_STRING_set(value, dnsName.data(), static_cast<int>(dnsName.size())) != 1) {
            ::ASN1_IA5STRING_free(value);
            ::GENERAL_NAME_free(name);
            fail("Failed to set DNS alternative name");
            return false;
        }

        ::GENERAL_NAME_set0_value(name, GEN_DNS, value);

        if (sk_GENERAL_NAME_push(names, name) == 0) {
            ::GENERAL_NAME_free(name);
            fail("Failed to append DNS alternative name");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool addIpAddress(GENERAL_NAMES *names, const std::string &ipAddress)
    {
        if (ipAddress.empty())
            return true;

        ASN1_OCTET_STRING *value = ::a2i_IPADDRESS(ipAddress.c_str());

        if (!value) {
            JOB_LOG_ERROR("[JobX509Generator] Invalid IP subject alternative name '{}'", ipAddress);
            return false;
        }

        GENERAL_NAME *name = ::GENERAL_NAME_new();

        if (!name) {
            ::ASN1_OCTET_STRING_free(value);
            fail("Failed to allocate IP alternative name");
            return false;
        }

        ::GENERAL_NAME_set0_value(name, GEN_IPADD, value);

        if (sk_GENERAL_NAME_push(names, name) == 0) {
            ::GENERAL_NAME_free(name);
            fail("Failed to append IP alternative name");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool addSubjectAlternativeNames(X509 *certificate, const JobSslOptions &opt)
    {
        GeneralNamesPtr names(
            sk_GENERAL_NAME_new_null(),
            ::GENERAL_NAMES_free
            );

        if (!names) {
            fail("Failed to allocate subject alternative names");
            return false;
        }

        for (const auto &dnsName : opt.dnsNames()) {
            if (!addDnsName(names.get(), dnsName))
                return false;
        }

        for (const auto &ipAddress : opt.ipAddresses()) {
            if (!addIpAddress(names.get(), ipAddress))
                return false;
        }

        if (sk_GENERAL_NAME_num(names.get()) == 0)
            return true;

        if (::X509_add1_ext_i2d(
                certificate,
                NID_subject_alt_name,
                names.get(),
                0,
                X509V3_ADD_DEFAULT) != 1)
        {
            fail("Failed to add subject alternative-name extension");
            return false;
        }

        return true;
    }

    [[nodiscard]] X509Ptr generateCertificate(const JobSslOptions &opt, EVP_PKEY *key)
    {
        ::ERR_clear_error();

        X509Ptr certificate(::X509_new(), ::X509_free);

        if (!certificate) {
            fail("Failed to allocate X.509 certificate");
            return certificate;
        }

        if (::X509_set_version(certificate.get(), 2) != 1) {
            fail("Failed to set X.509 certificate version");
            return X509Ptr(nullptr, ::X509_free);
        }

        if (!setRandomSerial(certificate.get()))
            return X509Ptr(nullptr, ::X509_free);

        if (opt.validDays() == 0) {
            JOB_LOG_ERROR("[JobX509Generator] Certificate validity must be at least one day");
            return X509Ptr(nullptr, ::X509_free);
        }

        if (!::X509_gmtime_adj(::X509_getm_notBefore(certificate.get()), 0)) {
            fail("Failed to set certificate start time");
            return X509Ptr(nullptr, ::X509_free);
        }

        constexpr long SECONDS_PER_DAY = 24L * 60L * 60L;

        if (opt.validDays() > static_cast<uint32_t>(LONG_MAX / SECONDS_PER_DAY)) {
            JOB_LOG_ERROR("[JobX509Generator] Certificate validity is too large");
            return X509Ptr(nullptr, ::X509_free);
        }

        const long validitySeconds = static_cast<long>(opt.validDays()) * SECONDS_PER_DAY;

        if (!::X509_gmtime_adj(::X509_getm_notAfter(certificate.get()), validitySeconds)) {
            fail("Failed to set certificate expiration time");
            return X509Ptr(nullptr, ::X509_free);
        }

        if (::X509_set_pubkey(certificate.get(), key) != 1) {
            fail("Failed to assign certificate public key");
            return X509Ptr(nullptr, ::X509_free);
        }

        if (!setSubject(certificate.get(), opt))
            return X509Ptr(nullptr, ::X509_free);

        if (!addExtension(certificate.get(), NID_basic_constraints, "critical,CA:FALSE"))
            return X509Ptr(nullptr, ::X509_free);

        if (opt.keyType() == JobSslOptions::KeyType::RSA) {
            if (!addExtension(certificate.get(), NID_key_usage, "critical,digitalSignature,keyEncipherment"))
                return X509Ptr(nullptr, ::X509_free);
        } else {
            if (!addExtension(certificate.get(), NID_key_usage, "critical,digitalSignature"))
                return X509Ptr(nullptr, ::X509_free);
        }

        if (!addExtension(certificate.get(), NID_ext_key_usage, "serverAuth,clientAuth"))
            return X509Ptr(nullptr, ::X509_free);

        if (!addExtension(certificate.get(), NID_subject_key_identifier, "hash"))
            return X509Ptr(nullptr, ::X509_free);

        if (!addExtension(certificate.get(), NID_authority_key_identifier, "keyid:always"))
            return X509Ptr(nullptr, ::X509_free);

        if (!addSubjectAlternativeNames(certificate.get(), opt))
            return X509Ptr(nullptr, ::X509_free);

        const EVP_MD *digest = digestFor(opt.digest());

        if (!digest) {
            JOB_LOG_ERROR("[JobX509Generator] Unsupported certificate digest");
            return X509Ptr(nullptr, ::X509_free);
        }

        if (::X509_sign(certificate.get(), key, digest) <= 0) {
            fail("Failed to sign X.509 certificate");
            return X509Ptr(nullptr, ::X509_free);
        }

        return certificate;
    }

    [[nodiscard]] bool writeCertificate(
        const std::filesystem::path &path,
        JobSslOptions::Encoding encoding,
        X509 *certificate)
    {
        FilePtr file = openOutputFile(path, 0644);

        if (!file)
            return false;

        ::ERR_clear_error();

        int result = 0;

        switch (encoding) {
        case JobSslOptions::Encoding::PEM:
            result = ::PEM_write_X509(file.get(), certificate);
            break;

        case JobSslOptions::Encoding::DER:
            result = ::i2d_X509_fp(file.get(), certificate);
            break;

        case JobSslOptions::Encoding::PKCS12:
            JOB_LOG_ERROR("[JobX509Generator] PKCS12 is not valid for separate certificate output");
            return false;
        }

        if (result != 1) {
            fail("Failed to write X.509 certificate");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool writePrivateKey(
        const std::filesystem::path &path,
        JobSslOptions::Encoding encoding,
        EVP_PKEY *key)
    {
        FilePtr file = openOutputFile(path, 0600);

        if (!file)
            return false;

        ::ERR_clear_error();

        int result = 0;

        switch (encoding) {
        case JobSslOptions::Encoding::PEM:
            result = ::PEM_write_PrivateKey(
                file.get(),
                key,
                nullptr,
                nullptr,
                0,
                nullptr,
                nullptr
                );
            break;

        case JobSslOptions::Encoding::DER:
            result = ::i2d_PrivateKey_fp(file.get(), key);
            break;

        case JobSslOptions::Encoding::PKCS12:
            JOB_LOG_ERROR("[JobX509Generator] PKCS12 is not valid for separate private-key output");
            return false;
        }

        if (result != 1) {
            fail("Failed to write private key");
            return false;
        }

        return true;
    }

    [[nodiscard]] bool securePassphrase(const JobSecureMem &passphrase, std::vector<char> &output)
    {
        output.clear();

        if (passphrase.empty()) {
            output.push_back('\0');
            return true;
        }

        if (std::memchr(passphrase.data(), '\0', passphrase.size())) {
            JOB_LOG_ERROR("[JobX509Generator] PKCS12 passphrase cannot contain embedded null bytes");
            return false;
        }

        try {
            output.resize(passphrase.size() + 1);
        } catch (...) {
            JOB_LOG_ERROR("[JobX509Generator] Failed to allocate temporary passphrase buffer");
            return false;
        }

        std::memcpy(output.data(), passphrase.data(), passphrase.size());
        output.back() = '\0';

        return true;
    }

    void clearPassphrase(std::vector<char> &passphrase) noexcept
    {
        if (!passphrase.empty())
            ::OPENSSL_cleanse(passphrase.data(), passphrase.size());

        passphrase.clear();
        passphrase.shrink_to_fit();
    }

    } // namespace

    bool JobX509Generator::generate(
        const JobSslOptions &opt,
        const std::filesystem::path &cert,
        const std::filesystem::path &priKey)
    {
        if (cert.empty() || priKey.empty()) {
            JOB_LOG_ERROR("[JobX509Generator] Certificate and private-key paths are required");
            return false;
        }

        if (cert == priKey) {
            JOB_LOG_ERROR("[JobX509Generator] Certificate and private-key paths must be different");
            return false;
        }

        if (opt.encoding() == JobSslOptions::Encoding::PKCS12) {
            JOB_LOG_ERROR("[JobX509Generator] Separate certificate and private-key generation does not support PKCS12 encoding");
            return false;
        }

        PKeyPtr key = generatePrivateKey(opt);

        if (!key)
            return false;

        X509Ptr certificate = generateCertificate(opt, key.get());

        if (!certificate)
            return false;

        if (!writePrivateKey(priKey, opt.encoding(), key.get())) {
            std::error_code error;
            std::filesystem::remove(priKey, error);
            return false;
        }

        if (!writeCertificate(cert, opt.encoding(), certificate.get())) {
            std::error_code error;
            std::filesystem::remove(priKey, error);
            std::filesystem::remove(cert, error);
            return false;
        }

        return true;
    }

    bool JobX509Generator::generate(
        const JobSslOptions &opt,
        const std::filesystem::path &idPath,
        const JobSecureMem &pass)
    {
        if (idPath.empty()) {
            JOB_LOG_ERROR("[JobX509Generator] Identity output path is required");
            return false;
        }

        if (opt.encoding() != JobSslOptions::Encoding::PKCS12) {
            JOB_LOG_ERROR("[JobX509Generator] Combined identity generation requires PKCS12 encoding");
            return false;
        }

        PKeyPtr key = generatePrivateKey(opt);

        if (!key)
            return false;

        X509Ptr certificate = generateCertificate(opt, key.get());

        if (!certificate)
            return false;

        std::vector<char> passphrase;

        if (!securePassphrase(pass, passphrase))
            return false;

        ::ERR_clear_error();

        Pkcs12Ptr identity(
            ::PKCS12_create(
                passphrase.data(),
                opt.commonName().c_str(),
                key.get(),
                certificate.get(),
                nullptr,
                0,
                0,
                0,
                0,
                0
                ),
            ::PKCS12_free
            );

        clearPassphrase(passphrase);

        if (!identity) {
            fail("Failed to create PKCS12 identity");
            return false;
        }

        FilePtr file = openOutputFile(idPath, 0600);

        if (!file)
            return false;

        if (::i2d_PKCS12_fp(file.get(), identity.get()) != 1) {
            std::error_code error;
            file.reset();
            std::filesystem::remove(idPath, error);
            fail("Failed to write PKCS12 identity");
            return false;
        }

        return true;
    }

} // namespace job::crypto

