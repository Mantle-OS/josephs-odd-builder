#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <istream>
#include <ostream>
#include <string>

#include <sodium/crypto_secretbox.h>
#include <job_random.h>
#include <job_secret_box.h>

#include <job_zstd_encrypting_transport.h>
#include <job_zstd_wire.h>
#include <job_zstd_decrypting_transport.h>

#include "transient_test_corruption.h"

namespace job::zstd {

namespace {

[[nodiscard]] job::crypto::JobSecureMem makeTestKey()
{
    job::crypto::JobSecureMem key(crypto_secretbox_KEYBYTES);
    job::crypto::JobRandom::secureBytes(key.data(), key.size());
    return key;
}

// Builds a real encrypted wire stream using JobZstdEncryptingTransport itself.
// This file trusts that class's own tests to have proven IT correct, and focuses entirely on whether the DEcrypting side reads it back right (including when it's been damaged after the fact).
[[nodiscard]] std::string buildEncryptedStream(const std::string &payload, const job::crypto::JobSecureMem &key, std::size_t chunkSize = 4096)
{
    std::stringbuf downstream(std::ios::out);
    JobZstdEncryptingTransport transport(&downstream, key, chunkSize);

    std::ostream out(&transport);
    out.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    REQUIRE(transport.finish());
    return downstream.str();
}

} // namespace

TEST_CASE("JobZstdDecryptingTransport round-trips a small payload", "[job_zstd][cryptotransport][decrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload = "The gate is open, the guard is asleep.";
    std::string const wireBytes = buildEncryptedStream(payload, key);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(result.str() == payload);
    REQUIRE(transport.atEnd());
    REQUIRE_FALSE(transport.wasTruncated());
    REQUIRE_FALSE(transport.hadAuthenticationError());
}

TEST_CASE("JobZstdDecryptingTransport round-trips a payload spanning many chunks", "[job_zstd][cryptotransport][decrypt][usage]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(4096 * 3 + 777, 'q');
    std::string const wireBytes = buildEncryptedStream(payload, key, 4096);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::string restored(payload.size(), '\0');
    std::istream in(&transport);
    in.read(restored.data(), static_cast<std::streamsize>(restored.size()));

    REQUIRE(static_cast<std::size_t>(in.gcount()) == payload.size());
    REQUIRE(restored == payload);
}

TEST_CASE("JobZstdDecryptingTransport handles reads that stop mid-chunk and resume later", "[job_zstd][cryptotransport][decrypt][usage]")
{
    // Exercises the "chunk wasn't fully consumed, stash the remainder" path in xsgetn() explicitly !
    // Reading in small, deliberately chunk-misaligned pieces rather than one big read.
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(4096 * 2, 'm');
    std::string const wireBytes = buildEncryptedStream(payload, key, 4096);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::string restored;
    char buf[777]; // Deliberately not a divisor of the 4096 chunk size.

    while (in.read(buf, sizeof(buf)) || in.gcount() > 0)
        restored.append(buf, static_cast<std::size_t>(in.gcount()));

    REQUIRE(restored == payload);
}

// 2
TEST_CASE("JobZstdDecryptingTransport round-trips an empty payload", "[job_zstd][cryptotransport][decrypt][edge]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const wireBytes = buildEncryptedStream("", key);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(result.str().empty());
    REQUIRE(transport.atEnd());
}

TEST_CASE("JobZstdDecryptingTransport reports wasTruncated when the stream ends mid-frame", "[job_zstd][cryptotransport][decrypt][edge][truncation]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(4096 * 2, 'x');
    std::string const wireBytes = buildEncryptedStream(payload, key, 4096);

    std::string const chopped = job::zstd::test::truncate(wireBytes, 0.6);

    std::stringbuf downstream(chopped, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(transport.wasTruncated());
    REQUIRE_FALSE(transport.hadAuthenticationError());
    REQUIRE_FALSE(transport.errorString().empty());
}

TEST_CASE("JobZstdDecryptingTransport reports hadAuthenticationError, not truncation, on tampered ciphertext", "[job_zstd][cryptotransport][decrypt][edge][security]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(500, 'y');
    std::string const wireBytes = buildEncryptedStream(payload, key);

    // Flip a bit well past the length-prefix headers, landing inside the
    // ciphertext itself rather than corrupting the framing.
    std::string const tampered = job::zstd::test::flipBit(wireBytes, wireBytes.size() - 10);

    std::stringbuf downstream(tampered, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(transport.hadAuthenticationError());
    REQUIRE_FALSE(transport.wasTruncated());
    REQUIRE_FALSE(transport.errorString().empty());
}

TEST_CASE("JobZstdDecryptingTransport reports hadAuthenticationError when decrypted with the wrong key", "[job_zstd][cryptotransport][decrypt][edge][security]")
{
    job::crypto::JobSecureMem const correctKey = makeTestKey();
    job::crypto::JobSecureMem const wrongKey = makeTestKey();
    std::string const payload = "confidential archive content";
    std::string const wireBytes = buildEncryptedStream(payload, correctKey);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, wrongKey);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(transport.hadAuthenticationError());
}

TEST_CASE("JobZstdDecryptingTransport atEnd is false before reading and false on an error state", "[job_zstd][cryptotransport][decrypt][edge][atend]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(200, 'z');
    std::string const wireBytes = buildEncryptedStream(payload, key);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    REQUIRE_FALSE(transport.atEnd());

    std::string restored(payload.size(), '\0');
    std::istream in(&transport);
    in.read(restored.data(), static_cast<std::streamsize>(restored.size()));

    REQUIRE(transport.atEnd());
}


TEST_CASE("JobZstdDecryptingTransport correctly skips over a legitimate empty chunk instead of stopping early", "[job_zstd][cryptotransport][decrypt][edge]")
{
    // JobZstdEncryptingTransport never emits an empty chunk itself (see its own tests)
    job::crypto::JobSecureMem const key = makeTestKey();
    std::ostringstream out;

    auto writeChunk = [&](const std::string &plaintext) {
        std::vector<unsigned char> const plain(plaintext.begin(), plaintext.end());
        std::vector<unsigned char> cipher;
        std::vector<unsigned char> nonce;
        REQUIRE(job::crypto::JobSecretBox::encrypt(plain, key, cipher, nonce));

        job::zstd::utils::writeString(out, std::string(nonce.begin(), nonce.end()));
        job::zstd::utils::writeString(out, std::string(cipher.begin(), cipher.end()));
    };

    writeChunk("first ");
    writeChunk("");       // The legitimate-but-empty chunk in question.
    writeChunk("second");

    std::stringbuf downstream(out.str(), std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(result.str() == "first second");
    REQUIRE(transport.atEnd());
    REQUIRE_FALSE(transport.wasTruncated());
    REQUIRE_FALSE(transport.hadAuthenticationError());
}

TEST_CASE("JobZstdDecryptingTransport treats a malformed nonce size as an authentication failure, not truncation", "[job_zstd][cryptotransport][decrypt][edge]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::ostringstream out;

    std::string const badNonce(10, '\0'); // Not crypto_secretbox_NONCEBYTES (24).
    std::string const someCipher(40, '\0');
    job::zstd::utils::writeString(out, badNonce);
    job::zstd::utils::writeString(out, someCipher);

    std::stringbuf downstream(out.str(), std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::istream in(&transport);
    std::ostringstream result;
    result << in.rdbuf();

    REQUIRE(transport.hadAuthenticationError());
    REQUIRE_FALSE(transport.wasTruncated());
}

TEST_CASE("JobZstdDecryptingTransport atEnd becomes true immediately after consuming exactly the last byte across multiple chunks", "[job_zstd][cryptotransport][decrypt][edge][atend]")
{
    job::crypto::JobSecureMem const key = makeTestKey();
    std::string const payload(4096 * 2, 'w'); // Exactly two full chunks, no partial tail.
    std::string const wireBytes = buildEncryptedStream(payload, key, 4096);

    std::stringbuf downstream(wireBytes, std::ios::in);
    JobZstdDecryptingTransport transport(&downstream, key);

    std::string restored(payload.size(), '\0');
    std::istream in(&transport);
    in.read(restored.data(), static_cast<std::streamsize>(restored.size()));

    REQUIRE(restored == payload);
    REQUIRE(transport.atEnd()); // No extra read attempt should be needed here either.
}
} // namespace job::zstd