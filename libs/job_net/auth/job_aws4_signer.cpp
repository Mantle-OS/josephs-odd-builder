#include "job_aws4_signer.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_hmac_sha256.h>
#include <job_secure_mem.h>
#include <job_sha256.h>

#include "job_http_request.h"
#include "job_iana.h"
#include "job_url.h"

namespace job::net {

JobAws4Signer::JobAws4Signer(CredentialsPtr credentials) :
    m_credentials{std::move(credentials)}
{
}

bool JobAws4Signer::sign(JobHttpRequest &request, std::optional<TimePoint> time) const noexcept
{
    if (!m_credentials)
        return false;

    return signRequest(*m_credentials, request, time);
}

bool JobAws4Signer::signRequest(const Aws4Auth &credentials, JobHttpRequest &request, std::optional<TimePoint> time) noexcept
{
    try {
        if (!credentials.isValid() || !request.isValid())
            return false;

        const auto secretAccessKey = credentials.token();
        if (!secretAccessKey || secretAccessKey->empty())
            return false;

        const TimePoint signingTime = time.value_or(std::chrono::system_clock::now());

        char amzDate[17]{};
        char dateStamp[9]{};
        formatIso8601Timestamps(signingTime, amzDate, dateStamp);

        auto &headers = request.headers();
        const std::string host = hostHeaderValue(request.url());

        if (host.empty() || !headers.set(JobIana::IanaHeaders::Host, host) || !headers.set("X-Amz-Date", amzDate))
            return false;

        if (credentials.hasSessionToken()) {
            const auto sessionToken = credentials.sessionToken();
            if (!sessionToken || sessionToken->empty())
                return false;

            const std::string_view tokenView{reinterpret_cast<const char *>(sessionToken->data()), sessionToken->size()};
            if (!headers.set("X-Amz-Security-Token", tokenView))
                return false;
        } else {
            headers.removeAll("X-Amz-Security-Token");
        }

        std::string payloadHash;
        const auto body = request.bodyView();

        if (body.empty()) {
            payloadHash = job::crypto::JobSha256::computeHex(std::string_view{});
        } else {
            const std::string_view bodyView{reinterpret_cast<const char *>(body.data()), body.size()};
            payloadHash = job::crypto::JobSha256::computeHex(bodyView);
        }

        if (payloadHash.empty())
            return false;

        if (credentials.service() == "s3" && !headers.set("X-Amz-Content-SHA256", payloadHash))
            return false;

        std::string signedHeaders;
        const std::string canonical = canonicalRequest(request, payloadHash, signedHeaders);
        if (canonical.empty() || signedHeaders.empty())
            return false;

        const std::string canonicalHash = job::crypto::JobSha256::computeHex(canonical);
        if (canonicalHash.empty())
            return false;

        const std::string toSign = stringToSign(credentials, amzDate, dateStamp, canonicalHash);
        if (toSign.empty())
            return false;

        const std::string signatureValue = signature(credentials, dateStamp, toSign);
        if (signatureValue.empty())
            return false;

        std::string authorization;
        authorization.reserve(credentials.accessKeyId().size() + credentials.region().size() +
                              credentials.service().size() + signedHeaders.size() + signatureValue.size() + 96);

        authorization += "AWS4-HMAC-SHA256 Credential=";
        authorization += credentials.accessKeyId();
        authorization.push_back('/');
        authorization += dateStamp;
        authorization.push_back('/');
        authorization += credentials.region();
        authorization.push_back('/');
        authorization += credentials.service();
        authorization += "/aws4_request, SignedHeaders=";
        authorization += signedHeaders;
        authorization += ", Signature=";
        authorization += signatureValue;

        return headers.set(JobIana::IanaHeaders::Authorization, authorization);
    } catch (...) {
        return false;
    }
}

void JobAws4Signer::formatIso8601Timestamps(TimePoint time, char (&amzDateOut)[17], char (&dateStampOut)[9]) noexcept
{
    using namespace std::chrono;

    const auto secondsTime = floor<seconds>(time);
    const auto dayPoint = floor<days>(secondsTime);

    const year_month_day date{dayPoint};
    const hh_mm_ss timeOfDay{secondsTime - dayPoint};

    const int year = static_cast<int>(date.year());
    const unsigned month = static_cast<unsigned>(date.month());
    const unsigned day = static_cast<unsigned>(date.day());
    const unsigned hour = static_cast<unsigned>(timeOfDay.hours().count());
    const unsigned minute = static_cast<unsigned>(timeOfDay.minutes().count());
    const unsigned second = static_cast<unsigned>(timeOfDay.seconds().count());

    std::snprintf(amzDateOut, sizeof(amzDateOut), "%04d%02u%02uT%02u%02u%02uZ", year, month, day, hour, minute, second);
    std::snprintf(dateStampOut, sizeof(dateStampOut), "%04d%02u%02u", year, month, day);
}

std::string JobAws4Signer::canonicalQuery(const JobUrl &url)
{
    const std::string_view query = url.query();
    if (query.empty())
        return {};

    std::vector<QueryItem> items;
    std::size_t begin = 0;

    while (begin <= query.size()) {
        const std::size_t end = query.find('&', begin);
        const std::string_view component = (end == std::string_view::npos) ? query.substr(begin) : query.substr(begin, end - begin);
        const std::size_t separator = component.find('=');

        const std::string_view rawName = (separator == std::string_view::npos) ? component : component.substr(0, separator);
        const std::string_view rawValue = (separator == std::string_view::npos) ? std::string_view{} : component.substr(separator + 1);

        items.push_back({JobUrl::uriEncode(rawName, false), JobUrl::uriEncode(rawValue, false)});

        if (end == std::string_view::npos)
            break;

        begin = end + 1;
    }

    std::sort(items.begin(), items.end(), [](const QueryItem &a, const QueryItem &b) {
        if (a.name != b.name)
            return a.name < b.name;
        return a.value < b.value;
    });

    std::string output;

    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0)
            output.push_back('&');

        output += items[i].name;
        output.push_back('=');
        output += items[i].value;
    }

    return output;
}

std::string JobAws4Signer::normalizeHeaderValue(std::string_view value)
{
    std::string output;
    output.reserve(value.size());

    bool pendingSpace = false;
    bool hasOutput = false;

    for (const unsigned char c : value) {
        if (std::isspace(c)) {
            if (hasOutput)
                pendingSpace = true;
            continue;
        }

        if (pendingSpace) {
            output.push_back(' ');
            pendingSpace = false;
        }

        output.push_back(static_cast<char>(c));
        hasOutput = true;
    }

    return output;
}

std::string JobAws4Signer::hostHeaderValue(const JobUrl &url)
{
    std::string host = url.host();
    if (host.empty())
        return {};

    const std::uint16_t port = url.port();
    if (port == 0)
        return host;

    const bool defaultPort = (url.scheme() == JobUrl::Scheme::Http && port == 80) ||
                             (url.scheme() == JobUrl::Scheme::Https && port == 443);

    if (!defaultPort) {
        host.push_back(':');
        host += std::to_string(port);
    }

    return host;
}

std::string JobAws4Signer::canonicalRequest(const JobHttpRequest &request, std::string_view payloadHash, std::string &signedHeaders)
{
    signedHeaders.clear();

    const std::string_view method = request.methodString();
    if (method.empty())
        return {};

    const std::string uri = request.url().encodedPath();
    const std::string query = canonicalQuery(request.url());

    std::vector<CanonicalHeader> canonicalHeaders;
    const auto &headers = request.headers();
    canonicalHeaders.reserve(headers.size());

    // JobHttpHeader stores one entry per field line, so a repeated name shows up
    // more than once here. SigV4 wants each signed name exactly once, with every
    // value trimmed, inner whitespace collapsed, and the results joined by a bare
    // comma in the order received. Emit at the name's first field line and skip
    // the rest.
    for (std::size_t i = 0; i < headers.size(); ++i) {
        const std::string_view name = headers.nameAt(i);
        if (name == "authorization")
            continue;


        if (headers.indexOf(name) != i)
            continue;

        const std::vector<std::string_view> lines = headers.values(name);
        std::string value;
        for (std::size_t j = 0; j < lines.size(); ++j) {
            if (j != 0)
                value.push_back(',');

            value += normalizeHeaderValue(lines[j]);
        }
        canonicalHeaders.push_back({std::string(name), std::move(value)});

    }

    std::sort(canonicalHeaders.begin(), canonicalHeaders.end(),
              [](const CanonicalHeader &a, const CanonicalHeader &b) {
                  return a.name < b.name;
              });

    std::string canonicalHeaderBlock;

    for (std::size_t i = 0; i < canonicalHeaders.size(); ++i) {
        const auto &header = canonicalHeaders[i];

        canonicalHeaderBlock += header.name;
        canonicalHeaderBlock.push_back(':');
        canonicalHeaderBlock += header.value;
        canonicalHeaderBlock.push_back('\n');

        if (i != 0)
            signedHeaders.push_back(';');

        signedHeaders += header.name;
    }

    std::string output;
    output.reserve(method.size() + uri.size() + query.size() + canonicalHeaderBlock.size() +
                   signedHeaders.size() + payloadHash.size() + 16);

    output += method;
    output.push_back('\n');
    output += uri;
    output.push_back('\n');
    output += query;
    output.push_back('\n');
    output += canonicalHeaderBlock;
    output.push_back('\n');
    output += signedHeaders;
    output.push_back('\n');
    output += payloadHash;

    return output;
}

std::string JobAws4Signer::stringToSign(const Aws4Auth &credentials,
                                        std::string_view amzDate,
                                        std::string_view dateStamp,
                                        std::string_view canonicalRequestHash)
{
    if (amzDate.empty() || dateStamp.empty() || canonicalRequestHash.empty() ||
        credentials.region().empty() || credentials.service().empty()) {
        return {};
    }

    std::string output;
    output.reserve(amzDate.size() + dateStamp.size() + credentials.region().size() +
                   credentials.service().size() + canonicalRequestHash.size() + 48);

    output += "AWS4-HMAC-SHA256\n";
    output += amzDate;
    output.push_back('\n');
    output += dateStamp;
    output.push_back('/');
    output += credentials.region();
    output.push_back('/');
    output += credentials.service();
    output += "/aws4_request\n";
    output += canonicalRequestHash;

    return output;
}

std::string JobAws4Signer::signature(const Aws4Auth &credentials,
                                     std::string_view dateStamp,
                                     std::string_view stringToSign) noexcept
{
    try {
        const auto secretAccessKey = credentials.token();
        if (!secretAccessKey || secretAccessKey->empty() || dateStamp.empty() || stringToSign.empty())
            return {};

        constexpr std::string_view kAws4Prefix{"AWS4"};
        job::crypto::JobSecureMem initialKey{kAws4Prefix.size() + secretAccessKey->size()};

        if (initialKey.empty())
            return {};

        std::memcpy(initialKey.data(), kAws4Prefix.data(), kAws4Prefix.size());
        std::memcpy(initialKey.data() + kAws4Prefix.size(), secretAccessKey->data(), secretAccessKey->size());

        auto dateKey = job::crypto::JobHmacSha256::computeSecure(dateStamp, initialKey);
        if (dateKey.empty())
            return {};

        auto regionKey = job::crypto::JobHmacSha256::computeSecure(credentials.region(), dateKey);
        if (regionKey.empty())
            return {};

        auto serviceKey = job::crypto::JobHmacSha256::computeSecure(credentials.service(), regionKey);
        if (serviceKey.empty())
            return {};

        auto signingKey = job::crypto::JobHmacSha256::computeSecure("aws4_request", serviceKey);
        if (signingKey.empty())
            return {};

        const auto mac = job::crypto::JobHmacSha256::compute(stringToSign, signingKey);
        return job::crypto::JobHmacSha256::toHex(mac);
    } catch (...) {
        return {};
    }
}

} // namespace job::net