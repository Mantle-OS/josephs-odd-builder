#pragma once

#include <memory>
#include <string_view>

#include <job_secure_mem.h>

#include "job_api_key_auth.h"
#include "job_aws4_auth.h"
#include "job_basic_auth.h"
#include "job_bearer_auth.h"
#include "job_custom_auth.h"
#include "job_http_auth_type.h"
#include "job_ihttp_auth.h"
#include "job_token_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT JobHttpAuthFactory
{
public:
    JobHttpAuthFactory() = delete;
    ~JobHttpAuthFactory() = delete;

    JobHttpAuthFactory(const JobHttpAuthFactory &) = delete;
    JobHttpAuthFactory &operator=(const JobHttpAuthFactory &) = delete;
    JobHttpAuthFactory(JobHttpAuthFactory &&) = delete;
    JobHttpAuthFactory &operator=(JobHttpAuthFactory &&) = delete;

    [[nodiscard]] static IJobHttpAuth::Ptr create(JobHttpAuthType type,
                                                  job::crypto::JobSecureMem::Ptr token = nullptr,
                                                  job::crypto::JobSecureMem::Ptr sessionToken = nullptr,
                                                  std::string_view id = {},
                                                  std::string_view region = {},
                                                  std::string_view service = {})
    {
        switch (type) {
        case JobHttpAuthType::Bearer:
            if (!token)
                return nullptr;
            return std::shared_ptr<BearerAuth>(new BearerAuth{token});

        case JobHttpAuthType::Token:
            if (!token)
                return nullptr;
            return std::shared_ptr<TokenAuth>(new TokenAuth{token});

        case JobHttpAuthType::ApiKey:
            if (!token)
                return nullptr;
            return std::shared_ptr<ApiKeyAuth>(new ApiKeyAuth{id, token});

        case JobHttpAuthType::Basic:
            return std::shared_ptr<BasicAuth>(new BasicAuth{id, token});

        case JobHttpAuthType::Aws4HmacSha256:
            if (!token)
                return nullptr;

            if (sessionToken)
                return std::shared_ptr<Aws4Auth>(new Aws4Auth{id, token, sessionToken, region, service});

            return std::shared_ptr<Aws4Auth>(new Aws4Auth{id, token, region, service});

        case JobHttpAuthType::Custom:
            if (!token)
                return nullptr;
            return std::shared_ptr<CustomAuth>(new CustomAuth{id, token});

        case JobHttpAuthType::Unknown:
        default:
            return nullptr;
        }
    }

    [[nodiscard]] static IJobHttpAuth::Ptr create(JobHttpAuthType type,
                                                  std::string_view token,
                                                  std::string_view sessionToken = {},
                                                  std::string_view id = {},
                                                  std::string_view region = {},
                                                  std::string_view service = {})
    {
        switch (type) {
        case JobHttpAuthType::Bearer:
            if (token.empty())
                return nullptr;
            return std::shared_ptr<BearerAuth>(new BearerAuth{token});

        case JobHttpAuthType::Token:
            if (token.empty())
                return nullptr;
            return std::shared_ptr<TokenAuth>(new TokenAuth{token});

        case JobHttpAuthType::ApiKey:
            if (token.empty())
                return nullptr;
            return std::shared_ptr<ApiKeyAuth>(new ApiKeyAuth{id, token});

        case JobHttpAuthType::Basic:
            return std::shared_ptr<BasicAuth>(new BasicAuth{id, token});

        case JobHttpAuthType::Aws4HmacSha256:
            if (token.empty())
                return nullptr;

            if (!sessionToken.empty())
                return std::shared_ptr<Aws4Auth>(new Aws4Auth{id, token, sessionToken, region, service});

            return std::shared_ptr<Aws4Auth>(new Aws4Auth{id, token, region, service});

        case JobHttpAuthType::Custom:
            if (token.empty())
                return nullptr;
            return std::shared_ptr<CustomAuth>(new CustomAuth{id, token});

        case JobHttpAuthType::Unknown:
        default:
            return nullptr;
        }
    }
};

} // namespace job::net