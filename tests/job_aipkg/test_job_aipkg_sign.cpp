#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include <filesystem>
#include <random>
#include <algorithm>

#include <job_crypto_keys.h>
#include <job_aipkg_sign.h>

using namespace job::aipkg;
using namespace job::crypto;

namespace {

class TempKeyDir
{
public:
    TempKeyDir()
    {
        std::random_device rd;
        m_path = std::filesystem::temp_directory_path() / ("aipkg_sign_test_" + std::to_string(rd()));
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

[[nodiscard]] AiPkgBlock makeBlock(uint64_t height = 42) noexcept
{
    AiPkgBlock b{};
    b.height = height;
    b.prev.fill(0x11);
    b.sth.size = 4;
    b.sth.root.fill(0x22);
    b.sth.ts_ms = 1000;
    return b;
}

[[nodiscard]] AiPkgAttestation makeAttestation() noexcept
{
    AiPkgAttestation a{};
    a.pkg_id = "ZImage_Turbo";
    a.version = "1.0.0";
    a.sha.fill(0x33);
    a.ts_ms = 2000;
    return a;
}

[[nodiscard]] AiPkgDelegate makeDelegate() noexcept
{
    AiPkgDelegate d{};
    d.builder_pub.fill(0x44);
    d.project_id = "test-project";
    d.not_before_ms = 0;
    d.not_after_ms = 999999;
    return d;
}

[[nodiscard]] AiPkgTx makeTx() noexcept
{
    AiPkgTx t{};
    t.to_pub.fill(0x55);
    t.amount = 100;
    t.nonce = 1;
    return t;
}

} // namespace

TEST_CASE("JobAiPkgSign Unified Cryptographic Lifecycle Suite", "[aipkg][sign]")
{
    TempKeyDir dir;
    JobAiPkgSign signer;
    REQUIRE(setupSigner(signer, dir.path(), "id.pub", "id.key"));

    // BLOCK ONE: Usage / Real-World Documentation Examples
    SECTION("Block 1: Canonical round-trip usage guidelines")
    {
        // Example 1: Standard block authority sequencing.
        // Who's the boss? Tony Danza. Who authorized this block? mint_pub.
        SECTION("Signing and statelessly verifying a ledger block")
        {
            AiPkgBlock block = makeBlock();
            REQUIRE(signer.signBlock(block));

            // Verification is static and handles the record's internal key context natively
            REQUIRE(JobAiPkgSign::verifyBlock(block));
        }

        // Example 2: Package developer attestation flow.
        SECTION("Signing and statelessly verifying a manifest attestation")
        {
            AiPkgAttestation attestation = makeAttestation();
            REQUIRE(signer.signAttestation(attestation));
            REQUIRE(JobAiPkgSign::verifyAttestation(attestation));
        }

        // Example 3: Developer authorization updates.
        SECTION("Signing and statelessly verifying a delegate credential")
        {
            AiPkgDelegate delegate = makeDelegate();
            REQUIRE(signer.signDelegate(delegate));
            REQUIRE(JobAiPkgSign::verifyDelegate(delegate));
        }

        // Example 4: Linear transfer settlements.
        SECTION("Signing and statelessly verifying a ledger transaction")
        {
            AiPkgTx tx = makeTx();
            REQUIRE(signer.signTx(tx));
            REQUIRE(JobAiPkgSign::verifyTx(tx));
        }
    }

    // BLOCK TWO: Corner Cases & Invariant Hardening
    SECTION("Block 2: Hardening and boundary abuse constraints")
    {
        SECTION("State checks for uninitialized signing identities")
        {
            JobAiPkgSign emptySigner;
            AiPkgBlock block = makeBlock();

            REQUIRE_FALSE(emptySigner.hasSigningKeys());
            REQUIRE_FALSE(emptySigner.signBlock(block));

            // Confirm no corrupting metadata or partial updates leaked during a rejected sign pass
            Hash32 const zero{};
            REQUIRE(std::equal(zero.begin(), zero.end(), block.sig.begin()));
            REQUIRE(std::equal(zero.begin(), zero.end(), block.mint_pub.begin()));
        }

        SECTION("Data payload parameter tampering detection")
        {
            AiPkgBlock block = makeBlock();
            REQUIRE(signer.signBlock(block));

            // Tamper with a payload data field after authorization has been stamped
            block.height += 1;
            REQUIRE_FALSE(JobAiPkgSign::verifyBlock(block));
        }

        SECTION("Embedded cryptographic boundary key tampering detection")
        {
            AiPkgTx tx = makeTx();
            REQUIRE(signer.signTx(tx));

            // Flip bytes in the identity slot -- Highway to the danger zone Goose...
            tx.from_pub[0] ^= 0xFF;
            REQUIRE_FALSE(JobAiPkgSign::verifyTx(tx));
        }

        SECTION("Signature splicing attacks across asymmetrical data layouts")
        {
            TempKeyDir otherDir;
            JobAiPkgSign otherSigner;
            REQUIRE(setupSigner(otherSigner, otherDir.path(), "other.pub", "other.key"));

            // Build block2 with unique internal data fields so the payloads do not conflict
            AiPkgBlock block1 = makeBlock(42);
            AiPkgBlock block2 = makeBlock(99); // Distinct payload footprint!

            REQUIRE(signer.signBlock(block1));
            REQUIRE(otherSigner.signBlock(block2));

            // Attack trace: Splice signature A onto payload B
            AiPkgBlock spliced = block2;
            spliced.sig = block1.sig;
            spliced.mint_pub = block1.mint_pub;

            // Must reject cleanly because the canvas hash under signature A does not match layout B
            REQUIRE_FALSE(JobAiPkgSign::verifyBlock(spliced));
        }


        SECTION("Garbage embedded pubkey fails verification rather than misbehaving")
        {
            AiPkgBlock block = makeBlock();
            REQUIRE(signer.signBlock(block));

            // A genuinely invalid pubkey (all zeros -- not a valid Ed25519 point)
            // must fail cleanly, not crash or silently accept.
            // Late to the party? Libsodium drops the hammer on bad curve points.
            block.mint_pub.fill(0x00);
            REQUIRE_FALSE(JobAiPkgSign::verifyBlock(block));
        }

    }

// BLOCK THREE: Performance Benchmarks / Stress Validation
#ifdef JOB_TEST_BENCHMARKS
    SECTION("Block 3: Cryptographic multi-part performance scaling boundaries")
    {
        AiPkgBlock block = makeBlock();
        REQUIRE(signer.signBlock(block));

        // Push it to the limit. Evaluate baseline costs for in-memory multi-pass processing.
        BENCHMARK("Stateful block signing execution") {
            return signer.signBlock(block);
        };

        BENCHMARK("Stateless block verification context setup and calculation loop") {
            return JobAiPkgSign::verifyBlock(block);
        };
    }
#endif
}