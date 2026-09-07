#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "job_aws4_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpRequest;
class JobUrl;

class JOBNET_EXPORT JobAws4Signer
{
public:
    using Ptr  = std::shared_ptr<JobAws4Signer>;
    using WPtr = std::weak_ptr<JobAws4Signer>;
    using UPtr = std::unique_ptr<JobAws4Signer>;

    using TimePoint = std::chrono::system_clock::time_point;
    using CredentialsPtr = std::shared_ptr<Aws4Auth>;

    explicit JobAws4Signer(CredentialsPtr credentials);
    ~JobAws4Signer() = default;

    JobAws4Signer(const JobAws4Signer &) = delete;
    JobAws4Signer &operator=(const JobAws4Signer &) = delete;
    JobAws4Signer(JobAws4Signer &&) noexcept = default;
    JobAws4Signer &operator=(JobAws4Signer &&) noexcept = default;

    [[nodiscard]] static Ptr createShared(CredentialsPtr credentials)
    {
        return std::make_shared<JobAws4Signer>(std::move(credentials));
    }

    [[nodiscard]] static UPtr createUniq(CredentialsPtr credentials)
    {
        return std::make_unique<JobAws4Signer>(std::move(credentials));
    }

    [[nodiscard]] bool sign(JobHttpRequest &request,
                            std::optional<TimePoint> time = std::nullopt) const noexcept;

    [[nodiscard]] static bool signRequest(const Aws4Auth &credentials,
                                          JobHttpRequest &request,
                                          std::optional<TimePoint> time = std::nullopt) noexcept;

private:
    struct QueryItem
    {
        std::string name;
        std::string value;
    };

    struct CanonicalHeader
    {
        std::string name;
        std::string value;
    };

    static void formatIso8601Timestamps(TimePoint time,
                                        char (&amzDateOut)[17],
                                        char (&dateStampOut)[9]) noexcept;

    [[nodiscard]] static std::string canonicalQuery(const JobUrl &url);

    [[nodiscard]] static std::string normalizeHeaderValue(std::string_view value);

    [[nodiscard]] static std::string hostHeaderValue(const JobUrl &url);

    [[nodiscard]] static std::string canonicalRequest(const JobHttpRequest &request,
                                                      std::string_view payloadHash,
                                                      std::string &signedHeaders);

    [[nodiscard]] static std::string stringToSign(const Aws4Auth &credentials,
                                                  std::string_view amzDate,
                                                  std::string_view dateStamp,
                                                  std::string_view canonicalRequestHash);

    [[nodiscard]] static std::string signature(const Aws4Auth &credentials,
                                               std::string_view dateStamp,
                                               std::string_view stringToSign) noexcept;

    CredentialsPtr m_credentials;
};

} // namespace job::net