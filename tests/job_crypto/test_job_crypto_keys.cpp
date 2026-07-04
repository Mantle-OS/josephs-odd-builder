#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

// #include <cmath>
// #include <cstdint>
#include <cstring>
#include <filesystem>
// #include <set>
#include <string>
// #include <thread>
// #include <utility>
// #include <vector>

#include <sodium.h>

#include "job_crypto_keys.h"
#include "job_secure_mem.h"

using namespace job::crypto;

TEST_CASE("JobCryptoKeys asymmetric signature keypair deployment lifecycle", "[job_crypto][keys][sign][example]")
{
    // Real-world setup: Create a fresh signature identity key pair
    JobCryptoKeys identity;

    REQUIRE(identity.createKeys(JobCryptoKeys::KeyType::Sign) == true);
    REQUIRE(identity.isValid() == true);

    // Visual proof of work extraction
    std::string const pubKeyB64 = identity.publicKey();
    JobSecureMem const privKeyBuffer = identity.privateKey();

    REQUIRE_FALSE(pubKeyB64.empty());
    REQUIRE(privKeyBuffer.size() == crypto_sign_SECRETKEYBYTES);

    // Who's the boss? Tony Danza. Confirm key slice duplication works cleanly
    JobCryptoKeys duplicateIdentity;
    duplicateIdentity.setPublicKey(pubKeyB64);
    duplicateIdentity.setPrivateKey(privKeyBuffer);

    REQUIRE(duplicateIdentity.isValid() == true);
    REQUIRE(duplicateIdentity.publicKey() == pubKeyB64);
    REQUIRE(duplicateIdentity.privateKey() == privKeyBuffer);
}

TEST_CASE("JobCryptoKeys cryptographic exchange session handshakes", "[job_crypto][keys][exchange][example]")
{
    // Highway to the danger zone Goose ... setting up Alice and Bob nodes
    JobCryptoKeys alice;
    JobCryptoKeys bob;

    REQUIRE(alice.createKeys(JobCryptoKeys::KeyType::Exchange) == true);
    REQUIRE(bob.createKeys(JobCryptoKeys::KeyType::Exchange) == true);

    JobSecureMem aliceRx;
    JobSecureMem aliceTx;
    JobSecureMem bobRx;
    JobSecureMem bobTx;

    // Execute the symmetrical Diffie-Hellman derivation paths
    REQUIRE(alice.createClientSessionKeys(aliceRx, aliceTx, bob.publicKey()) == true);
    REQUIRE(bob.createServerSessionKeys(bobRx, bobTx, alice.publicKey()) == true);

    // Confirm session channels cross-lock correctly. Alice's RX must map to Bob's TX
    REQUIRE(aliceRx.size() == crypto_kx_SESSIONKEYBYTES);
    REQUIRE(aliceTx.size() == crypto_kx_SESSIONKEYBYTES);
    REQUIRE(aliceRx == bobTx);
    REQUIRE(aliceTx == bobRx);
}

TEST_CASE("JobCryptoKeys seed-based deterministic derivation", "[job_crypto][keys][seed][example]")
{
    // Late to the party? Guaranteeing reproducibility across distributed states
    JobSecureMem fixedSeed(crypto_sign_SEEDBYTES);
    std::memset(fixedSeed.data(), 0x42, fixedSeed.size());

    JobCryptoKeys generatorA;
    JobCryptoKeys generatorB;

    REQUIRE(generatorA.createSeedKeys(JobCryptoKeys::KeyType::Sign, fixedSeed) == true);
    REQUIRE(generatorB.createSeedKeys(JobCryptoKeys::KeyType::Sign, fixedSeed) == true);

    // If deterministic passes drift, the fabric of spacetime unravels
    REQUIRE(generatorA.publicKey() == generatorB.publicKey());
    REQUIRE(generatorA.privateKey() == generatorB.privateKey());
}


//  BLOCK TWO: EDGE CASES AND INVARIANT CORNER TRAPS
TEST_CASE("JobCryptoKeys handling of invalid configurations and corrupted parameters", "[job_crypto][keys][edge]")
{
    JobCryptoKeys identity;

    SECTION("Initial uninitialized instance state tracking variables")
    {
        REQUIRE_FALSE(identity.isValid());
        REQUIRE(identity.publicKey().empty());
        REQUIRE(identity.privateKey().empty());
    }

    SECTION("Enforcing dimension validation constraints on seed material")
    {
        JobSecureMem tinySeed(8); // Invalid payload footprint size
        std::memset(tinySeed.data(), 0x11, tinySeed.size());

        REQUIRE_FALSE(identity.createSeedKeys(JobCryptoKeys::KeyType::Sign, tinySeed));
        REQUIRE_FALSE(identity.isValid());
    }

    SECTION("Graceful handling of truncated or corrupted public keys during handshakes")
    {
        REQUIRE(identity.createKeys(JobCryptoKeys::KeyType::Exchange));

        JobSecureMem rx;
        JobSecureMem tx;
        std::string const malformedPubKey = "GarbageBase64DataStringTruncated!!!";

        // Processing truncated keys must return failure safely rather than crashing
        REQUIRE_FALSE(identity.createClientSessionKeys(rx, tx, malformedPubKey));
        REQUIRE_FALSE(identity.createServerSessionKeys(rx, tx, malformedPubKey));
    }

    SECTION("Dynamic path management de-duplication testing")
    {
        std::filesystem::path const targetPath = std::filesystem::current_path() / "test_keystore";
        std::size_t const initialSize = identity.keyDirectories().size();

        identity.addKeyDirectory(targetPath);
        identity.addKeyDirectory(targetPath); // Duplicate execution injection pass

        // Enforce that duplicate insertion requests are discarded
        REQUIRE(identity.keyDirectories().size() == initialSize + 1);
    }
}


#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("JobCryptoKeys Engine Performance Characterization Profiles", "[job_crypto][keys][benchmark]")
{
    JobCryptoKeys engineNode;
    // no discard
    REQUIRE(engineNode.createKeys(JobCryptoKeys::KeyType::Exchange));

    // no discard add require my friend
    JobCryptoKeys peerNode;
    REQUIRE(peerNode.createKeys(JobCryptoKeys::KeyType::Exchange));
    std::string const peerPubStr = peerNode.publicKey();

    JobSecureMem sharedRx;
    JobSecureMem sharedTx;

    BENCHMARK("Asymmetric Ed25519 Signature Keypair Generation Loop") {
        JobCryptoKeys transNode;
        return transNode.createKeys(JobCryptoKeys::KeyType::Sign);
    };

    BENCHMARK("Asymmetric X25519 Exchange Keypair Generation Loop") {
        JobCryptoKeys transNode;
        return transNode.createKeys(JobCryptoKeys::KeyType::Exchange);
    };

    BENCHMARK("Diffie-Hellman Session Key Derivation Throughput Step") {
        return engineNode.createClientSessionKeys(sharedRx, sharedTx, peerPubStr);
    };
}
#endif