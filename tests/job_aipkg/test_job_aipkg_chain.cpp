#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <array>
#include <algorithm>

#include <job_aipkg_chain.h>
#include <job_aipkg_trust_state.h>
#include <job_aipkg_merkle.h>

using namespace job::aipkg;

// Helper to instantiate a minimal valid mock block layout
[[nodiscard]] static AiPkgBlock createMockBlock(uint64_t height, const Hash32 &prev, uint64_t treeSize, const Hash32 &treeRoot) noexcept
{
    AiPkgBlock block{};
    block.height = height;
    std::copy(prev.begin(), prev.end(), block.prev.begin());
    block.sth.size = treeSize;
    std::copy(treeRoot.begin(), treeRoot.end(), block.sth.root.begin());
    return block;
}

TEST_CASE("AiPkg Chain Linkage and Trust State Transitions", "[aipkg][chain][trust_state]")
{
    // SETUP DECK: Seed a few baseline tree structures
    std::vector<Hash32> leaves = {
        JobAiPkgMerkle::leafHash({'1'}),
        JobAiPkgMerkle::leafHash({'2'}),
        JobAiPkgMerkle::leafHash({'3'})
    };

    Hash32 const rootSize1 = JobAiPkgMerkle::computeRoot({leaves[0]});
    Hash32 const rootSize2 = JobAiPkgMerkle::computeRoot({leaves[0], leaves[1]});
    Hash32 const rootSize3 = JobAiPkgMerkle::computeRoot({leaves[0], leaves[1], leaves[2]});

    Hash32 const zeroPrev{};

    // Assemble an append-only timeline sequence
    AiPkgBlock b0 = createMockBlock(0, zeroPrev, 1, rootSize1);
    b0.sig[0] = 0xAA; // Give it a distinctive mock signature imprint

    Hash32 const h0 = JobAiPkgChain::hashFullBlock(b0);
    AiPkgBlock b1 = createMockBlock(1, h0, 2, rootSize2);
    b1.sig[0] = 0xBB;

    Hash32 const h1 = JobAiPkgChain::hashFullBlock(b1);
    AiPkgBlock b2 = createMockBlock(2, h1, 3, rootSize3);
    b2.sig[0] = 0xCC;


    ///////////////////////////////////////////////////////////////////


    SECTION("Block 1: Canonical usage of chain linkage and state advancing")
    {
        SECTION("Verifying linear cryptographic continuity across contiguous blocks")
        {
            // Verify that block 1 correctly anchors to full hash of block 0 (signature included!)
            REQUIRE(JobAiPkgChain::verifyLinkage(b1, b0) == true);
            REQUIRE(JobAiPkgChain::verifyLinkage(b2, b1) == true);
        }

        SECTION("Bootstrapping and managing state machine transitions via trust state engine")
        {
            JobAiPkgTrustState state;
            REQUIRE_FALSE(state.hasTrustedState());

            // Bootstrap tracking from the genesis block state snapshot
            REQUIRE(state.bootstrap(b0.sth) == true);
            REQUIRE(state.hasTrustedState());

            // Advance state tracking from Size 1 -> Size 2. The missing element is leaf 2 (leaves[1])
            std::vector<Hash32> const proof1To2 = {leaves[1]};
            REQUIRE(state.tryAdvance(b1.sth, proof1To2) == true);
            REQUIRE(state.trustedSTH().size == 2);
        }
    }

    // 2
    SECTION("Block 2: Hardening the invariants against adversarial mutations and timeline faults")
    {
        SECTION("Tampering with a signature must break downstream chain verification")
        {
            AiPkgBlock corruptedB0 = b0;
            // Who's the boss? The full hash. Changing the signature flips the link outcome completely.
            corruptedB0.sig[0] = 0x99;

            REQUIRE_FALSE(JobAiPkgChain::verifyLinkage(b1, corruptedB0));
        }

        SECTION("Rejecting invalid timeline step skips and temporal folds")
        {
            // Height gap constraint check (0 -> 2 skips height 1 entirely)
            REQUIRE_FALSE(JobAiPkgChain::verifyLinkage(b2, b0));

            // Backward time travel / loop generation detection
            REQUIRE_FALSE(JobAiPkgChain::verifyLinkage(b0, b1));
        }

        SECTION("Enforcing one-time-only restrictions on state engine bootstrap")
        {
            JobAiPkgTrustState state;
            REQUIRE(state.bootstrap(b0.sth) == true);

            // Double-bootstrap attempt must fail cleanly without altering underlying tracking context
            REQUIRE_FALSE(state.bootstrap(b1.sth));
            REQUIRE(state.trustedSTH().size == 1);
        }

        SECTION("Rejecting state transitions backed by fraudulent consistency paths")
        {
            JobAiPkgTrustState state;
            REQUIRE(state.bootstrap(b0.sth) == true);

            // Hand it an empty proof array for a size jump. Late to the party? Reject flatly.
            std::vector<Hash32> const badProof = {};
            REQUIRE_FALSE(state.tryAdvance(b1.sth, badProof));
            REQUIRE(state.trustedSTH().size == 1); // Ensure state stays pristine
        }
    }

#ifdef JOB_TEST_BENCHMARKS
    SECTION("Block 3: Hot-path sequencing serialization costs")
    {
        // Push it to the limit. Measure structural cost of full msgpack payload processing.
        BENCHMARK("Hash complete block layout (Data + Signature)") {
            return JobAiPkgChain::hashFullBlock(b2);
        };

        BENCHMARK("Verify linear step linkage") {
            return JobAiPkgChain::verifyLinkage(b2, b1);
        };
    }
#endif
}