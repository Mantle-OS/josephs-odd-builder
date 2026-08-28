#include "job_custom_auth.h"

#include <utility>

namespace job::net {

CustomAuth::CustomAuth(std::string_view scheme, std::string_view token) :
    IJobHttpAuth("Authorization", scheme, token)
{
}

CustomAuth::CustomAuth(std::string_view scheme, job::crypto::JobSecureMem &&token) :
    IJobHttpAuth("Authorization", scheme, std::move(token))
{
}

JobHttpAuthType CustomAuth::authType() const noexcept
{
    return JobHttpAuthType::Custom;
}

bool CustomAuth::isValid() const noexcept
{
    return !scheme().empty() && hasToken();
}

} // namespace job::net