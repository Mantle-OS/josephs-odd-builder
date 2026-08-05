#pragma once

#include <filesystem>

#include "job_secure_mem.h"
#include "job_ssl_options.h"
#include "jobcrypto_export.h"

// NOTE ALL PLATFORM IMPLEMENTATIONS MUST DEFINE THE FOLLOWING
// [[nodiscard]] static bool generate(const JobSslOptions &opt, const std::filesystem::path &cert, const std::filesystem::path &priKey);
// [[nodiscard]] static bool generate(const JobSslOptions &opt, const std::filesystem::path &idPath, const JobSecureMem &pass);

namespace job::crypto {

class JOBCRYPTO_EXPORT JobX509Generator {
public:
    JobX509Generator() = delete;
    ~JobX509Generator() = delete;

    JobX509Generator(const JobX509Generator &) = delete;
    JobX509Generator &operator=(const JobX509Generator &) = delete;
    JobX509Generator(JobX509Generator &&) = delete;
    JobX509Generator &operator=(JobX509Generator &&) = delete;

    [[nodiscard]] static bool generate(const JobSslOptions &opt, const std::filesystem::path &cert, const std::filesystem::path &priKey);
    [[nodiscard]] static bool generate(const JobSslOptions &opt, const std::filesystem::path &idPath, const JobSecureMem &pass);
};

} // namespace job::crypto