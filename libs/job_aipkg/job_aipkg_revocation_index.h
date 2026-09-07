#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_map>

#include <aipkg_ledger/ledger_revoke.hpp>

#include "job_aipkg_utils.h"
#include "jobaipkg_export.h"

namespace job::aipkg {

using job::serializer::generated::AiPkgRevoke;

enum class RevokeKind : uint32_t
{
    Key         = 1,
    Attestation = 2,
    Package     = 3,
};

class JOBAIPKG_EXPORT JobAiPkgRevocationIndex
{
public:
    JobAiPkgRevocationIndex() = default;

    [[nodiscard]] bool addRevoke(const AiPkgRevoke &revoke) noexcept;

    [[nodiscard]] bool isKeyRevoked(const Hash32 &publicKey) const noexcept;
    [[nodiscard]] bool isAttestationRevoked(const Hash32 &attestationLeafHash) const noexcept;
    [[nodiscard]] bool isPackageRevoked(const std::string &pkgId,
                                        const std::string &version) const noexcept;

    [[nodiscard]] std::optional<AiPkgRevoke> keyRevocation(const Hash32 &publicKey) const noexcept;
    [[nodiscard]] std::optional<AiPkgRevoke> attestationRevocation(const Hash32 &attestationLeafHash) const noexcept;
    [[nodiscard]] std::optional<AiPkgRevoke> packageRevocation(const std::string &pkgId,
                                                               const std::string &version) const noexcept;

    [[nodiscard]] size_t size() const noexcept;

private:
    struct Hash32Hasher
    {
        [[nodiscard]] size_t operator()(const Hash32 &h) const noexcept
        {
            size_t out = 0;
            std::memcpy(&out, h.data(), sizeof(out));
            return out;
        }
    };

    std::unordered_map<Hash32, AiPkgRevoke, Hash32Hasher>   m_revokedKeys;
    std::unordered_map<Hash32, AiPkgRevoke, Hash32Hasher>   m_revokedAttestations;
    std::unordered_map<std::string, AiPkgRevoke>            m_revokedPackages; // key: pkgId + "\x1f" + version
};

}