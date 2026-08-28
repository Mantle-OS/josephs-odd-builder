#pragma once

#include <string>
#include <string_view>

#include "job_ihttp_auth.h"
#include "jobnet_export.h"

namespace job::net {

class JobHttpAuthFactory;

class JOBNET_EXPORT Aws4Auth final : public IJobHttpAuth
{
public:
    ~Aws4Auth() override = default;

    Aws4Auth(const Aws4Auth &) = delete;
    Aws4Auth &operator=(const Aws4Auth &) = delete;
    Aws4Auth(Aws4Auth &&) = delete;
    Aws4Auth &operator=(Aws4Auth &&) = delete;

    [[nodiscard]] JobHttpAuthType authType() const noexcept override;
    [[nodiscard]] bool isValid() const noexcept override;

    [[nodiscard]] const std::string &accessKeyId() const noexcept;
    [[nodiscard]] const std::string &region() const noexcept;
    [[nodiscard]] const std::string &service() const noexcept;

    [[nodiscard]] const job::crypto::JobSecureMem &sessionToken() const noexcept;
    [[nodiscard]] bool hasSessionToken() const noexcept;

    void clear() noexcept override;

private:
    friend class JobHttpAuthFactory;

    Aws4Auth(std::string_view accessKeyId,
             std::string_view secretAccessKey,
             std::string_view region,
             std::string_view service);

    Aws4Auth(std::string_view accessKeyId,
             job::crypto::JobSecureMem &&secretAccessKey,
             std::string_view region,
             std::string_view service);

    Aws4Auth(std::string_view accessKeyId,
             std::string_view secretAccessKey,
             std::string_view sessionToken,
             std::string_view region,
             std::string_view service);

    Aws4Auth(std::string_view accessKeyId,
             job::crypto::JobSecureMem &&secretAccessKey,
             job::crypto::JobSecureMem &&sessionToken,
             std::string_view region,
             std::string_view service);

    void setSessionToken(std::string_view token);
    void setSessionToken(job::crypto::JobSecureMem &&token) noexcept;

    std::string               m_accessKeyId;
    std::string               m_region;
    std::string               m_service;
    job::crypto::JobSecureMem m_sessionToken;
};

} // namespace job::net