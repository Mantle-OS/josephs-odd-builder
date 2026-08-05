#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#include "jobcrypto_export.h"

namespace job::crypto {

class JOBCRYPTO_EXPORT JobSslOptions {
public:
    enum class KeyType : uint8_t {
        RSA = 0,
        EC
    };

    enum class EcCurve : uint8_t {
        P256 = 0,
        P384,
        P521
    };

    enum class Digest : uint8_t {
        SHA256 = 0,
        SHA384,
        SHA512
    };

    enum class Encoding : uint8_t {
        PEM = 0,
        DER,
        PKCS12
    };

    JobSslOptions() = default;

    [[nodiscard]] KeyType keyType() const noexcept
    {
        return m_keyType;
    }

    void setKeyType(KeyType type) noexcept
    {
        m_keyType = type;
    }

    [[nodiscard]] EcCurve ecCurve() const noexcept
    {
        return m_ecCurve;
    }

    void setEcCurve(EcCurve curve) noexcept
    {
        m_ecCurve = curve;
    }

    [[nodiscard]] Digest digest() const noexcept
    {
        return m_digest;
    }

    void setDigest(Digest digest) noexcept
    {
        m_digest = digest;
    }

    [[nodiscard]] Encoding encoding() const noexcept
    {
        return m_encoding;
    }

    void setEncoding(Encoding encoding) noexcept
    {
        m_encoding = encoding;
    }

    [[nodiscard]] uint32_t rsaBits() const noexcept
    {
        return m_rsaBits;
    }

    void setRsaBits(uint32_t bits) noexcept
    {
        m_rsaBits = bits;
    }

    [[nodiscard]] uint32_t validDays() const noexcept
    {
        return m_validDays;
    }

    void setValidDays(uint32_t days) noexcept
    {
        m_validDays = days;
    }

    [[nodiscard]] const std::string &commonName() const noexcept
    {
        return m_commonName;
    }

    void setCommonName(std::string name)
    {
        m_commonName = std::move(name);
    }

    [[nodiscard]] const std::string &organization() const noexcept
    {
        return m_organization;
    }

    void setOrganization(std::string organization)
    {
        m_organization = std::move(organization);
    }

    [[nodiscard]] const std::string &country() const noexcept
    {
        return m_country;
    }

    void setCountry(std::string country)
    {
        m_country = std::move(country);
    }

    [[nodiscard]] const std::vector<std::string> &dnsNames() const noexcept
    {
        return m_dnsNames;
    }

    void setDnsNames(std::vector<std::string> names)
    {
        m_dnsNames = std::move(names);
    }

    void addDnsName(std::string name)
    {
        m_dnsNames.emplace_back(std::move(name));
    }

    void clearDnsNames() noexcept
    {
        m_dnsNames.clear();
    }

    [[nodiscard]] const std::vector<std::string> &ipAddresses() const noexcept
    {
        return m_ipAddresses;
    }

    void setIpAddresses(std::vector<std::string> addresses)
    {
        m_ipAddresses = std::move(addresses);
    }

    void addIpAddress(std::string address)
    {
        m_ipAddresses.emplace_back(std::move(address));
    }

    void clearIpAddresses() noexcept
    {
        m_ipAddresses.clear();
    }

private:
    KeyType                     m_keyType{KeyType::EC};
    EcCurve                     m_ecCurve{EcCurve::P256};
    Digest                      m_digest{Digest::SHA256};
    Encoding                    m_encoding{Encoding::PEM};

    uint32_t                    m_rsaBits{2048};
    uint32_t                    m_validDays{1};

    std::string                 m_commonName{"localhost"};
    std::string                 m_organization{"JOB"};
    std::string                 m_country{"US"};

    std::vector<std::string>    m_dnsNames{"localhost"};
    std::vector<std::string>    m_ipAddresses{"127.0.0.1", "::1"};
};

} // namespace job::crypto
