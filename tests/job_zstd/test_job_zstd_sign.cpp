// test_job_zstd_sign.cpp
#include <catch2/catch_test_macros.hpp>

#include "job_zstd_sign.h"
#include "job_crypto_keys.h"
#include "transient_test_filesystem.h"

#include <fstream>
#include <sstream>
#include <string>

namespace job::zstd {

namespace {

struct KeyPairFiles
{
    std::filesystem::path pub;
    std::filesystem::path priv;
};

// Generates a real Ed25519 keypair via JobCryptoKeys and writes it to disk
// under custom filenames -- custom names matter here specifically because
// several tests need TWO independent keypairs coexisting in the same
// scratch directory (e.g. "correct" vs "wrong" key mismatch scenarios).
[[nodiscard]] KeyPairFiles generateKeyPairFiles(const std::filesystem::path &dir, const std::string &pubName, const std::string &privName)
{
    job::crypto::JobCryptoKeys keys;
    if (!keys.createKeys(job::crypto::JobCryptoKeys::KeyType::Sign))
        return {};

    if (!keys.saveKeys(dir, pubName, privName))
        return {};

    return {dir / pubName, dir / privName};
}

[[nodiscard]] std::string readFileText(const std::filesystem::path &path)
{
    std::ifstream file(path, std::ios::binary);
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

} // namespace

// ---------------------------------------------------------------------------
// Block one: usage / examples
// ---------------------------------------------------------------------------

TEST_CASE("JobZstdSign signs a file and verifies it with the matching keypair", "[job_zstd][sign][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");
    REQUIRE_FALSE(keys.pub.empty());

    std::filesystem::path const data = scratch.makeFile("archive.zst", "the compressed archive lives here");
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));
    REQUIRE(signer.hasSigningKeys());
    REQUIRE(signer.hasVerificationKey());

    REQUIRE(signer.signFile(data, sig));
    REQUIRE(std::filesystem::exists(sig));

    std::string const signatureBase64 = readFileText(sig);
    REQUIRE_FALSE(signatureBase64.empty());

    JobZstdSign verifier;
    REQUIRE(verifier.setPublicKeyFile(keys.pub));
    REQUIRE(verifier.verifyFile(data, signatureBase64));
}

TEST_CASE("JobZstdSign's signature does not verify against tampered data", "[job_zstd][sign][usage][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    std::filesystem::path const data = scratch.makeFile("archive.zst", "original content");
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));
    REQUIRE(signer.signFile(data, sig));

    std::string const signatureBase64 = readFileText(sig);

    // Tamper with the data AFTER signing -- the signature on disk still
    // reflects the original bytes.
    std::ofstream(data, std::ios::binary | std::ios::trunc) << "tampered content";

    JobZstdSign verifier;
    REQUIRE(verifier.setPublicKeyFile(keys.pub));
    REQUIRE_FALSE(verifier.verifyFile(data, signatureBase64));
}

TEST_CASE("JobZstdSign tracks progress across a full sign", "[job_zstd][sign][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    std::filesystem::path const data = scratch.makeFile("archive.zst", std::string(5000, 'x'));
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));
    REQUIRE(signer.signFile(data, sig));

    REQUIRE(signer.total() == 5000);
    REQUIRE(signer.current() == signer.total());
}

TEST_CASE("JobZstdSign overwrite=true replaces an existing signature file", "[job_zstd][sign][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    std::filesystem::path const data = scratch.makeFile("archive.zst", "content");
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));

    REQUIRE(signer.signFile(data, sig));
    std::string const firstSignature = readFileText(sig);

    REQUIRE(signer.signFile(data, sig, true));
    std::string const secondSignature = readFileText(sig);

    // Same data, same key -- Ed25519 is deterministic, so re-signing
    // identical input produces an identical signature. The point of this
    // test is that overwrite=true didn't REFUSE the second call, not that
    // the bytes differ.
    REQUIRE(secondSignature == firstSignature);
}

// ---------------------------------------------------------------------------
// Block two: edge cases
// ---------------------------------------------------------------------------

TEST_CASE("JobZstdSign setPublicKeyFile rejects a malformed key file", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const junk = scratch.makeFile("not_a_key.pub", "definitely not base64 Ed25519 key material");

    JobZstdSign signer;
    REQUIRE_FALSE(signer.setPublicKeyFile(junk));
    REQUIRE_FALSE(signer.hasVerificationKey());
}

TEST_CASE("JobZstdSign setPrivateKeyFile fails without a public key set first", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    JobZstdSign signer;
    REQUIRE_FALSE(signer.setPrivateKeyFile(keys.priv));
    REQUIRE_FALSE(signer.hasSigningKeys());
}

TEST_CASE("JobZstdSign setPrivateKeyFile refuses a private key that doesn't pair with the current public key", "[job_zstd][sign][edge][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keyPairA = generateKeyPairFiles(scratch.root(), "a.pub", "a.priv");
    KeyPairFiles const keyPairB = generateKeyPairFiles(scratch.root(), "b.pub", "b.priv");

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keyPairA.pub));

    // B's private key does not mathematically correspond to A's public key --
    // this is exactly the "internally well-formed but wrongly paired"
    // scenario privateKeyMatchesPublicKey() exists to catch.
    REQUIRE_FALSE(signer.setPrivateKeyFile(keyPairB.priv));
    REQUIRE_FALSE(signer.hasSigningKeys());

    // Verification-only capability must survive the rejected pairing attempt --
    // the public key itself was never in question.
    REQUIRE(signer.hasVerificationKey());
}

TEST_CASE("JobZstdSign re-reads a rotated public key file at the same path rather than trusting a stale cache", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keyPairA = generateKeyPairFiles(scratch.root(), "a.pub", "a.priv");
    KeyPairFiles const keyPairB = generateKeyPairFiles(scratch.root(), "b.pub", "b.priv");

    std::filesystem::path const rotatingPath = scratch.root() / "id.pub";
    std::filesystem::copy_file(keyPairA.pub, rotatingPath, std::filesystem::copy_options::overwrite_existing);

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(rotatingPath));
    REQUIRE(signer.setPrivateKeyFile(keyPairA.priv)); // Pairs fine with A.

    // Rotate the file IN PLACE -- same path, different content.
    std::filesystem::copy_file(keyPairB.pub, rotatingPath, std::filesystem::copy_options::overwrite_existing);
    REQUIRE(signer.setPublicKeyFile(rotatingPath)); // Same path as before -- must NOT short-circuit.

    // A's private key must no longer pair with what's now on file.
    REQUIRE_FALSE(signer.setPrivateKeyFile(keyPairA.priv));

    // B's private key must pair successfully with the rotated content.
    REQUIRE(signer.setPrivateKeyFile(keyPairB.priv));
    REQUIRE(signer.hasSigningKeys());
}

TEST_CASE("JobZstdSign signFile fails fast with no signing keys set", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const data = scratch.makeFile("archive.zst", "content");
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE_FALSE(signer.signFile(data, sig));
    REQUIRE_FALSE(signer.errorString().empty());
    REQUIRE_FALSE(std::filesystem::exists(sig));
}

TEST_CASE("JobZstdSign signFile fails when the input file does not exist", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));

    REQUIRE_FALSE(signer.signFile(scratch.root() / "nope.zst", scratch.root() / "nope.zst.sig"));
}

TEST_CASE("JobZstdSign signFile with overwrite=false refuses to replace an existing signature", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    std::filesystem::path const data = scratch.makeFile("archive.zst", "content");
    std::filesystem::path const sig = scratch.makeFile("archive.zst.sig", "pre-existing sentinel content");

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(keys.pub));
    REQUIRE(signer.setPrivateKeyFile(keys.priv));

    REQUIRE_FALSE(signer.signFile(data, sig, false));
    REQUIRE(readFileText(sig) == "pre-existing sentinel content"); // Untouched.
}

TEST_CASE("JobZstdSign verifyFile fails fast with no verification key set", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const data = scratch.makeFile("archive.zst", "content");

    JobZstdSign verifier;
    REQUIRE_FALSE(verifier.verifyFile(data, "irrelevant-signature-text"));
}

TEST_CASE("JobZstdSign verifyFile fails against the wrong public key", "[job_zstd][sign][edge][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const correctKeys = generateKeyPairFiles(scratch.root(), "correct.pub", "correct.priv");
    KeyPairFiles const wrongKeys = generateKeyPairFiles(scratch.root(), "wrong.pub", "wrong.priv");

    std::filesystem::path const data = scratch.makeFile("archive.zst", "content only the right key should authenticate");
    std::filesystem::path const sig = scratch.root() / "archive.zst.sig";

    JobZstdSign signer;
    REQUIRE(signer.setPublicKeyFile(correctKeys.pub));
    REQUIRE(signer.setPrivateKeyFile(correctKeys.priv));
    REQUIRE(signer.signFile(data, sig));

    std::string const signatureBase64 = readFileText(sig);

    JobZstdSign verifier;
    REQUIRE(verifier.setPublicKeyFile(wrongKeys.pub));
    REQUIRE_FALSE(verifier.verifyFile(data, signatureBase64));
}

TEST_CASE("JobZstdSign verifyFile handles garbage signature text without crashing", "[job_zstd][sign][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");
    std::filesystem::path const data = scratch.makeFile("archive.zst", "content");

    JobZstdSign verifier;
    REQUIRE(verifier.setPublicKeyFile(keys.pub));
    REQUIRE_FALSE(verifier.verifyFile(data, "not-a-real-signature-at-all"));
}

TEST_CASE("JobZstdSign hasSigningKeys and hasVerificationKey are independent of each other", "[job_zstd][sign][edge]")
{
    // A verify-only caller never sets a private key at all -- hasSigningKeys()
    // must reflect that honestly rather than being coupled to
    // hasVerificationKey() through some shared internal flag.
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");

    JobZstdSign verifier;
    REQUIRE(verifier.setPublicKeyFile(keys.pub));

    REQUIRE(verifier.hasVerificationKey());
    REQUIRE_FALSE(verifier.hasSigningKeys());
}

} // namespace job::zstd