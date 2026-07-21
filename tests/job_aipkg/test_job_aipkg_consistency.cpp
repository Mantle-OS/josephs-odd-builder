#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include <vector>

#include <job_aipkg_consistency.h>
#include <job_aipkg_merkle.h>

using namespace job::aipkg;

TEST_CASE("RFC 6962 Consistency Proof Verification (BLAKE2b)", "[aipkg][consistency][proof]")
{
    // ------------------------------------------------------------------------
    // SETUP DECK: Building the historical timeline trees
    // ------------------------------------------------------------------------
    std::vector<std::vector<uint8_t>> rawLeaves = {
        {'L', '1'}, {'L', '2'}, {'L', '3'}, {'L', '4'}
    };

    std::vector<Hash32> leafHashes;
    for (const auto& leaf : rawLeaves)
        leafHashes.push_back(JobAiPkgMerkle::leafHash(leaf));


    std::vector<Hash32> roots(5);
    roots[1] = JobAiPkgMerkle::computeRoot({leafHashes[0]});
    roots[2] = JobAiPkgMerkle::computeRoot({leafHashes[0], leafHashes[1]});
    roots[3] = JobAiPkgMerkle::computeRoot({leafHashes[0], leafHashes[1], leafHashes[2]});
    roots[4] = JobAiPkgMerkle::computeRoot({leafHashes[0], leafHashes[1], leafHashes[2], leafHashes[3]});

    Hash32 h1  = leafHashes[0];
    Hash32 h2  = leafHashes[1];
    Hash32 h3  = leafHashes[2];
    Hash32 h4  = leafHashes[3];
    Hash32 h12 = JobAiPkgMerkle::nodeHash(h1, h2);
    Hash32 h34 = JobAiPkgMerkle::nodeHash(h3, h4);


    SECTION("Block 1: Canonical usage and append-only progression verification")
    {
        // Example 1: The standard tree update cycle. Size 1 -> Size 2.
        // Highway to the danger zone Goose... if the server can't hand you h2,
        // your history has been compromised or rewritten.
        SECTION("Verifying a simple sequential tree extension")
        {
            std::vector<Hash32> proofSize1To2 = {h2};
            bool const isValid = JobAiPkgConsistency::verify(1, roots[1], 2, roots[2], proofSize1To2);

            REQUIRE(isValid == true);
        }

        // Example 2: Verifying a multi-level leap across power-of-two boundaries.
        SECTION("Verifying a non-sequential multi-level tree leap")
        {
            std::vector<Hash32> proofSize1To4 = {h2, h34};
            bool const isValid = JobAiPkgConsistency::verify(1, roots[1], 4, roots[4], proofSize1To4);

            REQUIRE(isValid == true);
        }

        // Example 3: Handling left-heavy, un-balanced extensions.
        // Who's the boss? Tony Danza. Who's the left child? h12.
        SECTION("Verifying progression where the older tree spans into the right branch")
        {
            std::vector<Hash32> proofSize3To4 = {h3, h4, h12};
            bool const isValid = JobAiPkgConsistency::verify(3, roots[3], 4, roots[4], proofSize3To4);

            REQUIRE(isValid == true);
        }
    }


    // Stressing the boundaries: invalid sizes, mismatched histories, and raw noise.
    SECTION("Block 2: Hardening the boundaries against adversarial proofs")
    {
        SECTION("Trivial state matching where size does not change")
        {
            // Late to the party? If oldSize == newSize, the proof MUST be completely empty.
            std::vector<Hash32> emptyProof;
            REQUIRE(JobAiPkgConsistency::verify(3, roots[3], 3, roots[3], emptyProof) == true);

            // Feeding it a trailing artifact when nothing changed must be flatly rejected.
            REQUIRE(JobAiPkgConsistency::verify(3, roots[3], 3, roots[3], {h1}) == false);
        }

        SECTION("Violating spatial layout constraints")
        {
            // Reversing the dimensional arrow of time (old size > new size)
            REQUIRE(JobAiPkgConsistency::verify(3, roots[3], 2, roots[2], {h1}) == false);

            // Size 0 is historically blank; you cannot prove consistency with a ghost
            REQUIRE(JobAiPkgConsistency::verify(0, Hash32{}, 2, roots[2], {h1}) == false);
        }

        SECTION("Mismatched proof payload length mutations")
        {
            // Starving the verification path loop (Proof too short)
            REQUIRE(JobAiPkgConsistency::verify(2, roots[2], 4, roots[4], {}) == false);

            // Over-allocating the proof array (Leftover proof junk)
            std::vector<Hash32> bloatedProof = {h3, h4, h12, h1};
            REQUIRE(JobAiPkgConsistency::verify(3, roots[3], 4, roots[4], bloatedProof) == false);
        }

        SECTION("Providing a valid proof structure but target root mismatches")
        {
            std::vector<Hash32> proofSize2To3 = {h3};
            // Correct structural path, but comparing against a corrupted old root baseline
            REQUIRE(JobAiPkgConsistency::verify(2, roots[1], 3, roots[3], proofSize2To3) == false);
        }
    }

// BLOCK THREE: Performance Benchmarks / Stability Stress
#ifdef JOB_TEST_BENCHMARKS
    SECTION("Block 3: Verification performance characteristics")
    {
        // Push it to the limit. Evaluate hot-path execution costs for deep leaps.
        BENCHMARK("Verify complex 3 to 4 node consistency leap") {
            return JobAiPkgConsistency::verify(3, roots[3], 4, roots[4], {h3, h4, h12});
        };
    }
#endif
}