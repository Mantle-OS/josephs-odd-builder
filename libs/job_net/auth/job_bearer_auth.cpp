#include "job_bearer_auth.h"

namespace job::net {

BearerAuth::BearerAuth(std::string_view token) :
    IJobHttpAuth("Authorization", "Bearer", token)
{
}

BearerAuth::BearerAuth(job::crypto::JobSecureMem &&token) :
    IJobHttpAuth("Authorization", "Bearer", std::move(token))
{
}

JobHttpAuthType BearerAuth::authType() const noexcept
{
    return JobHttpAuthType::Bearer;
}

bool BearerAuth::isValid() const noexcept
{
    return hasToken();
}

} // namespace job::net