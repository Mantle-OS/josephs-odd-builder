#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <job_secure_mem.h>

#include "job_http_auth_type.h"
#include "jobnet_export.h"

namespace job::net {

class JOBNET_EXPORT IJobHttpAuth
{
public:
    using Ptr  = std::shared_ptr<IJobHttpAuth>;
    using WPtr = std::weak_ptr<IJobHttpAuth>;

    virtual ~IJobHttpAuth() = default;

    IJobHttpAuth(const IJobHttpAuth &) = delete;
    IJobHttpAuth &operator=(const IJobHttpAuth &) = delete;
    IJobHttpAuth(IJobHttpAuth &&) = delete;
    IJobHttpAuth &operator=(IJobHttpAuth &&) = delete;

    [[nodiscard]] virtual JobHttpAuthType authType() const noexcept = 0;
    [[nodiscard]] virtual bool isValid() const noexcept = 0;

    [[nodiscard]] const std::string &headerName() const noexcept;
    [[nodiscard]] const std::string &scheme() const noexcept;

    [[nodiscard]] const job::crypto::JobSecureMem &token() const noexcept;
    [[nodiscard]] bool hasToken() const noexcept;

    virtual void clear() noexcept;

protected:
    IJobHttpAuth() = default;

    IJobHttpAuth(std::string_view headerName,
                 std::string_view scheme,
                 std::string_view token);

    IJobHttpAuth(std::string_view headerName,
                 std::string_view scheme,
                 job::crypto::JobSecureMem &&token);

    void setHeaderName(std::string_view headerName);
    void setScheme(std::string_view scheme);

    void setToken(std::string_view token);
    void setToken(job::crypto::JobSecureMem &&token) noexcept;

    std::string               m_headerName;
    std::string               m_scheme;
    job::crypto::JobSecureMem m_token;
};

} // namespace job::net