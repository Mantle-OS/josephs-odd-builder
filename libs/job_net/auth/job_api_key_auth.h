#pragma once

#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT ApiKeyAuth final : public IJobHttpAuth
{
public:
    ~ApiKeyAuth() override = default;

    ApiKeyAuth(const ApiKeyAuth &) = delete;
    ApiKeyAuth &operator=(const ApiKeyAuth &) = delete;
    ApiKeyAuth(ApiKeyAuth &&) = delete;
    ApiKeyAuth &operator=(ApiKeyAuth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

private:
    friend class JobHttpAuthFactory;

    ApiKeyAuth(std::string_view headerName = "X-API-Key",
               std::string_view token = {});

    ApiKeyAuth(std::string_view headerName = "X-API-Key",
               job::crypto::JobSecureMem::Ptr token = nullptr);
};

} // namespace job::net