#include "job_api_key_auth.h"

namespace job::net {

ApiKeyAuth::ApiKeyAuth(std::string_view token,
                       std::string_view headerName) :
    IJobHttpAuth(headerName, {}, token)
{
}

ApiKeyAuth::ApiKeyAuth(job::crypto::JobSecureMem &&token,
                       std::string_view headerName) :
    IJobHttpAuth(headerName, {}, std::move(token))
{
}

JobHttpAuthType ApiKeyAuth::authType() const noexcept
{
    return JobHttpAuthType::ApiKey;
}

bool ApiKeyAuth::isValid() const noexcept
{
    return !headerName().empty() && hasToken();
}

} // namespace job::net