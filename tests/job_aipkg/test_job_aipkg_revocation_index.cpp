#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <filesystem>
#include <random>
#include <algorithm>

#include <job_crypto_keys.h>
#include <job_aipkg_sign.h>
#include <job_aipkg_merkle.h>
#include "job_aipkg_revocation_index.h"

using namespace job::aipkg;
using namespace job::crypto;

namespace {

class TempKeyDir
{
public:
    TempKeyDir()
    {
        std::random_device rd;
        m_path = std::filesystem::temp_directory_path() / ("aipkg_revoke_test_" + std::to_string(rd()));
        std::filesystem::create_directories(m_path);
    }

    ~TempKeyDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(m_path, ec);
    }

    [[nodiscard]] std::filesystem::path path() const noexcept { return m_path; }

private:
    std::filesystem::path m_path;
};

[[nodiscard]] bool setupSigner(JobAiPkgSign &signer, const std::filesystem::path &dir, const std::string &pubName, const std::string &priName) noexcept
{
    JobCryptoKeys keys;
    if (!keys.createKeys(JobCryptoKeys::KeyType::Sign))
        return false;

    if (!keys.saveKeys(dir, pubName, priName))
        return false;

    return signer.loadKeyPair(dir / pubName, dir / priName);
}

[[nodiscard]] static AiPkgRevoke makeKeyRevoke(const Hash32 &targetKey) noexcept
{
    AiPkgRevoke r{};
    r.kind = static_cast<uint32_t>(RevokeKind::Key);
    std::copy(targetKey.begin(), targetKey.end(), r.target_hash.begin());
    r.reason = 1; // COMPROMISE
    r.ts_ms = 5000;
    return r;
}

[[nodiscard]] static AiPkgRevoke makeAttestationRevoke(const Hash32 &leafHash) noexcept
{
    AiPkgRevoke r{};
    r.kind = static_cast<uint32_t>(RevokeKind::Attestation);
    std::copy(leafHash.begin(), leafHash.end(), r.target_hash.begin());
    r.reason = 2; // SUPERSEDED
    r.ts_ms = 6000;
    return r;
}

[[nodiscard]] static AiPkgRevoke makePackageRevoke(const std::string &pkgId, const std::string &version) noexcept
{
    AiPkgRevoke r{};
    r.kind = static_cast<uint32_t>(RevokeKind::Package);
    r.pkg_id = pkgId;
    r.version = version;
    r.reason = 3; // POLICY
    r.ts_ms = 7000;
    return r;
}

} // namespace

TEST_CASE("AiPkg Revocation Index and Authentication Suite", "[aipkg][revoke][index]")
{
    TempKeyDir dir;
    JobAiPkgSign signer;
    REQUIRE(setupSigner(signer, dir.path(), "revoke_id.pub", "revoke_id.key"));

    // Set up unique mock targets
    Hash32 mockPubKey{};
    mockPubKey.fill(0xAA);

    std::vector<uint8_t> const mockAttestationBytes = {'A', 'T', 'T', '1'};
    Hash32 const mockLeafHash = JobAiPkgMerkle::leafHash(mockAttestationBytes);

    // ========================================================================
    // BLOCK ONE: Usage / Real-World Documentation Examples
    // ========================================================================
    SECTION("Block 1: Canonical authenticated revocation indexing flow")
    {
        JobAiPkgRevocationIndex index;

        // Example 1: Creating, signing, and registering a public key revocation
        SECTION("Revoking a compromised public key authority")
        {
            AiPkgRevoke revoke = makeKeyRevoke(mockPubKey);

            // Step 1: Sign the revocation object natively using your keypair identity
            REQUIRE(signer.signRevoke(revoke));

            // Step 2: Verify the structure statelessly before throwing it in the registry
            REQUIRE(JobAiPkgSign::verifyRevoke(revoke));

            // Step 3: Insert into the fast lookup database index
            REQUIRE(index.addRevoke(revoke));
            REQUIRE(index.isKeyRevoked(mockPubKey));

            // Step 4: Confirm detailed metrics are fetchable on demand
            auto const record = index.keyRevocation(mockPubKey);
            REQUIRE(record.has_value());
            REQUIRE(record->reason ==  1);
        }

        // Example 2: Revoking an attestation by its Merkle leaf hash footprint
        SECTION("Revoking a deployed binary attestation leaf entry")
        {
            AiPkgRevoke revoke = makeAttestationRevoke(mockLeafHash);
            REQUIRE(signer.signRevoke(revoke));
            REQUIRE(JobAiPkgSign::verifyRevoke(revoke));

            REQUIRE(index.addRevoke(revoke));
            REQUIRE(index.isAttestationRevoked(mockLeafHash));
        }

        // Example 3: Revoking entire explicit package coordinates
        SECTION("Revoking a specific package name/version mapping")
        {
            std::string const pkgId = "lib_vulnerability";
            std::string const version = "2.4.1";

            AiPkgRevoke revoke = makePackageRevoke(pkgId, version);
            REQUIRE(signer.signRevoke(revoke));
            REQUIRE(JobAiPkgSign::verifyRevoke(revoke));

            REQUIRE(index.addRevoke(revoke));
            REQUIRE(index.isPackageRevoked(pkgId, version));
        }
    }

    // ========================================================================
    // BLOCK TWO: Corner Cases & Invariant Hardening
    // ========================================================================
    SECTION("Block 2: Hardening invariants and adversarial injection boundaries")
    {
        JobAiPkgRevocationIndex index;

        SECTION("Rejecting invalid schema kind identifiers")
        {
            AiPkgRevoke badRevoke = makeKeyRevoke(mockPubKey);
            badRevoke.kind = 99; // Totally fictitious category type

            REQUIRE_FALSE(index.addRevoke(badRevoke));
        }

        SECTION("Enforcing field requirements on conditional type variations")
        {
            // Package revocations must contain valid coordinates, blank ones are trash
            AiPkgRevoke malformedPkg = makePackageRevoke("", "1.0.0");
            REQUIRE_FALSE(index.addRevoke(malformedPkg));
        }

        SECTION("Authentication tampering and data modification detection")
        {
            AiPkgRevoke revoke = makeKeyRevoke(mockPubKey);
            REQUIRE(signer.signRevoke(revoke));

            // Post-signature data mutation attempt
            // Who's the boss? The signature envelope. Changing the reason breaks validation.
            revoke.reason = 3; // Swapping COMPROMISE for POLICY
            REQUIRE_FALSE(JobAiPkgSign::verifyRevoke(revoke));
        }

        SECTION("Splicing authenticated revocation tokens across mismatched targets")
        {
            Hash32 anotherKey{};
            anotherKey.fill(0xBB);

            AiPkgRevoke r1 = makeKeyRevoke(mockPubKey);
            AiPkgRevoke r2 = makeKeyRevoke(anotherKey); // Distinct target data space!

            REQUIRE(signer.signRevoke(r1));
            REQUIRE(signer.signRevoke(r2));

            // Transpose signature block A onto target data payload alignment footprint B
            AiPkgRevoke spliced = r2;
            spliced.signature = r1.signature;
            spliced.signer_pub = r1.signer_pub;

            // Must reject cleanly -- Highway to the danger zone Goose...
            REQUIRE_FALSE(JobAiPkgSign::verifyRevoke(spliced));
        }

        SECTION("Garbage embedded signer keys fail validation cleanly")
        {
            AiPkgRevoke revoke = makeKeyRevoke(mockPubKey);
            REQUIRE(signer.signRevoke(revoke));

            // Late to the party? Bad curve points are ejected straight away.
            revoke.signer_pub.fill(0x00);
            REQUIRE_FALSE(JobAiPkgSign::verifyRevoke(revoke));
        }

        SECTION("Missing index lookup hits return clean structural options")
        {
            Hash32 unknownKey{};
            unknownKey.fill(0x77);

            REQUIRE_FALSE(index.isKeyRevoked(unknownKey));
            REQUIRE(index.keyRevocation(unknownKey) == std::nullopt);
        }
    }

// ========================================================================
// BLOCK THREE: Performance Benchmarks / Stress Validation
// ========================================================================
#ifdef JOB_TEST_BENCHMARKS
    SECTION("Block 3: Execution costs and lookup performance metrics")
    {
        AiPkgRevoke r = makeKeyRevoke(mockPubKey);
        REQUIRE(signer.signRevoke(r));

        JobAiPkgRevocationIndex index;
        REQUIRE(index.addRevoke(r));

        // Push it to the limit. Evaluate baseline cost for fast lookups and payload checking.
        BENCHMARK("Authenticated revocation signature checking") {
            return JobAiPkgSign::verifyRevoke(r);
        };

        BENCHMARK("Hash32 structure dictionary mapping retrieval speed") {
            return index.isKeyRevoked(mockPubKey);
        };
    }
#endif
}