#pragma once

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <string_view>

#include <job_secure_mem.h>

#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobUrl
{
public:
    using Ptr = std::shared_ptr<JobUrl>;

    enum class Scheme : uint8_t
    {
        Unknown = 0,
        File,
        Http,
        Https,
        Tcp,
        Udp,
        Unix,
        Git,
        GitSm,
        Svn,
        Ssh,
        Serial
    };

    enum class FormatOption : uint8_t
    {
        None = 0,
        EncodeSpaces = 1 << 0,
        StripTrailingSlash = 1 << 1,
        IncludePassword = 1 << 2
    };

    enum class PasswdMode : uint8_t
    {
        Strict = 0,
        Lenient
    };

    JobUrl() = default;
    explicit JobUrl(const std::string &urlString);
    JobUrl(const JobUrl &other);
    JobUrl(JobUrl &&other) noexcept;
    JobUrl &operator=(const JobUrl &other);
    JobUrl &operator=(JobUrl &&other) noexcept;
    ~JobUrl();

    bool parse(const std::string &urlString);

    [[nodiscard]] std::string toString(uint8_t options = static_cast<uint8_t>(FormatOption::None)) const;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] size_t size() const noexcept;

    void clear() noexcept;

    [[nodiscard]] Scheme scheme() const noexcept;
    void setScheme(Scheme scheme);
    void setScheme(const std::string &schemeStr);

    [[nodiscard]] static std::string schemeToString(Scheme scheme) noexcept;
    [[nodiscard]] static Scheme schemeFromString(const std::string &str) noexcept;

    [[nodiscard]] const std::string &host() const noexcept;
    void setHost(const std::string &h);

    [[nodiscard]] uint16_t port() const noexcept;
    void setPort(uint16_t p);

    [[nodiscard]] const std::string &path() const noexcept;
    void setPath(const std::string &p);

    [[nodiscard]] const std::string &query() const noexcept;
    void setQuery(const std::string &q);

    [[nodiscard]] const std::string &fragment() const noexcept;
    void setFragment(const std::string &f);

    [[nodiscard]] const std::string &username() const noexcept;
    void setUsername(const std::string &u);

    void setPassword(const std::string &p)
    {
        // p is now in memory it needs to be whiped like really free'd
        setPassword(p.data(), p.size());
    }

    void setPassword(const char *data, size_t len)
    {
        if (len == 0) {
            m_password.reset();
            return;
        }

        m_password = std::make_unique<crypto::JobSecureMem>(len);
        m_password->copyFrom(data, len);
    }

    [[nodiscard]] const crypto::JobSecureMem *passwordSecure() const noexcept
    {
        return m_password.get();
    }

    [[nodiscard]] std::string password(bool include) const
    {
        if (!include || !m_password)
            return {};

        if (m_base64EncodePwd)
            return m_password->toBase64();

        return "********";
    }

    [[nodiscard]] std::string authority(bool includePassword = false) const;

    [[nodiscard]] std::map<std::string, std::string> queryItems() const;
    void setQueryItems(const std::map<std::string, std::string> &items);

    [[nodiscard]] static std::string encodeComponent(const std::string &input);
    [[nodiscard]] static std::string decodeComponent(const std::string &input);

    bool operator==(const JobUrl &other) const noexcept;
    bool operator!=(const JobUrl &other) const noexcept;

    void dump(std::ostream &os = std::cout) const;

    void setbase64EncodePwd(bool b64EncPwd);
    [[nodiscard]] bool base64EncodePwd() const;

    void setPasswdMode(PasswdMode passwdMode);
    [[nodiscard]] PasswdMode passwdMode() const;

    // Migrated when AwsSigner was added.
    [[nodiscard]] static constexpr int hexValue(char c) noexcept
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;

        return -1;
    }

    [[nodiscard]] static constexpr bool isUnreserved(unsigned char c) noexcept
    {
        return (c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') ||
               c == '-' ||
               c == '.' ||
               c == '_' ||
               c == '~';
    }

    static void appendEncodedByte(std::string &output, unsigned char c)
    {
        static constexpr char kHex[] = "0123456789ABCDEF";

        if (isUnreserved(c)) {
            output.push_back(static_cast<char>(c));
            return;
        }

        output.push_back('%');
        output.push_back(kHex[(c >> 4) & 0x0f]);
        output.push_back(kHex[c & 0x0f]);
    }

    [[nodiscard]] static std::string uriEncode(std::string_view input, bool preserveSlash)
    {
        std::string output;
        output.reserve(input.size());

        for (std::size_t i = 0; i < input.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(input[i]);

            if (preserveSlash && c == '/') {
                output.push_back('/');
                continue;
            }

            if (c == '%' && i + 2 < input.size()) {
                const int high = hexValue(input[i + 1]);
                const int low = hexValue(input[i + 2]);

                if (high >= 0 && low >= 0) {
                    const auto decoded = static_cast<unsigned char>((high << 4) | low);
                    appendEncodedByte(output, decoded);
                    i += 2;
                    continue;
                }
            }

            appendEncodedByte(output, c);
        }

        return output;
    }

    [[nodiscard]] std::string encodedPath() const
    {
        if (m_path.empty())
            return "/";

        return uriEncode(m_path, true);
    }

private:
    Scheme m_scheme{Scheme::Unknown};
    std::string m_host;
    uint16_t m_port{0};
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    std::string m_username;
    std::unique_ptr<crypto::JobSecureMem> m_password;
    bool m_valid{false};
    bool m_base64EncodePwd{false};
    PasswdMode m_passwdMode{PasswdMode::Strict};
};

} // namespace job::net