#include "job_basic_auth.h"

#include <utility>

namespace job::net {

BasicAuth::BasicAuth(std::string_view username,
                     std::string_view password) :
    IJobHttpAuth("Authorization", "Basic", password),
    m_username(username)
{

}

BasicAuth::BasicAuth(std::string_view username,
                     job::crypto::JobSecureMem &&password) :
    IJobHttpAuth("Authorization", "Basic", std::move(password)),
    m_username(username)
{

}

JobHttpAuthType BasicAuth::authType() const noexcept
{
    return JobHttpAuthType::Basic;
}

bool BasicAuth::isValid() const noexcept
{
    return !m_username.empty() &&
           m_username.find(':') == std::string::npos;
}

const std::string &BasicAuth::username() const noexcept
{
    return m_username;
}

void BasicAuth::clear() noexcept
{
    IJobHttpAuth::clear();
    m_username.clear();
}

} // namespace job::net