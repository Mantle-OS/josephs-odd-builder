#pragma once

#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT BearerAuth final : public IJobHttpAuth
{
public:
    ~BearerAuth() override = default;

    BearerAuth(const BearerAuth &) = delete;
    BearerAuth &operator=(const BearerAuth &) = delete;
    BearerAuth(BearerAuth &&) = delete;
    BearerAuth &operator=(BearerAuth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

private:
    friend class JobHttpAuthFactory;

    explicit BearerAuth(std::string_view token);
    explicit BearerAuth(job::crypto::JobSecureMem &&token);
};

} // namespace job::net