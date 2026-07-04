#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <vector>
#include <string>

#include <sodium/crypto_secretbox.h>
#include <job_random.h>
#include <job_secret_box.h>

#include <job_zstd_wire.h>
#include <job_zstd_encrypting_transport.h>

namespace job::zstd {

namespace {

[[nodiscard]] job::crypto::JobSecureMem makeTestKey()
{
    job::crypto::JobSecureMem key(crypto_secretbox_KEYBYTES);
    job::crypto::JobRandom::secureBytes(key.data(), key.size());
    return key;
}

// Manually parses what JobZstdEncryptingTransport wrote, independent of
// JobZstdDecryptingTransport -- this file's job is proving the ENCRYPTING
// side is correct on its own, not round-tripping through its own mirror
// and hoping two bugs don't cancel out.
struct ParsedChunk
{
    std::vector<unsigned char> nonce;
    std::vector<unsigned char> cipher;
};

[[nodiscard]] bool parseAllChunks(const std::string &wireBytes, std::vector<ParsedChunk> &out)
{
    std::istringstream in(wireBytes);

    while (in.peek() != std::istringstream::traits_type::eof()) {
        std::string nonceStr;
        std::string cipherStr;

        if (!job::zstd::utils::readString(in, nonceStr) || !job::zstd::utils::readString(in, cipherStr))
            return false;

        ParsedChunk chunk;
        chunk.nonce.assign(nonceStr.begin(), nonceStr.end());
        chunk.cipher.assign(cipherStr.begin(), cipherStr.end());
        out.push_back(std::move(chunk));
    }

    return true;
}

[[nodiscard]] std::string decryptChunk(const ParsedChunk &chunk, const job::crypto::JobSecureMem &key)
{
    job::crypto::JobSecureMem plain;
    if (!job::crypto::JobSecretBox::decrypt(chunk.cipher, key, chunk.nonce, plain))
        return {};

    return std::string(reinterpret_cast<const char *>(plain.data()), plain.size());
}

} // namespace

TEST_CASE("JobZstdEncryptingTransport encrypts a small payload into one recoverable chunk", "[job_zstd][cryptotransport][encrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload = "The gate is open, the guard is asleep.";

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, 65536);

    std::ostream out(&transport);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(transport.finish());

    std::vector<ParsedChunk> chunks;
    REQUIRE(parseAllChunks(downstream.str(), chunks));
    REQUIRE(chunks.size() == 1);

    REQUIRE(decryptChunk(chunks[0], key) == payload);
}

TEST_CASE("JobZstdEncryptingTransport splits a payload larger than one chunk into multiple chunks", "[job_zstd][cryptotransport][encrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    constexpr std::size_t kChunkSize = 1024;
    std::string const payload(kChunkSize * 3 + 100, 'x'); // 3 full chunks + a partial final one

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, kChunkSize);

    std::ostream out(&transport);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(transport.finish());

    std::vector<ParsedChunk> chunks;
    REQUIRE(parseAllChunks(downstream.str(), chunks));
    REQUIRE(chunks.size() == 4); // 3 full + 1 partial tail

    std::string reassembled;
    for (const auto &chunk : chunks)
        reassembled += decryptChunk(chunk, key);

    REQUIRE(reassembled == payload);
}

TEST_CASE("JobZstdEncryptingTransport produces no chunk at all for an empty payload", "[job_zstd][cryptotransport][encrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, 65536);

    REQUIRE(transport.finish()); // Nothing was ever written -> finish() should be a harmless no-op.
    REQUIRE(downstream.str().empty());
}

TEST_CASE("JobZstdEncryptingTransport produces a payload exactly one chunk in size as a single chunk with no empty tail", "[job_zstd][cryptotransport][encrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    constexpr std::size_t kChunkSize = 256;
    std::string const payload(kChunkSize, 'A');

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, kChunkSize);

    std::ostream out(&transport);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(transport.finish()); // Buffer was flushed exactly at the boundary so finish() shouldn't invent an empty extra chunk.

    std::vector<ParsedChunk> chunks;
    REQUIRE(parseAllChunks(downstream.str(), chunks));
    REQUIRE(chunks.size() == 1);
    REQUIRE(decryptChunk(chunks[0], key) == payload);
}


// 2
TEST_CASE("JobZstdEncryptingTransport reports hadEncodeError on an invalid key size", "[job_zstd][cryptotransport][encrypt][edge]")
{
    job::crypto::JobSecureMem const badKey(4); // Not crypto_secretbox_KEYBYTES.

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, badKey, 65536);

    std::ostream out(&transport);
    out.write("some data", 9);

    REQUIRE_FALSE(transport.finish());
    REQUIRE(transport.hadEncodeError());
    REQUIRE_FALSE(transport.errorString().empty());
}

TEST_CASE("JobZstdEncryptingTransport refuses further writes once encoding has failed", "[job_zstd][cryptotransport][encrypt][edge]")
{
    job::crypto::JobSecureMem const badKey(4);
    constexpr std::size_t kChunkSize = 8;

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, badKey, kChunkSize);

    std::ostream out(&transport);
    out.write("exactly8", 8); // Fills and attempts to flush the first chunk, should fail immediately.

    REQUIRE(transport.hadEncodeError());

    std::streamsize const written = transport.sputn("more", 4);
    REQUIRE(written == 0); // Refused outright, not partially accepted.
}

TEST_CASE("JobZstdEncryptingTransport two consecutive chunks use different nonces", "[job_zstd][cryptotransport][encrypt][edge][security]")
{
    // Nonce reuse under the same key is a real cryptographic failure mode
    // for secretbox, This is the one property worth pinning down explicitly rather than trusting JobRandom implicitly.
    job::crypto::JobSecureMem const key = makeTestKey();
    constexpr std::size_t kChunkSize = 16;
    std::string const payload(kChunkSize * 2, 'z');

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, kChunkSize);

    std::ostream out(&transport);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(transport.finish());

    std::vector<ParsedChunk> chunks;
    REQUIRE(parseAllChunks(downstream.str(), chunks));
    REQUIRE(chunks.size() == 2);
    REQUIRE(chunks[0].nonce != chunks[1].nonce);
}

TEST_CASE("JobZstdEncryptingTransport rejects a zero chunk size at construction", "[job_zstd][cryptotransport][encrypt][edge]")
{
    job::crypto::JobSecureMem const key = makeTestKey();

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, 0);

    REQUIRE(transport.hadEncodeError());
    REQUIRE_FALSE(transport.errorString().empty());

    // And it must not hang -- this is the whole point of the fix.
    std::ostream out(&transport);
    out.write("data", 4);
    REQUIRE(transport.hadEncodeError());
}

TEST_CASE("JobZstdEncryptingTransport finish is idempotent and repeats the same outcome", "[job_zstd][cryptotransport][encrypt][edge]")
{
    job::crypto::JobSecureMem const key = makeTestKey();

    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, 65536);

    std::ostream out(&transport);
    out.write("hello", 5);

    REQUIRE(transport.finish());
    REQUIRE(transport.finish()); // Second call: no re-flush, just reports the same success.

    std::string const firstResult = downstream.str();
    REQUIRE(transport.finish());
    REQUIRE(downstream.str() == firstResult); // Nothing got written a second time.
}

} // namespace job::zstd