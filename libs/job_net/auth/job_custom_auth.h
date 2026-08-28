#pragma once

#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT CustomAuth final : public IJobHttpAuth
{
public:
    ~CustomAuth() override = default;

    CustomAuth(const CustomAuth &) = delete;
    CustomAuth &operator=(const CustomAuth &) = delete;
    CustomAuth(CustomAuth &&) = delete;
    CustomAuth &operator=(CustomAuth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

private:
    friend class JobHttpAuthFactory;

    CustomAuth(std::string_view scheme,
               std::string_view token);

    CustomAuth(std::string_view scheme,
               job::crypto::JobSecureMem &&token);
};

} // namespace job::net