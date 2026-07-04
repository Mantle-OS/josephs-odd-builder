#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <sodium/crypto_pwhash.h>

#include "job_secure_mem.h"

namespace job::crypto {

class JobPasswordUtils
{
public:
    JobPasswordUtils() = delete;
    ~JobPasswordUtils() = delete;
    [[nodiscard]] static bool hashPasswordForStorage(const JobSecureMem &password,
                                                     std::string &outHash) noexcept;

    [[nodiscard]] static bool verifyPasswordAgainstStorage(const JobSecureMem &password,
                                                           const std::string &storedHash) noexcept;

    [[nodiscard]] static bool deriveKeyFromPassword(JobSecureMem &outDerivedKey,
                                                    const JobSecureMem &password,
                                                    const std::vector<unsigned char> &salt) noexcept;
private:
    static constexpr unsigned long long kOpsLimitInteractive = crypto_pwhash_OPSLIMIT_INTERACTIVE;
    static constexpr std::size_t        kMemLimitInteractive = crypto_pwhash_MEMLIMIT_INTERACTIVE;
};

} // namespace job::crypto