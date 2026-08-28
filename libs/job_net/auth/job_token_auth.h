#pragma once

#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT TokenAuth final : public IJobHttpAuth
{
public:
    ~TokenAuth() override = default;

    TokenAuth(const TokenAuth &) = delete;
    TokenAuth &operator=(const TokenAuth &) = delete;
    TokenAuth(TokenAuth &&) = delete;
    TokenAuth &operator=(TokenAuth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

private:
    friend class JobHttpAuthFactory;

    explicit TokenAuth(std::string_view token);
    explicit TokenAuth(job::crypto::JobSecureMem &&token);
};

} // namespace job::net