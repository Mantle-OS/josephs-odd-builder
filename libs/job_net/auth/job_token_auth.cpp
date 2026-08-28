#include "job_token_auth.h"

namespace job::net {

TokenAuth::TokenAuth(std::string_view token) :
    IJobHttpAuth("Authorization", "Token", token)
{
}

TokenAuth::TokenAuth(job::crypto::JobSecureMem &&token) :
    IJobHttpAuth("Authorization", "Token", std::move(token))
{
}

JobHttpAuthType TokenAuth::authType() const noexcept
{
    return JobHttpAuthType::Token;
}

bool TokenAuth::isValid() const noexcept
{
    return hasToken();
}

} // namespace job::net