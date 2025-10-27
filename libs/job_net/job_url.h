#pragma once

#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <iostream>

#include <job_secure_mem.h>

namespace job::net {

class JobUrl {
public:
    enum class Scheme : uint8_t {
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

    enum class FormatOption : uint8_t {
        None = 0,
        EncodeSpaces = 1 << 0,
        StripTrailingSlash = 1 << 1,
        IncludePassword = 1 << 2
    };

    enum class PasswdMode : uint8_t{
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
    [[nodiscard]] static std::string schemeToString(Scheme scheme);
    [[nodiscard]] static Scheme schemeFromString(const std::string &str);

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

    void setPassword(const std::string &p);

    // Rules
    // 1) check preprocess defines use that if its insecure
    // 2) if the user overrides m_base64EncodePwd = true.
    //    2.a is m_passwdMode == Lenient ? decode the base64 to raw string : just show the base64 string
    // 3) m_base64EncodePwd = false ret "*******" with log
    [[nodiscard]] std::string password(bool include = false) const;

    [[nodiscard]] std::string authority(bool includePassword = false) const;

    [[nodiscard]] std::map<std::string, std::string> queryItems() const;
    void setQueryItems(const std::map<std::string, std::string> &items);

    static std::string encodeComponent(const std::string &input);
    static std::string decodeComponent(const std::string &input);

    bool operator==(const JobUrl &other) const noexcept;
    bool operator!=(const JobUrl &other) const noexcept;

    void dump(std::ostream &os = std::cout) const;

    void setbase64EncodePwd(bool b64EncPwd);
    [[nodiscard]] bool base64EncodePwd() const;
    void setPasswdMode(PasswdMode passwdMode);
    [[nodiscard]] PasswdMode passwdMode() const;

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
