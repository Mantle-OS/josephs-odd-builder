#include "job_url.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <sodium.h>

namespace job::io {
namespace {
static const std::regex kUrlRegex(
    R"(^([a-zA-Z][a-zA-Z0-9+\-.]*):\/\/(?:([^:@]+)(?::([^@]+))?@)?([^:\/?#]+)?(?::(\d+))?(\/[^?#]*)?(?:\?([^#]*))?(?:#(.*))?$)",
    std::regex::ECMAScript
    );
}

using crypto::JobSecureMem;

JobUrl::JobUrl(const std::string &urlString)
{
    parse(urlString);
}

JobUrl::JobUrl(const JobUrl &other)
{
    *this = other;
}

JobUrl::JobUrl(JobUrl &&other) noexcept = default;

JobUrl &JobUrl::operator=(const JobUrl &other)
{
    if (this != &other) {
        m_scheme = other.m_scheme;
        m_host = other.m_host;
        m_port = other.m_port;
        m_path = other.m_path;
        m_query = other.m_query;
        m_fragment = other.m_fragment;
        m_username = other.m_username;
        m_valid = other.m_valid;
        m_base64EncodePwd = other.m_base64EncodePwd;
        m_passwdMode = other.m_passwdMode;

        if (other.m_password) {
            m_password = std::make_unique<JobSecureMem>(other.m_password->size());
            m_password->copyFrom(other.m_password->data(), other.m_password->size());
        } else {
            m_password.reset();
        }
    }
    return *this;
}

JobUrl &JobUrl::operator=(JobUrl &&other) noexcept = default;

JobUrl::~JobUrl() = default;

bool JobUrl::parse(const std::string &urlString)
{
    clear();
    std::smatch match;
    if (!std::regex_match(urlString, match, kUrlRegex)) {
        m_valid = false;
        if (m_passwdMode == PasswdMode::Strict)
            throw std::invalid_argument("Invalid URL format: " + urlString);
        return false;
    }

    try {
        m_scheme = schemeFromString(match[1]);
        if (match[2].matched)
            m_username = match[2];
        if (match[3].matched)
            setPassword(match[3]);
        if (match[4].matched)
            m_host = match[4];
        if (match[5].matched)
            m_port = static_cast<uint16_t>(std::stoi(match[5]));
        if (match[6].matched)
            m_path = match[6];
        if (match[7].matched)
            m_query = match[7];
        if (match[8].matched)
            m_fragment = match[8];
    } catch (...) {
        if (m_passwdMode == PasswdMode::Strict)
            throw;
        m_valid = false;
        return false;
    }

    m_valid = (m_scheme != Scheme::Unknown) && (!m_host.empty() || m_scheme == Scheme::File || m_scheme == Scheme::Unix);
    if (!m_valid && m_passwdMode == PasswdMode::Strict) {
        throw std::invalid_argument("Invalid URL format (strict): " + urlString);
    }
    return m_valid;
}

std::string JobUrl::toString(uint8_t options) const
{
    std::ostringstream oss;

    if (m_scheme != Scheme::Unknown)
        oss << schemeToString(m_scheme) << "://";

    if (!m_username.empty()) {
        oss << encodeComponent(m_username);

        if (options & static_cast<uint8_t>(FormatOption::IncludePassword)) {
            if (m_password) {
                if (m_base64EncodePwd) {
                    std::string encoded = m_password->toBase64();
                    oss << ':' << encoded;
                } else {
                    oss << ":********";
                }
            }
        }
        oss << '@';
    }

    oss << m_host;
    if (m_port)
        oss << ':' << m_port;
    if (!m_path.empty())
        oss << m_path;
    if (!m_query.empty())
        oss << '?' << m_query;
    if (!m_fragment.empty())
        oss << '#' << m_fragment;

    std::string ret = oss.str();

    if (options & static_cast<uint8_t>(FormatOption::StripTrailingSlash)) {
        while (!ret.empty() && ret.back() == '/')
            ret.pop_back();
    }

    return ret;
}


bool JobUrl::isValid() const noexcept
{
    return m_valid;
}

bool JobUrl::isEmpty() const noexcept
{
    return m_scheme == Scheme::Unknown && m_host.empty();
}

size_t JobUrl::size() const noexcept
{
    return toString().size();
}

void JobUrl::clear() noexcept
{
    m_scheme = Scheme::Unknown;
    m_host.clear();
    m_port = 0;
    m_path.clear();
    m_query.clear();
    m_fragment.clear();
    m_username.clear();
    m_password.reset();
    m_valid = false;
}

JobUrl::Scheme JobUrl::scheme() const noexcept
{
    return m_scheme;
}
void JobUrl::setScheme(Scheme scheme)
{
    m_scheme = scheme;
}
void JobUrl::setScheme(const std::string &schemeStr)
{
    m_scheme = schemeFromString(schemeStr);
}

std::string JobUrl::schemeToString(Scheme scheme)
{
    switch (scheme) {
    case Scheme::File:
        return "file";
    case Scheme::Http:
        return "http";
    case Scheme::Https:
        return "https";
    case Scheme::Tcp:
        return "tcp";
    case Scheme::Udp:
        return "udp";
    case Scheme::Unix:
        return "unix";
    case Scheme::Git:
        return "git";
    case Scheme::GitSm:
        return "gitsm";
    case Scheme::Svn:
        return "svn";
    case Scheme::Ssh:
        return "ssh";
    case Scheme::Serial:
        return "serial";
    default:
        return "unknown";
    }
}

JobUrl::Scheme JobUrl::schemeFromString(const std::string &str)
{
    std::string lower;
    lower.resize(str.size());
    std::transform(str.begin(), str.end(), lower.begin(), ::tolower);

    if (lower == "file")
        return Scheme::File;
    if (lower == "http")
        return Scheme::Http;
    if (lower == "https")
        return Scheme::Https;
    if (lower == "tcp")
        return Scheme::Tcp;
    if (lower == "udp")
        return Scheme::Udp;
    if (lower == "unix")
        return Scheme::Unix;
    if (lower == "git")
        return Scheme::Git;
    if (lower == "gitsm")
        return Scheme::GitSm;
    if (lower == "svn")
        return Scheme::Svn;
    if (lower == "ssh")
        return Scheme::Ssh;
    if (lower == "serial")
        return Scheme::Serial;
    return Scheme::Unknown;
}

// Components
const std::string &JobUrl::host() const noexcept
{
    return m_host;
}
void JobUrl::setHost(const std::string &h)
{
    m_host = h;
}

uint16_t JobUrl::port() const noexcept
{
    return m_port;
}
void JobUrl::setPort(uint16_t p)
{
    m_port = p;
}

const std::string &JobUrl::path() const noexcept
{
    return m_path;
}
void JobUrl::setPath(const std::string &p)
{
    m_path = p;
}

const std::string &JobUrl::query() const noexcept
{
    return m_query;
}
void JobUrl::setQuery(const std::string &q)
{
    m_query = q;
}

const std::string &JobUrl::fragment() const noexcept
{
    return m_fragment;
}
void JobUrl::setFragment(const std::string &f)
{
    m_fragment = f;
}

const std::string &JobUrl::username() const noexcept
{
    return m_username;
}
void JobUrl::setUsername(const std::string &u)
{
    m_username = u;
}

void JobUrl::setPassword(const std::string &p)
{
    if (p.empty()) {
        m_password.reset();
        return;
    }
    m_password = std::make_unique<JobSecureMem>(p.size());
    m_password->copyFrom(p.data(), p.size());
}

std::string JobUrl::password(bool include) const
{
    if (!include || !m_password)
        return {};

#if defined(JOB_SECUREMEM_ALLOW_STRING)
    // Direct access mode (for debug or local builds)
    return m_password->toString();
#else
    // Secure mode (default)
    if (m_base64EncodePwd) {
        std::string encoded = m_password->toBase64();

        if (m_passwdMode == PasswdMode::Lenient) {
            // Decode immediately into readable string for lenient inspection
            return m_password->fromBase64toString(encoded);
        }

        // Otherwise return base64 string (safe for export/logging)
        return encoded;
    }

    // Masked fallback
    return "********";
#endif
}


std::string JobUrl::authority(bool includePassword) const
{
    std::ostringstream oss;
    if (!m_username.empty()) {
        oss << m_username;
        if (includePassword && m_password)
            oss << ':' << password(true);
        oss << '@';
    }
    oss << m_host;
    if (m_port)
        oss << ':' << m_port;
    return oss.str();
}

std::map<std::string, std::string> JobUrl::queryItems() const
{
    std::map<std::string, std::string> items;
    std::istringstream iss(m_query);
    std::string pair;
    while (std::getline(iss, pair, '&')) {
        auto eqPos = pair.find('=');
        if (eqPos != std::string::npos)
            items[decodeComponent(pair.substr(0, eqPos))] = decodeComponent(pair.substr(eqPos + 1));
    }
    return items;
}

void JobUrl::setQueryItems(const std::map<std::string, std::string> &items)
{
    std::ostringstream oss;
    bool first = true;
    for (const auto &[k, v] : items) {
        if (!first) oss << '&';
        first = false;
        oss << encodeComponent(k) << '=' << encodeComponent(v);
    }
    m_query = oss.str();
}

std::string JobUrl::encodeComponent(const std::string &input)
{
    std::ostringstream oss;
    for (unsigned char c : input) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            oss << c;
        else
            oss << '%' << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(c);
    }
    return oss.str();
}

std::string JobUrl::decodeComponent(const std::string &input)
{
    std::ostringstream oss;
    for (size_t i = 0; i < input.size();) {
        if (input[i] == '%' && i + 2 < input.size()) {
            int value = 0;
            std::istringstream iss(input.substr(i + 1, 2));
            if (iss >> std::hex >> value)
                oss << static_cast<char>(value);
            i += 3;
        } else {
            oss << input[i++];
        }
    }
    return oss.str();
}

bool JobUrl::operator==(const JobUrl &other) const noexcept
{
    return m_scheme == other.m_scheme && m_host == other.m_host &&
           m_port == other.m_port && m_path == other.m_path &&
           m_query == other.m_query && m_fragment == other.m_fragment &&
           m_username == other.m_username;
}

bool JobUrl::operator!=(const JobUrl &other) const noexcept
{
    return !(*this == other);
}

void JobUrl::dump(std::ostream &os) const
{
    os << "Scheme: " << schemeToString(m_scheme) << '\n'
       << "Host: " << m_host << '\n'
       << "Port: " << m_port << '\n'
       << "Path: " << m_path << '\n'
       << "Query: " << m_query << '\n'
       << "Fragment: " << m_fragment << '\n'
       << "Username: " << m_username << '\n'
       << "Password: " << (m_password ? "[secure]" : "(none)") << '\n'
       << "Valid: " << std::boolalpha << m_valid << '\n';
}

} // namespace job::io
