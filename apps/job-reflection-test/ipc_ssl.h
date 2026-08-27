#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <unistd.h>

// job_crypto
#include <job_ssl_options.h>
#include <job_x509_generator.h>

// job_net
#include <job_ssl_context.h>
#include <ssl_client.h>
#include <ssl_server.h>

#include "packed.h"
#include "tmp_file.h"

class PackedSslCert
{
public:
    using Ptr = std::shared_ptr<PackedSslCert>;
    using WPtr = std::weak_ptr<PackedSslCert>;
    using UPtr = std::unique_ptr<PackedSslCert>;

    PackedSslCert() :
        m_certificate(makePath("certificate.pem")),
        m_privateKey(makePath("private_key.pem"))
    {
    }

    ~PackedSslCert() = default;

    PackedSslCert(const PackedSslCert &) = delete;
    PackedSslCert &operator=(const PackedSslCert &) = delete;
    PackedSslCert(PackedSslCert &&) = delete;
    PackedSslCert &operator=(PackedSslCert &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<PackedSslCert>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<PackedSslCert>();
    }

    [[nodiscard]] bool generate()
    {
        job::crypto::JobSslOptions options;

        options.setKeyType(job::crypto::JobSslOptions::KeyType::EC);
        options.setEcCurve(job::crypto::JobSslOptions::EcCurve::P256);
        options.setDigest (job::crypto::JobSslOptions::Digest::SHA256);
        options.setValidDays(1);
        options.setCommonName("localhost");
        options.setOrganization("JosephsOddBuilder Reflection Test");
        options.setCountry("US");
        options.setDnsNames({"localhost"});
        options.setIpAddresses({"127.0.0.1", "::1"});
        options.setEncoding(job::crypto::JobSslOptions::Encoding::PEM);

        return job::crypto::JobX509Generator::generate(options, m_certificate.path(), m_privateKey.path());
    }

    [[nodiscard]] job::net::JobSslContext::Ptr createServerContext() const
    {
        auto context = std::make_shared<job::net::JobSslContext>(job::net::JobSslContext::SslMode::Server);

        if (!context->isValid())
            return {};

        context->setVerifyMode(job::net::JobSslContext::VerifyMode::None);

        if (!context->loadCertificateFile(m_certificate.path(), job::net::JobSslContext::EncodingType::PEM))
            return {};

        if (!context->loadPrivateKeyFile(m_privateKey.path(), job::net::JobSslContext::EncodingType::PEM, {}))
            return {};

        return context;
    }

    [[nodiscard]] job::net::JobSslContext::Ptr createClientContext() const
    {
        auto context = std::make_shared<job::net::JobSslContext>(job::net::JobSslContext::SslMode::Client);

        if (!context->isValid())
            return {};

        context->setVerifyMode(job::net::JobSslContext::VerifyMode::None);
        return context;
    }

    [[nodiscard]] const std::string &certificatePath() const noexcept
    {
        return m_certificate.path();
    }

    [[nodiscard]] const std::string &privateKeyPath() const noexcept
    {
        return m_privateKey.path();
    }

private:
    [[nodiscard]] static std::string makePath(const std::string &name)
    {
        static std::atomic<std::uint64_t> counter{0};

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto index = counter.fetch_add(1, std::memory_order_relaxed);

        return (std::filesystem::temp_directory_path() /
                ("job_reflection_ssl_" + std::to_string(::getpid()) + "_" +
                 std::to_string(stamp) + "_" + std::to_string(index) + "_" + name)).string();
    }

    TmpFile m_certificate;
    TmpFile m_privateKey;
};

class PackedSslReader
{
public:
    using Ptr = std::shared_ptr<PackedSslReader>;
    using WPtr = std::weak_ptr<PackedSslReader>;
    using UPtr = std::unique_ptr<PackedSslReader>;
    using Callback = std::function<void(const Packed &)>;

    PackedSslReader() = default;
    ~PackedSslReader() = default;

    PackedSslReader(const PackedSslReader &) = delete;
    PackedSslReader &operator=(const PackedSslReader &) = delete;
    PackedSslReader(PackedSslReader &&) = delete;
    PackedSslReader &operator=(PackedSslReader &&) = delete;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<PackedSslReader>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<PackedSslReader>();
    }

    void setCallback(Callback callback)
    {
        m_callback = std::move(callback);
    }

    void read(const char *data, std::size_t len)
    {
        while (len > 0) {
            const std::size_t remaining = sizeof(Packed) - m_offset;
            const std::size_t count = std::min(remaining, len);

            std::memcpy(m_buffer.data() + m_offset, data, count);

            m_offset += count;
            data += count;
            len -= count;

            if (m_offset != sizeof(Packed))
                continue;

            Packed packed;
            std::memcpy(&packed, m_buffer.data(), sizeof(Packed));
            m_offset = 0;

            if (m_callback)
                m_callback(packed);
        }
    }

    void reset() noexcept
    {
        m_offset = 0;
    }

    [[nodiscard]] static const char *data(const Packed &packed) noexcept
    {
        return reinterpret_cast<const char *>(&packed);
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept
    {
        return sizeof(Packed);
    }

private:
    std::array<char, sizeof(Packed)> m_buffer{};
    std::size_t m_offset{0};
    Callback m_callback;
};