#include "job_aipkg_revocation_index.h"

#include <cstring>

namespace job::aipkg {

namespace {

[[nodiscard]] std::string packageKey(const std::string &pkgId, const std::string &version)
{
    // \x1f (unit separator) as delimiter, not a character either field is
    // realistically expected to contain, avoids a pkg_id/version
    // concatenation collision (e.g. "foo"+"1.0" vs "foo1"+".0").
    return pkgId + '\x1f' + version;
}

} // namespace

bool JobAiPkgRevocationIndex::addRevoke(const AiPkgRevoke &revoke) noexcept
{
    auto const kind = static_cast<RevokeKind>(revoke.kind);

    switch (kind) {
    case RevokeKind::Key: {
        Hash32 key{};
        std::copy(revoke.target_hash.begin(), revoke.target_hash.end(), key.begin());
        m_revokedKeys[key] = revoke;
        return true;
    }
    case RevokeKind::Attestation: {
        Hash32 key{};
        std::copy(revoke.target_hash.begin(), revoke.target_hash.end(), key.begin());
        m_revokedAttestations[key] = revoke;
        return true;
    }
    case RevokeKind::Package: {
        if (revoke.pkg_id.empty() || revoke.version.empty())
            return false; // malformed: Package revoke requires both
        m_revokedPackages[packageKey(revoke.pkg_id, revoke.version)] = revoke;
        return true;
    }
    default:
        return false; // unrecognized kind
    }
}

bool JobAiPkgRevocationIndex::isKeyRevoked(const Hash32 &publicKey) const noexcept
{
    return m_revokedKeys.find(publicKey) != m_revokedKeys.end();
}

bool JobAiPkgRevocationIndex::isAttestationRevoked(const Hash32 &attestationLeafHash) const noexcept
{
    return m_revokedAttestations.find(attestationLeafHash) != m_revokedAttestations.end();
}

bool JobAiPkgRevocationIndex::isPackageRevoked(const std::string &pkgId, const std::string &version) const noexcept
{
    return m_revokedPackages.find(packageKey(pkgId, version)) != m_revokedPackages.end();
}

std::optional<AiPkgRevoke> JobAiPkgRevocationIndex::keyRevocation(const Hash32 &publicKey) const noexcept
{
    auto const it = m_revokedKeys.find(publicKey);
    if (it == m_revokedKeys.end())
        return std::nullopt;
    return it->second;
}

std::optional<AiPkgRevoke> JobAiPkgRevocationIndex::attestationRevocation(const Hash32 &attestationLeafHash) const noexcept
{
    auto const it = m_revokedAttestations.find(attestationLeafHash);
    if (it == m_revokedAttestations.end())
        return std::nullopt;
    return it->second;
}

std::optional<AiPkgRevoke> JobAiPkgRevocationIndex::packageRevocation(const std::string &pkgId, const std::string &version) const noexcept
{
    auto const it = m_revokedPackages.find(packageKey(pkgId, version));
    if (it == m_revokedPackages.end())
        return std::nullopt;
    return it->second;
}

size_t JobAiPkgRevocationIndex::size() const noexcept
{
    return m_revokedKeys.size() + m_revokedAttestations.size() + m_revokedPackages.size();
}

} // namespace job::aipkg