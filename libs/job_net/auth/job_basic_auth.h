#pragma once

#include <string>
#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT BasicAuth final : public IJobHttpAuth
{
public:
    ~BasicAuth() override = default;

    BasicAuth(const BasicAuth &) = delete;
    BasicAuth &operator=(const BasicAuth &) = delete;
    BasicAuth(BasicAuth &&) = delete;
    BasicAuth &operator=(BasicAuth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

    [[nodiscard]] const std::string &username() const noexcept;

    void clear() noexcept override;

private:
    friend class JobHttpAuthFactory;

    BasicAuth(std::string_view username, std::string_view password);
    BasicAuth(std::string_view username, job::crypto::JobSecureMem &&password);

    std::string m_username;
};

} // namespace job::net