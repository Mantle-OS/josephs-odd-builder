#include "job_fifo_ctx.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <vector>
#include <string>
#include <string_view>
#include <sstream>

#include <job_stealing_ctx.h>

#include "token/byte_lattice.h"

#include "token/corpus_chemist.h"
#include "token/motif_token.h"

using namespace job::ai::token;

// =========================================================
// CorpusChemist
// =========================================================
TEST_CASE("CorpusChemist: Breaking Bad", "[token][motif][chemist]")
{
    // BLOCK ONE: Single Threaded Discovery (Fallback)
    SECTION("Finding needles in haystacks (Single Threaded)")
    {
        std::string haystack;
        // Deterministic noise generator
        const std::string alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

        for (int i = 0; i < 2000; ++i) {
            for(int k=0; k<6; ++k)
                haystack += alphabet[(i * 7 + k) % alphabet.size()];

            if (i % 3 == 0)
                haystack += "SECRET";
        }

        // nullptr -> Forces fallback path
        CorpusChemist chemist(haystack, nullptr);

        std::string result = chemist.findReactiveMolecule(6, 5000, 12345);
        REQUIRE(result == "SECRET");
    }

    SECTION("Handling Empty or Small Corpus (Single Threaded)")
    {
        std::string tiny = "abc";
        CorpusChemist chemist(tiny, nullptr);

        REQUIRE(chemist.findReactiveMolecule(4, 100, 1) == "");
        REQUIRE(chemist.findReactiveMolecule(0, 100, 1) == "");
    }

    // BLOCK TWO: Multi-Threaded Discovery (Monte Carlo)
    SECTION("The Hive Mind (Threaded)")
    {
        job::threads::JobStealerCtx ctx(4);

        {
            std::string tiny = "abc";
            CorpusChemist chemist(tiny, ctx.pool);
            REQUIRE(chemist.findReactiveMolecule(4, 100, 1) == "");
        }

        {
            std::string haystack;
            const std::string alphabet = "abcdefghijklmnopqrstuvwxyz";
            for (int i = 0; i < 10000; ++i) {
                for(int k = 0; k < 6; ++k)
                    haystack += alphabet[(i * 7 + k) % alphabet.size()];

                if (i % 3 == 0)
                    haystack += "WINNER";
            }

            CorpusChemist chemist(haystack, ctx.pool);
            std::string result = chemist.findReactiveMolecule(6, 15000, 999);
            REQUIRE(result == "WINNER");
        }
    }
}

// =========================================================
// Motif
// =========================================================

TEST_CASE("MotifToken: The Hive Mind", "[token][motif][impl]")
{
    MotifToken token;

    // BLOCK ONE: God Particles (Initialization)
    SECTION("God Particles exist from the start")
    {
        // "Job" is hardcoded in the constructor.
        // It should encode to a SINGLE motif ID, not 3 byte lattices.
        std::string inputStr = "Job";
        std::vector<uint8_t> input(inputStr.begin(), inputStr.end());
        std::vector<ByteLattice> output(10);

        size_t n = token.encode(input, output);

        REQUIRE(n == 1); // 1 atom

        // Check lane Z. If it's a motif, Z > 0.5. If it's a byte, Z = -1.0.
        REQUIRE(output[0].z > 0.5f);
        REQUIRE(output[0].mass == 1.0f);
    }

    SECTION("Greedy Matching Logic")
    {
        // "The " is a god particle.
        // "Job" is a god particle.
        // Input: "The Job" (7 bytes)
        // Expect: 2 atoms.

        std::string text = "The Job";
        std::vector<uint8_t> input(text.begin(), text.end());
        std::vector<ByteLattice> output(10);

        size_t n = token.encode(input, output);
        REQUIRE(n == 2);
    }

    SECTION("Fallback to Bytes")
    {
        // "Zorglub" is definitely not in the dictionary.
        std::string text = "Zorglub";
        std::vector<uint8_t> input(text.begin(), text.end());
        std::vector<ByteLattice> output(20);

        size_t n = token.encode(input, output);

        REQUIRE(n == 7); // 7 raw bytes
        REQUIRE(output[0].z == -1.0f);
        REQUIRE(ByteLattice::decode(output[0]) == 'Z');
    }

    // BLOCK TWO: Mutation & Learning
    SECTION("Evolution (Mutation)")
    {
        std::string corpus;
        for(int i=0; i<500; ++i) corpus += "TEST"; // 2000 bytes

        token.setCorpus(corpus, nullptr);
        bool learned = false;
        for(int i=0; i<50; ++i) {
            token.mutate(i * 999); // Different seeds

            // Check if "TEST" is now a motif
            // We can check by encoding "TEST" and seeing if it results in 1 atom
            std::string probe = "TEST";
            std::vector<uint8_t> pIn(probe.begin(), probe.end());
            std::vector<ByteLattice> pOut(10);
            if (token.encode(pIn, pOut) == 1) {
                learned = true;
                break;
            }
        }

        REQUIRE(learned);
    }

    // Serialization
    SECTION("Save and Load (Persistence)")
    {
        std::stringstream ss;
        REQUIRE(token.save(ss));

        MotifToken token2;

        REQUIRE(token2.load(ss));

        std::string text = "Job"; // God particle
        std::vector<uint8_t> input(text.begin(), text.end());
        std::vector<ByteLattice> output(10);

        size_t n = token2.encode(input, output);
        REQUIRE(n == 1);
    }

    SECTION("Decoding Motifs")
    {
        // Encode "Job" -> 1 Atom
        std::string text = "Job";
        std::vector<uint8_t> input(text.begin(), text.end());
        std::vector<ByteLattice> encoded(10);
        size_t nEnc = token.encode(input, encoded);

        // Decode 1 Atom -> "Job"
        std::vector<uint8_t> decoded(10);
        size_t nDec = token.decode(std::span(encoded.data(), nEnc), decoded);

        REQUIRE(nDec == 3);
        REQUIRE(decoded[0] == 'J');
        REQUIRE(decoded[1] == 'o');
        REQUIRE(decoded[2] == 'b');
    }

    // EDGE CASE: Squatter Removal
    SECTION("Squatter Removal (Decay)")
    {
        REQUIRE(token.getMotifCount() < MotifToken::kMaxCapacity);
    }
}




#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("MotifToken benchmarks", "[token][motif][benchmark]")
{
    // -----------------------
    // Helpers
    // -----------------------
    auto as_bytes = [](const std::string& s) {
        return std::vector<uint8_t>(s.begin(), s.end());
    };

    auto make_natural = [](size_t approxBytes) {
        std::string s;
        s.reserve(approxBytes);
        while (s.size() < approxBytes) {
            // includes likely motifs like "The " and "Job" (per your tests),
            // plus a bunch of normal text.
            s += "The Job was to do the job. The Job is the job. ";
            s += "The theater is there and the theme is the thing. ";
        }
        s.resize(approxBytes);
        return s;
    };

    auto make_near_miss = [](size_t approxBytes) {
        // Stresses greedy matcher: lots of prefixes that *almost* match motifs,
        // but differ at the end (forces deeper scans + fallback decisions).
        std::string s;
        s.reserve(approxBytes);

        const std::string base = "The JoX "; // looks like "The Jo" then fails
        while (s.size() < approxBytes) {
            s += base;
            s += "The JoY ";
            s += "The JoZ ";
            s += "The JoQ ";
            s += "The Job "; // include some actual hits too
        }
        s.resize(approxBytes);
        return s;
    };

    // -----------------------
    // Setup: token + buffers
    // -----------------------
    MotifToken tok;

    // Optionally prime learning so benchmarks include some learned motifs
    // (kept small so it doesn't dominate setup).
    {
        std::string corpus;
        corpus.reserve(4000);
        for (int i = 0; i < 1000; ++i) corpus += "TEST";
        tok.setCorpus(corpus, nullptr);
        for (int i = 0; i < 10; ++i)
            tok.mutate(i * 999);
    }

    for (int n : {64, 256, 1024, 4096, 16384, 65536}) {
        const std::string natural = make_natural((size_t)n);
        const std::string nearMiss = make_near_miss((size_t)n);

        auto inNat = as_bytes(natural);
        auto inMiss = as_bytes(nearMiss);

        std::vector<ByteLattice> outNat(inNat.size() + 64);   // motifs compress, but be safe
        std::vector<ByteLattice> outMiss(inMiss.size() + 64);
        std::vector<uint8_t> decoded(inNat.size() + 256);

        // BENCHMARK("motif encode natural N=" + std::to_string(n)) {
        //     const auto ntok = tok.encode(inNat, outNat);
        //     return ntok;
        // };

        // BENCHMARK("motif encode near-miss N=" + std::to_string(n)) {
        //     const auto ntok = tok.encode(inMiss, outMiss);
        //     return ntok;
        // };

        BENCHMARK("motif encode+decode natural N=" + std::to_string(n)) {
            const auto ntok = tok.encode(inNat, outNat);
            const auto nout = tok.decode(std::span<const ByteLattice>(outNat.data(), ntok), decoded);
            return nout;
        };

        // Non-timed sanity check to ensure no UB in the benchmark
        {
            const auto ntok = tok.encode(inNat, outNat);
            const auto nout = tok.decode(std::span<const ByteLattice>(outNat.data(), ntok), decoded);

            const std::string roundtrip(decoded.begin(), decoded.begin() + (std::ptrdiff_t)nout);
            REQUIRE(roundtrip == natural);
        }
    }

    // -----------------------
    // Benchmark: mutation/learning (how expensive is "get smarter")
    // -----------------------

    BENCHMARK("motif mutate x50 (corpus=TEST*500)") {
        job::threads::JobStealerCtx ctx(4);
        MotifToken t;
        std::string corpus;
        corpus.reserve(2000);
        for (int i = 0; i < 500; ++i)
            corpus += "TEST";

        t.setCorpus(corpus, ctx.pool);

        // Return motif count so work can't be optimized away
        for (int i = 0; i < 50; ++i)
            t.mutate(i * 999);
        return t.getMotifCount();
    };
}
#endif










