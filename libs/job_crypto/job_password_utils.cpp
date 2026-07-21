#include "job_password_utils.h"

#ifndef NDEBUG
#include <job_logger.h>
#endif

#include <sodium.h>

#include "job_crypto_init.h"
namespace job::crypto {

bool JobPasswordUtils::hashPasswordForStorage(const JobSecureMem &password,
                                              std::string &outHash) noexcept
{
    outHash.clear();

    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return false;

    if (password.empty())
        return false;

    char hash[crypto_pwhash_STRBYTES] = {};

    int const result = crypto_pwhash_str(
        hash,
        reinterpret_cast<const char *>(password.data()),
        password.size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE
        );

    if (result != 0) {
        sodium_memzero(hash, sizeof(hash));
        return false;
    }

    outHash.assign(hash);

    sodium_memzero(hash, sizeof(hash));
    return true;
}

bool JobPasswordUtils::verifyPasswordAgainstStorage(const JobSecureMem &password, const std::string &storedHash) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return false;

    if (password.empty() || storedHash.empty())
        return false;

    int const result = crypto_pwhash_str_verify(
        storedHash.c_str(),
        reinterpret_cast<const char*>(password.data()),
        static_cast<unsigned long long>(password.size())
    );
    return (result == 0);
}

bool JobPasswordUtils::deriveKeyFromPassword(JobSecureMem &outDerivedKey,
                                             const JobSecureMem &password,
                                             const std::vector<unsigned char> &salt) noexcept
{
    if (!JobCryptoInit::isInitialized() && !JobCryptoInit::initialize())
        return false;

    if (salt.size() != crypto_pwhash_SALTBYTES) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobPasswordUtils] Derivation aborted: Salt size must equal exactly {} bytes", crypto_pwhash_SALTBYTES);
#endif
        return false;
    }

    // Allocate the underlying key memory footprint slice if unallocated
    if (outDerivedKey.size() != crypto_secretbox_KEYBYTES) {
        outDerivedKey = JobSecureMem(crypto_secretbox_KEYBYTES);
    }

    int const result = crypto_pwhash(
        outDerivedKey.data(),
        outDerivedKey.size(),
        reinterpret_cast<const char*>(password.data()),
        static_cast<unsigned long long>(password.size()),
        salt.data(),
        kOpsLimitInteractive,
        kMemLimitInteractive,
        crypto_pwhash_ALG_ARGON2ID13
        );

    if (result != 0) {
#ifndef NDEBUG
        JOB_LOG_ERROR("[JobPasswordUtils] Deterministic sub-key derivation pass failed.");
#endif
        outDerivedKey.clear(); // Zero out memory state immediately
        return false;
    }

    return true;
}

} // namespace job::crypto