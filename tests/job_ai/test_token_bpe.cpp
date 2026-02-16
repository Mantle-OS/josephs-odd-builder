#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif


#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include <span>
#include <algorithm>

#include "bpe_token.h"
#include "byte_lattice.h"

using job::ai::token::BPEToken;
using job::ai::token::ByteLattice;

static std::vector<uint8_t> bytesOf(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static std::string stringOf(const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

static std::vector<ByteLattice> encodeAll(BPEToken& tok, const std::vector<uint8_t>& in) {
    std::vector<ByteLattice> out(in.size() * 4 + 16); // headroom
    const auto n = tok.encode(in, out, /*mass*/1.0f);
    out.resize(n);
    return out;
}

static std::vector<uint8_t> decodeAll(BPEToken& tok, std::span<const ByteLattice> in) {
    std::vector<uint8_t> out(in.size() * 16 + 64); // headroom
    const auto n = tok.decode(in, out);
    out.resize(n);
    return out;
}


TEST_CASE("BPEToken can train, encode, and decode a realistic mini-scenario", "[token][bpe][usage]")
{
    BPEToken tok;
    const std::string corpus =
        "the theater is there\n"
        "the theme is the thing\n"
        "there there there\n";

    tok.train(corpus, /*targetVocabSize*/ 400);

    const std::string msg = "the theater is there";
    auto in = bytesOf(msg);

    auto encoded = encodeAll(tok, in);
    REQUIRE_FALSE(encoded.empty());

    auto decoded = decodeAll(tok, encoded);
    REQUIRE(stringOf(decoded) == msg);
}

TEST_CASE("BPEToken save/load preserves behavior on the same input", "[token][bpe][io]")
{
    BPEToken tok;
    tok.train("abababababababab", 350);

    auto in = bytesOf("abababababab");

    auto enc1 = encodeAll(tok, in);
    auto dec1 = decodeAll(tok, enc1);
    REQUIRE(stringOf(dec1) == "abababababab");

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    REQUIRE(tok.save(ss));

    BPEToken tok2;
    ss.seekg(0);
    REQUIRE(tok2.load(ss));

    auto enc2 = encodeAll(tok2, in);
    auto dec2 = decodeAll(tok2, enc2);

    REQUIRE(stringOf(dec2) == "abababababab");

    // Stronger: encoding output identical (this is part correctness, part determinism)
    REQUIRE(enc2.size() == enc1.size());
    for (size_t i = 0; i < enc1.size(); ++i) {
        REQUIRE(enc2[i].x == enc1[i].x);
        REQUIRE(enc2[i].y == enc1[i].y);
        REQUIRE(enc2[i].z == enc1[i].z);
        REQUIRE(enc2[i].mass == enc1[i].mass);
    }
}

//
// Block 2: edge cases
//

TEST_CASE("BPEToken encode handles empty input and/or empty output", "[token][bpe][edge]")
{
    BPEToken tok;
    tok.train("hello hello hello", 300);

    std::vector<uint8_t> emptyIn;
    std::vector<ByteLattice> out(16);

    REQUIRE(tok.encode(emptyIn, out, 1.0f) == 0);

    std::vector<uint8_t> in = bytesOf("hello");
    std::vector<ByteLattice> emptyOut;
    REQUIRE(tok.encode(in, emptyOut, 1.0f) == 0);
}

TEST_CASE("BPEToken decode handles empty input and/or empty output", "[token][bpe][edge]")
{
    BPEToken tok;
    tok.train("hello hello hello", 300);

    std::vector<ByteLattice> emptyIn;
    std::vector<uint8_t> out(16);

    REQUIRE(tok.decode(emptyIn, out) == 0);

    auto in = encodeAll(tok, bytesOf("hello"));
    std::vector<uint8_t> emptyOut;
    REQUIRE(tok.decode(in, emptyOut) == 0);
}

TEST_CASE("BPEToken respects output span size on encode and decode", "[token][bpe][edge][bounds]")
{
    BPEToken tok;
    tok.train("abababababababababab", 400);

    auto in = bytesOf("abababababababab");

    // encode truncation
    std::vector<ByteLattice> outLat(2);
    const auto nTok = tok.encode(in, outLat, 1.0f);
    REQUIRE(nTok <= outLat.size());

    // decode truncation
    std::vector<uint8_t> outBytes(3);
    const auto nOut = tok.decode(std::span<const ByteLattice>(outLat.data(), nTok), outBytes);
    REQUIRE(nOut <= outBytes.size());
}

TEST_CASE("BPEToken training with targetVocabSize <= base vocab produces no merges", "[token][bpe][edge][train]")
{
    BPEToken tok;
    tok.train("anything at all", BPEToken::kBaseVocab);

    auto in = bytesOf("hello");
    auto enc = encodeAll(tok, in);

    // With no merges, expect 1:1 mapping
    REQUIRE(enc.size() == in.size());

    auto dec = decodeAll(tok, enc);
    REQUIRE(stringOf(dec) == "hello");
}

TEST_CASE("BPEToken lattice id packing roundtrips for representative ids", "[token][bpe][edge][lattice]")
{
    // This test ensures your (x,y,z) 7-bit packing stays stable.
    // We test it indirectly: encode raw byte stream with no merges should be stable.
    BPEToken tok; // no training => no merges => ids are 0..255

    std::vector<uint8_t> in;
    for (int i = 0; i < 256; ++i)
        in.push_back((uint8_t)i);

    auto enc = encodeAll(tok, in);
    REQUIRE(enc.size() == in.size());

    auto dec = decodeAll(tok, enc);
    REQUIRE(dec.size() == in.size());
    REQUIRE(dec == in);
}

TEST_CASE("BPEToken encoding is deterministic given the same training corpus and vocab size", "[token][bpe][edge][determinism]")
{
    const std::string corpus =
        "the theater is there\n"
        "the theme is the thing\n"
        "there there there\n";

    BPEToken a;
    BPEToken b;
    a.train(corpus, 450);
    b.train(corpus, 450);

    auto in = bytesOf("the theater is there");
    auto encA = encodeAll(a, in);
    auto encB = encodeAll(b, in);

    REQUIRE(encA.size() == encB.size());
    for (size_t i = 0; i < encA.size(); ++i) {
        REQUIRE(encA[i].x == encB[i].x);
        REQUIRE(encA[i].y == encB[i].y);
        REQUIRE(encA[i].z == encB[i].z);
        REQUIRE(encA[i].mass == encB[i].mass);
    }

    auto decA = decodeAll(a, encA);
    auto decB = decodeAll(b, encB);
    REQUIRE(decA == decB);
    REQUIRE(stringOf(decA) == "the theater is there");
}

TEST_CASE("BPEToken encode handles repeated stale PQ entries without corrupting output", "[token][bpe][edge][pq]")
{
    // This is tailored to catch the common 'stale queue element merges wrong pair' bug.
    // A corpus that encourages merges that create lots of overlapping candidates.
    BPEToken tok;
    tok.train("aaaaabaaaaabaaaaabaaaaab", 600);

    const std::string msg = "aaaaabaaaaabaaaaab";
    auto in = bytesOf(msg);

    auto enc = encodeAll(tok, in);
    REQUIRE_FALSE(enc.empty());

    auto dec = decodeAll(tok, enc);
    REQUIRE(stringOf(dec) == msg);
}

//
// Block 3: benchmarks / stress (optional)
//

#ifdef JOB_TEST_BENCHMARKS
TEST_CASE("BPEToken benchmarks", "[token][bpe][benchmark]")
{
    BPEToken tok;

    // Build a moderately large training corpus
    std::string corpus;
    corpus.reserve(200000);
    for (int i = 0; i < 50000; ++i)
        corpus += "the theater is there ";

    tok.train(corpus, 2000);

    // Parameter sweep
    for (int n : {64, 256, 1024, 4096, 16384, 65536}) {
        std::string msg;
        msg.reserve(n);
        while ((int)msg.size() < n)
            msg += "the theater is there ";

        std::vector<uint8_t> in(msg.begin(), msg.end());

        // Buffers reused per-iteration (so benchmark measures encode/decode, not allocations)
        std::vector<ByteLattice> lat(in.size() * 2);
        std::vector<uint8_t> out(in.size() * 2);

        // BENCHMARK("bpe encode N=" + std::to_string(n)) {
        //     const auto ntok = tok.encode(in, lat, 1.0f);
        //     return ntok;
        // };

        BENCHMARK("bpe encode+decode N=" + std::to_string(n)) {
            const auto ntok = tok.encode(in, lat, 1.0f);
            const auto nout = tok.decode(std::span<const ByteLattice>(lat.data(), ntok), out);
            return nout;
        };
    }
}
#endif


