// # warning never tested nor even compiled .
#include "../job_x509_generator.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <bcrypt.h>
#include <ncrypt.h>
#include <wincrypt.h>
#include <ws2tcpip.h>

#include <job_logger.h>

namespace job::crypto {

namespace {

struct NCryptObjectCloser {
    void operator()(NCRYPT_HANDLE handle) const noexcept
    {
        if (handle)
            ::NCryptFreeObject(handle);
    }
};

struct CertContextCloser {
    void operator()(const CERT_CONTEXT *context) const noexcept
    {
        if (context)
            ::CertFreeCertificateContext(context);
    }
};

struct CertStoreCloser {
    void operator()(HCERTSTORE store) const noexcept
    {
        if (store)
            ::CertCloseStore(store, 0);
    }
};

struct LocalFreeCloser {
    void operator()(void *memory) const noexcept
    {
        if (memory)
            ::LocalFree(memory);
    }
};

struct HandleCloser {
    void operator()(HANDLE handle) const noexcept
    {
        if (handle && handle != INVALID_HANDLE_VALUE)
            ::CloseHandle(handle);
    }
};

using ProviderPtr = std::unique_ptr<std::remove_pointer_t<NCRYPT_PROV_HANDLE>, NCryptObjectCloser>;
using KeyPtr = std::unique_ptr<std::remove_pointer_t<NCRYPT_KEY_HANDLE>, NCryptObjectCloser>;
using CertContextPtr = std::unique_ptr<const CERT_CONTEXT, CertContextCloser>;
using CertStorePtr = std::unique_ptr<std::remove_pointer_t<HCERTSTORE>, CertStoreCloser>;
using LocalPtr = std::unique_ptr<void, LocalFreeCloser>;
using HandlePtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

struct EncodedExtension {
    std::string oid;
    std::vector<unsigned char> value;
    bool critical{false};
};

[[nodiscard]] std::string windowsErrorString(DWORD error)
{
    char *message = nullptr;

    const DWORD length = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        error,
        0,
        reinterpret_cast<char *>(&message),
        0,
        nullptr
        );

    LocalPtr memory(message);

    if (length == 0 || !message)
        return "Windows error " + std::to_string(error);

    std::string result(message, length);

    while (!result.empty()
           && (result.back() == '\r'
               || result.back() == '\n'
               || result.back() == ' '))
    {
        result.pop_back();
    }

    return result;
}

void fail(const std::string &message, DWORD error = ::GetLastError())
{
    JOB_LOG_ERROR(
        "[JobX509Generator] {}: {}",
        message,
        windowsErrorString(error)
        );
}

void failSecurityStatus(const std::string &message, SECURITY_STATUS status)
{
    JOB_LOG_ERROR(
        "[JobX509Generator] {}: NCrypt status 0x{:08x}",
        message,
        static_cast<unsigned long>(status)
        );
}

[[nodiscard]] std::wstring utf8ToWide(const std::string &text)
{
    if (text.empty())
        return {};

    const int count = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0
        );

    if (count <= 0)
        return {};

    std::wstring result(static_cast<size_t>(count), L'\0');

    if (::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            result.data(),
            count) != count)
    {
        return {};
    }

    return result;
}

[[nodiscard]] bool appendWideDnValue(
    std::wstring &dn,
    const wchar_t *field,
    const std::string &value)
{
    if (value.empty())
        return true;

    std::wstring wide = utf8ToWide(value);

    if (wide.empty()) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Failed to convert certificate subject field to UTF-16"
            );
        return false;
    }

    if (!dn.empty())
        dn += L", ";

    dn += field;
    dn += L"=\"";

    for (wchar_t character : wide) {
        if (character == L'"' || character == L'\\')
            dn += L'\\';

        dn += character;
    }

    dn += L'"';

    return true;
}

[[nodiscard]] bool encodeSubjectName(
    const JobSslOptions &opt,
    std::vector<unsigned char> &encoded)
{
    std::wstring dn;

    if (!appendWideDnValue(dn, L"CN", opt.commonName()))
        return false;

    if (!appendWideDnValue(dn, L"O", opt.organization()))
        return false;

    if (!appendWideDnValue(dn, L"C", opt.country()))
        return false;

    if (dn.empty()) {
        JOB_LOG_ERROR("[JobX509Generator] Certificate subject cannot be empty");
        return false;
    }

    DWORD size = 0;

    if (!::CertStrToNameW(
            X509_ASN_ENCODING,
            dn.c_str(),
            CERT_X500_NAME_STR,
            nullptr,
            nullptr,
            &size,
            nullptr))
    {
        fail("Failed to calculate encoded certificate subject size");
        return false;
    }

    encoded.resize(size);

    if (!::CertStrToNameW(
            X509_ASN_ENCODING,
            dn.c_str(),
            CERT_X500_NAME_STR,
            nullptr,
            encoded.data(),
            &size,
            nullptr))
    {
        fail("Failed to encode certificate subject");
        return false;
    }

    encoded.resize(size);
    return true;
}

[[nodiscard]] LPCWSTR algorithmFor(const JobSslOptions &opt) noexcept
{
    switch (opt.keyType()) {
    case JobSslOptions::KeyType::RSA:
        return NCRYPT_RSA_ALGORITHM;

    case JobSslOptions::KeyType::EC:
        switch (opt.ecCurve()) {
        case JobSslOptions::EcCurve::P256:
            return NCRYPT_ECDSA_P256_ALGORITHM;

        case JobSslOptions::EcCurve::P384:
            return NCRYPT_ECDSA_P384_ALGORITHM;

        case JobSslOptions::EcCurve::P521:
            return NCRYPT_ECDSA_P521_ALGORITHM;
        }
    }

    return nullptr;
}

[[nodiscard]] LPCSTR signatureOidFor(const JobSslOptions &opt) noexcept
{
    if (opt.keyType() == JobSslOptions::KeyType::RSA) {
        switch (opt.digest()) {
        case JobSslOptions::Digest::SHA256:
            return szOID_RSA_SHA256RSA;

        case JobSslOptions::Digest::SHA384:
            return szOID_RSA_SHA384RSA;

        case JobSslOptions::Digest::SHA512:
            return szOID_RSA_SHA512RSA;
        }
    }

    switch (opt.digest()) {
    case JobSslOptions::Digest::SHA256:
        return szOID_ECDSA_SHA256;

    case JobSslOptions::Digest::SHA384:
        return szOID_ECDSA_SHA384;

    case JobSslOptions::Digest::SHA512:
        return szOID_ECDSA_SHA512;
    }

    return nullptr;
}

[[nodiscard]] KeyPtr generatePrivateKey(
    const JobSslOptions &opt,
    ProviderPtr &provider)
{
    NCRYPT_PROV_HANDLE rawProvider = 0;

    SECURITY_STATUS status = ::NCryptOpenStorageProvider(
        &rawProvider,
        MS_KEY_STORAGE_PROVIDER,
        0
        );

    if (status != ERROR_SUCCESS) {
        failSecurityStatus("Failed to open Microsoft key storage provider", status);
        return KeyPtr(nullptr);
    }

    provider.reset(
        reinterpret_cast<std::remove_pointer_t<NCRYPT_PROV_HANDLE> *>(rawProvider)
        );

    const LPCWSTR algorithm = algorithmFor(opt);

    if (!algorithm) {
        JOB_LOG_ERROR("[JobX509Generator] Unsupported CNG key algorithm");
        return KeyPtr(nullptr);
    }

    if (opt.keyType() == JobSslOptions::KeyType::RSA
        && opt.rsaBits() < 2048)
    {
        JOB_LOG_ERROR(
            "[JobX509Generator] RSA key size must be at least 2048 bits"
            );
        return KeyPtr(nullptr);
    }

    NCRYPT_KEY_HANDLE rawKey = 0;

    status = ::NCryptCreatePersistedKey(
        rawProvider,
        &rawKey,
        algorithm,
        nullptr,
        0,
        0
        );

    if (status != ERROR_SUCCESS) {
        failSecurityStatus("Failed to create ephemeral CNG key", status);
        return KeyPtr(nullptr);
    }

    KeyPtr key(
        reinterpret_cast<std::remove_pointer_t<NCRYPT_KEY_HANDLE> *>(rawKey)
        );

    if (opt.keyType() == JobSslOptions::KeyType::RSA) {
        const DWORD bits = opt.rsaBits();

        status = ::NCryptSetProperty(
            rawKey,
            NCRYPT_LENGTH_PROPERTY,
            reinterpret_cast<PBYTE>(const_cast<DWORD *>(&bits)),
            sizeof(bits),
            0
            );

        if (status != ERROR_SUCCESS) {
            failSecurityStatus("Failed to set RSA key length", status);
            return KeyPtr(nullptr);
        }
    }

    const DWORD exportPolicy =
        NCRYPT_ALLOW_EXPORT_FLAG
        | NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;

    status = ::NCryptSetProperty(
        rawKey,
        NCRYPT_EXPORT_POLICY_PROPERTY,
        reinterpret_cast<PBYTE>(
            const_cast<DWORD *>(&exportPolicy)
            ),
        sizeof(exportPolicy),
        0
        );

    if (status != ERROR_SUCCESS) {
        failSecurityStatus("Failed to set key export policy", status);
        return KeyPtr(nullptr);
    }

    status = ::NCryptFinalizeKey(rawKey, 0);

    if (status != ERROR_SUCCESS) {
        failSecurityStatus("Failed to finalize CNG key", status);
        return KeyPtr(nullptr);
    }

    return key;
}

[[nodiscard]] bool encodeObject(
    LPCSTR structureType,
    const void *object,
    std::vector<unsigned char> &encoded)
{
    DWORD size = 0;

    if (!::CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            structureType,
            object,
            0,
            nullptr,
            nullptr,
            &size))
    {
        fail("Failed to calculate encoded X.509 extension size");
        return false;
    }

    encoded.resize(size);

    if (!::CryptEncodeObjectEx(
            X509_ASN_ENCODING,
            structureType,
            object,
            0,
            nullptr,
            encoded.data(),
            &size))
    {
        fail("Failed to encode X.509 extension");
        return false;
    }

    encoded.resize(size);
    return true;
}

[[nodiscard]] bool addBasicConstraints(
    std::vector<EncodedExtension> &extensions)
{
    CERT_BASIC_CONSTRAINTS2_INFO constraints{};
    constraints.fCA = FALSE;
    constraints.fPathLenConstraint = FALSE;

    EncodedExtension extension;
    extension.oid = szOID_BASIC_CONSTRAINTS2;
    extension.critical = true;

    if (!encodeObject(
            X509_BASIC_CONSTRAINTS2,
            &constraints,
            extension.value))
    {
        return false;
    }

    extensions.emplace_back(std::move(extension));
    return true;
}

[[nodiscard]] bool addKeyUsage(
    const JobSslOptions &opt,
    std::vector<EncodedExtension> &extensions)
{
    BYTE usage = CERT_DIGITAL_SIGNATURE_KEY_USAGE;

    if (opt.keyType() == JobSslOptions::KeyType::RSA)
        usage |= CERT_KEY_ENCIPHERMENT_KEY_USAGE;

    CRYPT_BIT_BLOB usageBlob{};
    usageBlob.cbData = sizeof(usage);
    usageBlob.pbData = &usage;
    usageBlob.cUnusedBits = 0;

    EncodedExtension extension;
    extension.oid = szOID_KEY_USAGE;
    extension.critical = true;

    if (!encodeObject(X509_KEY_USAGE, &usageBlob, extension.value))
        return false;

    extensions.emplace_back(std::move(extension));
    return true;
}

[[nodiscard]] bool addExtendedKeyUsage(
    std::vector<EncodedExtension> &extensions)
{
    std::array<LPSTR, 2> usages{
        const_cast<LPSTR>(szOID_PKIX_KP_SERVER_AUTH),
        const_cast<LPSTR>(szOID_PKIX_KP_CLIENT_AUTH)
    };

    CERT_ENHKEY_USAGE enhancedUsage{};
    enhancedUsage.cUsageIdentifier =
        static_cast<DWORD>(usages.size());
    enhancedUsage.rgpszUsageIdentifier = usages.data();

    EncodedExtension extension;
    extension.oid = szOID_ENHANCED_KEY_USAGE;
    extension.critical = false;

    if (!encodeObject(
            X509_ENHANCED_KEY_USAGE,
            &enhancedUsage,
            extension.value))
    {
        return false;
    }

    extensions.emplace_back(std::move(extension));
    return true;
}

[[nodiscard]] bool parseIpAddress(
    const std::string &address,
    std::vector<unsigned char> &encoded)
{
    std::wstring wide = utf8ToWide(address);

    if (wide.empty())
        return false;

    IN_ADDR ipv4{};

    if (::InetPtonW(AF_INET, wide.c_str(), &ipv4) == 1) {
        const auto *bytes =
            reinterpret_cast<const unsigned char *>(&ipv4);

        encoded.assign(bytes, bytes + sizeof(ipv4));
        return true;
    }

    IN6_ADDR ipv6{};

    if (::InetPtonW(AF_INET6, wide.c_str(), &ipv6) == 1) {
        const auto *bytes =
            reinterpret_cast<const unsigned char *>(&ipv6);

        encoded.assign(bytes, bytes + sizeof(ipv6));
        return true;
    }

    return false;
}

[[nodiscard]] bool addSubjectAlternativeNames(
    const JobSslOptions &opt,
    std::vector<EncodedExtension> &extensions)
{
    const size_t total =
        opt.dnsNames().size()
        + opt.ipAddresses().size();

    if (total == 0)
        return true;

    if (total > std::numeric_limits<DWORD>::max()) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Too many subject alternative names"
            );
        return false;
    }

    std::vector<CERT_ALT_NAME_ENTRY> entries;
    std::vector<std::wstring> dnsNames;
    std::vector<std::vector<unsigned char>> ipAddresses;

    entries.reserve(total);
    dnsNames.reserve(opt.dnsNames().size());
    ipAddresses.reserve(opt.ipAddresses().size());

    for (const std::string &dnsName : opt.dnsNames()) {
        if (dnsName.empty())
            continue;

        std::wstring wide = utf8ToWide(dnsName);

        if (wide.empty()) {
            JOB_LOG_ERROR(
                "[JobX509Generator] Invalid UTF-8 DNS subject alternative name"
                );
            return false;
        }

        dnsNames.emplace_back(std::move(wide));

        CERT_ALT_NAME_ENTRY entry{};
        entry.dwAltNameChoice = CERT_ALT_NAME_DNS_NAME;
        entry.pwszDNSName = dnsNames.back().data();

        entries.emplace_back(entry);
    }

    for (const std::string &ipAddress : opt.ipAddresses()) {
        if (ipAddress.empty())
            continue;

        std::vector<unsigned char> bytes;

        if (!parseIpAddress(ipAddress, bytes)) {
            JOB_LOG_ERROR(
                "[JobX509Generator] Invalid IP subject alternative name '{}'",
                ipAddress
                );
            return false;
        }

        ipAddresses.emplace_back(std::move(bytes));

        CERT_ALT_NAME_ENTRY entry{};
        entry.dwAltNameChoice = CERT_ALT_NAME_IP_ADDRESS;
        entry.IPAddress.cbData =
            static_cast<DWORD>(ipAddresses.back().size());
        entry.IPAddress.pbData = ipAddresses.back().data();

        entries.emplace_back(entry);
    }

    if (entries.empty())
        return true;

    CERT_ALT_NAME_INFO altNames{};
    altNames.cAltEntry = static_cast<DWORD>(entries.size());
    altNames.rgAltEntry = entries.data();

    EncodedExtension extension;
    extension.oid = szOID_SUBJECT_ALT_NAME2;
    extension.critical = false;

    if (!encodeObject(
            X509_ALTERNATE_NAME,
            &altNames,
            extension.value))
    {
        return false;
    }

    extensions.emplace_back(std::move(extension));
    return true;
}

[[nodiscard]] bool buildExtensions(
    const JobSslOptions &opt,
    std::vector<EncodedExtension> &encodedExtensions,
    std::vector<CERT_EXTENSION> &nativeExtensions)
{
    if (!addBasicConstraints(encodedExtensions))
        return false;

    if (!addKeyUsage(opt, encodedExtensions))
        return false;

    if (!addExtendedKeyUsage(encodedExtensions))
        return false;

    if (!addSubjectAlternativeNames(opt, encodedExtensions))
        return false;

    nativeExtensions.reserve(encodedExtensions.size());

    for (EncodedExtension &source : encodedExtensions) {
        CERT_EXTENSION extension{};
        extension.pszObjId = source.oid.data();
        extension.fCritical = source.critical ? TRUE : FALSE;
        extension.Value.cbData =
            static_cast<DWORD>(source.value.size());
        extension.Value.pbData = source.value.data();

        nativeExtensions.emplace_back(extension);
    }

    return true;
}

[[nodiscard]] bool calculateValidity(
    uint32_t validDays,
    SYSTEMTIME &start,
    SYSTEMTIME &end)
{
    if (validDays == 0) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Certificate validity must be at least one day"
            );
        return false;
    }

    FILETIME now{};
    ::GetSystemTimeAsFileTime(&now);

    ULARGE_INTEGER ticks{};
    ticks.LowPart = now.dwLowDateTime;
    ticks.HighPart = now.dwHighDateTime;

    constexpr uint64_t TICKS_PER_SECOND = 10000000ULL;
    constexpr uint64_t SECONDS_PER_DAY = 24ULL * 60ULL * 60ULL;
    constexpr uint64_t TICKS_PER_DAY =
        TICKS_PER_SECOND * SECONDS_PER_DAY;

    const uint64_t days = validDays;

    if (days
        > (std::numeric_limits<uint64_t>::max() - ticks.QuadPart)
              / TICKS_PER_DAY)
    {
        JOB_LOG_ERROR(
            "[JobX509Generator] Certificate validity is too large"
            );
        return false;
    }

    FILETIME expiration{};
    ULARGE_INTEGER expirationTicks{};
    expirationTicks.QuadPart =
        ticks.QuadPart + days * TICKS_PER_DAY;

    expiration.dwLowDateTime = expirationTicks.LowPart;
    expiration.dwHighDateTime = expirationTicks.HighPart;

    if (!::FileTimeToSystemTime(&now, &start)) {
        fail("Failed to convert certificate start time");
        return false;
    }

    if (!::FileTimeToSystemTime(&expiration, &end)) {
        fail("Failed to convert certificate expiration time");
        return false;
    }

    return true;
}

[[nodiscard]] CertContextPtr generateCertificate(
    const JobSslOptions &opt,
    NCRYPT_KEY_HANDLE key)
{
    std::vector<unsigned char> encodedSubject;

    if (!encodeSubjectName(opt, encodedSubject))
        return CertContextPtr(nullptr);

    CERT_NAME_BLOB subject{};
    subject.cbData = static_cast<DWORD>(encodedSubject.size());
    subject.pbData = encodedSubject.data();

    std::vector<EncodedExtension> encodedExtensions;
    std::vector<CERT_EXTENSION> nativeExtensions;

    if (!buildExtensions(
            opt,
            encodedExtensions,
            nativeExtensions))
    {
        return CertContextPtr(nullptr);
    }

    CERT_EXTENSIONS extensions{};
    extensions.cExtension =
        static_cast<DWORD>(nativeExtensions.size());
    extensions.rgExtension = nativeExtensions.data();

    CRYPT_ALGORITHM_IDENTIFIER signatureAlgorithm{};
    signatureAlgorithm.pszObjId =
        const_cast<LPSTR>(signatureOidFor(opt));

    if (!signatureAlgorithm.pszObjId) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Unsupported signature algorithm"
            );
        return CertContextPtr(nullptr);
    }

    SYSTEMTIME start{};
    SYSTEMTIME end{};

    if (!calculateValidity(opt.validDays(), start, end))
        return CertContextPtr(nullptr);

    PCCERT_CONTEXT certificate =
        ::CertCreateSelfSignCertificate(
            static_cast<HCRYPTPROV_OR_NCRYPT_KEY_HANDLE>(key),
            &subject,
            CERT_CREATE_SELFSIGN_NO_KEY_INFO,
            nullptr,
            &signatureAlgorithm,
            &start,
            &end,
            nativeExtensions.empty() ? nullptr : &extensions
            );

    if (!certificate) {
        fail("Failed to create self-signed certificate");
        return CertContextPtr(nullptr);
    }

    return CertContextPtr(certificate);
}

[[nodiscard]] bool exportPrivateKey(
    NCRYPT_KEY_HANDLE key,
    std::vector<unsigned char> &encoded)
{
    DWORD size = 0;

    SECURITY_STATUS status = ::NCryptExportKey(
        key,
        0,
        NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
        nullptr,
        nullptr,
        0,
        &size,
        0
        );

    if (status != ERROR_SUCCESS) {
        failSecurityStatus(
            "Failed to calculate PKCS8 private-key size",
            status
            );
        return false;
    }

    encoded.resize(size);

    status = ::NCryptExportKey(
        key,
        0,
        NCRYPT_PKCS8_PRIVATE_KEY_BLOB,
        nullptr,
        encoded.data(),
        size,
        &size,
        0
        );

    if (status != ERROR_SUCCESS) {
        failSecurityStatus("Failed to export PKCS8 private key", status);
        ::SecureZeroMemory(encoded.data(), encoded.size());
        encoded.clear();
        return false;
    }

    encoded.resize(size);
    return true;
}

[[nodiscard]] bool encodePem(
    const char *label,
    const unsigned char *data,
    size_t size,
    std::vector<unsigned char> &output)
{
    if (!label || !data || size == 0)
        return false;

    if (size > std::numeric_limits<DWORD>::max()) {
        JOB_LOG_ERROR("[JobX509Generator] PEM input is too large");
        return false;
    }

    DWORD base64Size = 0;

    if (!::CryptBinaryToStringA(
            data,
            static_cast<DWORD>(size),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            nullptr,
            &base64Size))
    {
        fail("Failed to calculate PEM Base64 size");
        return false;
    }

    std::string base64(base64Size, '\0');

    if (!::CryptBinaryToStringA(
            data,
            static_cast<DWORD>(size),
            CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
            base64.data(),
            &base64Size))
    {
        fail("Failed to encode PEM Base64 data");
        return false;
    }

    if (!base64.empty() && base64.back() == '\0')
        base64.pop_back();

    std::string pem;
    pem += "-----BEGIN ";
    pem += label;
    pem += "-----\r\n";

    constexpr size_t LINE_LENGTH = 64;

    for (size_t offset = 0; offset < base64.size(); offset += LINE_LENGTH) {
        pem.append(
            base64,
            offset,
            std::min(LINE_LENGTH, base64.size() - offset)
            );
        pem += "\r\n";
    }

    pem += "-----END ";
    pem += label;
    pem += "-----\r\n";

    output.assign(pem.begin(), pem.end());
    return true;
}

[[nodiscard]] bool writeFile(
    const std::filesystem::path &path,
    const unsigned char *data,
    size_t size)
{
    if (path.empty()) {
        JOB_LOG_ERROR("[JobX509Generator] Output path is empty");
        return false;
    }

    if (!data || size == 0) {
        JOB_LOG_ERROR("[JobX509Generator] Output data is empty");
        return false;
    }

    HANDLE rawFile = ::CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
        );

    if (rawFile == INVALID_HANDLE_VALUE) {
        fail(
            "Failed to open output file '" + path.string() + "'"
            );
        return false;
    }

    HandlePtr file(
        reinterpret_cast<std::remove_pointer_t<HANDLE> *>(rawFile)
        );

    size_t writtenTotal = 0;

    while (writtenTotal < size) {
        const size_t remaining = size - writtenTotal;
        const DWORD chunk = static_cast<DWORD>(
            std::min<size_t>(
                remaining,
                std::numeric_limits<DWORD>::max()
                )
            );

        DWORD written = 0;

        if (!::WriteFile(
                rawFile,
                data + writtenTotal,
                chunk,
                &written,
                nullptr))
        {
            fail(
                "Failed to write output file '" + path.string() + "'"
                );
            file.reset();
            ::DeleteFileW(path.c_str());
            return false;
        }

        if (written == 0) {
            JOB_LOG_ERROR(
                "[JobX509Generator] Zero-byte write for '{}'",
                path.string()
                );
            file.reset();
            ::DeleteFileW(path.c_str());
            return false;
        }

        writtenTotal += written;
    }

    if (!::FlushFileBuffers(rawFile)) {
        fail(
            "Failed to flush output file '" + path.string() + "'"
            );
        file.reset();
        ::DeleteFileW(path.c_str());
        return false;
    }

    return true;
}

[[nodiscard]] bool writeCertificate(
    const std::filesystem::path &path,
    JobSslOptions::Encoding encoding,
    const CERT_CONTEXT *certificate)
{
    if (encoding == JobSslOptions::Encoding::PKCS12) {
        JOB_LOG_ERROR(
            "[JobX509Generator] PKCS12 is not valid for separate certificate output"
            );
        return false;
    }

    if (encoding == JobSslOptions::Encoding::DER) {
        return writeFile(
            path,
            certificate->pbCertEncoded,
            certificate->cbCertEncoded
            );
    }

    std::vector<unsigned char> pem;

    if (!encodePem(
            "CERTIFICATE",
            certificate->pbCertEncoded,
            certificate->cbCertEncoded,
            pem))
    {
        return false;
    }

    return writeFile(path, pem.data(), pem.size());
}

[[nodiscard]] bool writePrivateKey(
    const std::filesystem::path &path,
    JobSslOptions::Encoding encoding,
    NCRYPT_KEY_HANDLE key)
{
    if (encoding == JobSslOptions::Encoding::PKCS12) {
        JOB_LOG_ERROR(
            "[JobX509Generator] PKCS12 is not valid for separate private-key output"
            );
        return false;
    }

    std::vector<unsigned char> privateKey;

    if (!exportPrivateKey(key, privateKey))
        return false;

    bool result = false;

    if (encoding == JobSslOptions::Encoding::DER) {
        result = writeFile(
            path,
            privateKey.data(),
            privateKey.size()
            );
    } else {
        std::vector<unsigned char> pem;

        if (encodePem(
                "PRIVATE KEY",
                privateKey.data(),
                privateKey.size(),
                pem))
        {
            result = writeFile(path, pem.data(), pem.size());
            ::SecureZeroMemory(pem.data(), pem.size());
        }
    }

    ::SecureZeroMemory(privateKey.data(), privateKey.size());
    return result;
}

[[nodiscard]] bool securePassphrase(
    const JobSecureMem &passphrase,
    std::vector<wchar_t> &output)
{
    output.clear();

    if (passphrase.empty()) {
        output.push_back(L'\0');
        return true;
    }

    if (std::memchr(
            passphrase.data(),
            '\0',
            passphrase.size()))
    {
        JOB_LOG_ERROR(
            "[JobX509Generator] PKCS12 passphrase cannot contain embedded null bytes"
            );
        return false;
    }

    if (passphrase.size()
        > static_cast<size_t>(std::numeric_limits<int>::max()))
    {
        JOB_LOG_ERROR(
            "[JobX509Generator] PKCS12 passphrase is too large"
            );
        return false;
    }

    const int count = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        reinterpret_cast<const char *>(passphrase.data()),
        static_cast<int>(passphrase.size()),
        nullptr,
        0
        );

    if (count <= 0) {
        fail("Failed to convert PKCS12 passphrase from UTF-8");
        return false;
    }

    try {
        output.resize(static_cast<size_t>(count) + 1);
    } catch (...) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Failed to allocate temporary passphrase buffer"
            );
        return false;
    }

    if (::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            reinterpret_cast<const char *>(passphrase.data()),
            static_cast<int>(passphrase.size()),
            output.data(),
            count) != count)
    {
        fail("Failed to convert PKCS12 passphrase from UTF-8");
        ::SecureZeroMemory(
            output.data(),
            output.size() * sizeof(wchar_t)
            );
        output.clear();
        return false;
    }

    output.back() = L'\0';
    return true;
}

void clearPassphrase(std::vector<wchar_t> &passphrase) noexcept
{
    if (!passphrase.empty()) {
        ::SecureZeroMemory(
            passphrase.data(),
            passphrase.size() * sizeof(wchar_t)
            );
    }

    passphrase.clear();
    passphrase.shrink_to_fit();
}

[[nodiscard]] bool exportIdentity(
    const std::filesystem::path &path,
    CertContextPtr &certificate,
    KeyPtr &key,
    const JobSecureMem &passphrase)
{
    NCRYPT_KEY_HANDLE rawKey =
        reinterpret_cast<NCRYPT_KEY_HANDLE>(key.get());

    if (!::CertSetCertificateContextProperty(
            certificate.get(),
            CERT_NCRYPT_KEY_HANDLE_PROP_ID,
            CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
            &rawKey))
    {
        fail("Failed to associate CNG private key with certificate");
        return false;
    }

    key.release();

    HCERTSTORE rawStore = ::CertOpenStore(
        CERT_STORE_PROV_MEMORY,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        0,
        CERT_STORE_CREATE_NEW_FLAG,
        nullptr
        );

    if (!rawStore) {
        fail("Failed to create temporary certificate store");
        return false;
    }

    CertStorePtr store(
        reinterpret_cast<std::remove_pointer_t<HCERTSTORE> *>(rawStore)
        );

    if (!::CertAddCertificateContextToStore(
            rawStore,
            certificate.get(),
            CERT_STORE_ADD_ALWAYS,
            nullptr))
    {
        fail("Failed to add certificate to temporary store");
        return false;
    }

    std::vector<wchar_t> widePassphrase;

    if (!securePassphrase(passphrase, widePassphrase))
        return false;

    CRYPT_DATA_BLOB pfx{};

    constexpr DWORD EXPORT_FLAGS =
        EXPORT_PRIVATE_KEYS
        | REPORT_NO_PRIVATE_KEY
        | REPORT_NOT_ABLE_TO_EXPORT_PRIVATE_KEY;

    if (!::PFXExportCertStoreEx(
            rawStore,
            &pfx,
            widePassphrase.data(),
            nullptr,
            EXPORT_FLAGS))
    {
        clearPassphrase(widePassphrase);
        fail("Failed to calculate PKCS12 identity size");
        return false;
    }

    std::vector<unsigned char> encoded(pfx.cbData);
    pfx.pbData = encoded.data();

    if (!::PFXExportCertStoreEx(
            rawStore,
            &pfx,
            widePassphrase.data(),
            nullptr,
            EXPORT_FLAGS))
    {
        clearPassphrase(widePassphrase);
        ::SecureZeroMemory(encoded.data(), encoded.size());
        fail("Failed to export PKCS12 identity");
        return false;
    }

    clearPassphrase(widePassphrase);

    const bool result = writeFile(
        path,
        encoded.data(),
        pfx.cbData
        );

    ::SecureZeroMemory(encoded.data(), encoded.size());
    return result;
}

} // namespace

bool JobX509Generator::generate(
    const JobSslOptions &opt,
    const std::filesystem::path &cert,
    const std::filesystem::path &priKey)
{
    if (cert.empty() || priKey.empty()) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Certificate and private-key paths are required"
            );
        return false;
    }

    if (cert == priKey) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Certificate and private-key paths must be different"
            );
        return false;
    }

    if (opt.encoding() == JobSslOptions::Encoding::PKCS12) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Separate certificate and private-key generation does not support PKCS12 encoding"
            );
        return false;
    }

    ProviderPtr provider(nullptr);
    KeyPtr key = generatePrivateKey(opt, provider);

    if (!key)
        return false;

    const NCRYPT_KEY_HANDLE rawKey =
        reinterpret_cast<NCRYPT_KEY_HANDLE>(key.get());

    CertContextPtr certificate =
        generateCertificate(opt, rawKey);

    if (!certificate)
        return false;

    if (!writePrivateKey(
            priKey,
            opt.encoding(),
            rawKey))
    {
        ::DeleteFileW(priKey.c_str());
        return false;
    }

    if (!writeCertificate(
            cert,
            opt.encoding(),
            certificate.get()))
    {
        ::DeleteFileW(priKey.c_str());
        ::DeleteFileW(cert.c_str());
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
        JOB_LOG_ERROR(
            "[JobX509Generator] Identity output path is required"
            );
        return false;
    }

    if (opt.encoding() != JobSslOptions::Encoding::PKCS12) {
        JOB_LOG_ERROR(
            "[JobX509Generator] Combined identity generation requires PKCS12 encoding"
            );
        return false;
    }

    ProviderPtr provider(nullptr);
    KeyPtr key = generatePrivateKey(opt, provider);

    if (!key)
        return false;

    const NCRYPT_KEY_HANDLE rawKey =
        reinterpret_cast<NCRYPT_KEY_HANDLE>(key.get());

    CertContextPtr certificate =
        generateCertificate(opt, rawKey);

    if (!certificate)
        return false;

    if (!exportIdentity(idPath, certificate, key, pass)) {
        ::DeleteFileW(idPath.c_str());
        return false;
    }

    return true;
}

} // namespace job::crypto