// test_job_zstd.cpp
#include <catch2/catch_test_macros.hpp>

#include "job_zstd.h"
#include "job_crypto_keys.h"
#include "job_random.h"
#include "transient_test_filesystem.h"

#include <sodium/crypto_secretbox.h>
#include <chrono>
#include <memory>
#include <future>
#include <functional>
#include <fstream>
#include <sstream>

namespace job::zstd {

namespace {

[[nodiscard]] bool runAndWait(JobZstd &zstd, const std::function<void()> &trigger,
                              std::chrono::milliseconds timeout = std::chrono::seconds(10))
{
    auto donePromise = std::make_shared<std::promise<void>>();
    std::future<void> doneFuture = donePromise->get_future();

    zstd.setOnFinished([donePromise]() {
        donePromise->set_value();
    });

    trigger();

    return doneFuture.wait_for(timeout) == std::future_status::ready;
}

[[nodiscard]] job::crypto::JobSecureMem makeTestEncryptionKey()
{
    job::crypto::JobSecureMem key(crypto_secretbox_KEYBYTES);
    job::crypto::JobRandom::secureBytes(key.data(), key.size());
    return key;
}

struct KeyPairFiles
{
    std::filesystem::path pub;
    std::filesystem::path priv;
};

[[nodiscard]] KeyPairFiles generateKeyPairFiles(const std::filesystem::path &dir, const std::string &pubName, const std::string &privName)
{
    job::crypto::JobCryptoKeys keys;
    if (!keys.createKeys(job::crypto::JobCryptoKeys::KeyType::Sign))
        return {};

    if (!keys.saveKeys(dir, pubName, privName))
        return {};

    return {dir / pubName, dir / privName};
}

[[nodiscard]] std::string readFileContent(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

} // namespace


TEST_CASE("JobZstd compresses and decompresses a plain file asynchronously", "[job_zstd][async][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "async plain round-trip content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstd zstd;
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(); }));
    REQUIRE(zstd.errorString().empty());
    REQUIRE(std::filesystem::exists(archive));

    REQUIRE(zstd.setInput(archive.string()));
    REQUIRE(zstd.setOutput(restored.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.decompress(); }));
    REQUIRE(zstd.errorString().empty());

    REQUIRE(readFileContent(restored) == "async plain round-trip content");
}

TEST_CASE("JobZstd compresses with encryption and decompresses it back with the same key", "[job_zstd][async][usage][crypto]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "encrypted async content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    job::crypto::JobSecureMem const key = makeTestEncryptionKey();

    JobZstd zstd;
    zstd.setPrivateKey(key);
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(false, true); }));
    REQUIRE(zstd.errorString().empty());

    REQUIRE(zstd.setInput(archive.string()));
    REQUIRE(zstd.setOutput(restored.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.decompress(false, true); }));
    REQUIRE(zstd.errorString().empty());

    REQUIRE(readFileContent(restored) == "encrypted async content");
}

TEST_CASE("JobZstd compresses with a signature and verifies it on decompress", "[job_zstd][async][usage][sign]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");
    REQUIRE_FALSE(keys.pub.empty());

    std::filesystem::path const src = scratch.makeFile("payload.txt", "signed async content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstd zstd;
    REQUIRE(zstd.setPublicKeyFile(keys.pub));
    REQUIRE(zstd.setPrivateKeyFile(keys.priv));
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(true, false); }));
    REQUIRE(zstd.errorString().empty());
    REQUIRE(std::filesystem::exists(archive));
    REQUIRE(std::filesystem::exists(scratch.root() / "payload.zst.sig"));

    REQUIRE(zstd.setInput(archive.string()));
    REQUIRE(zstd.setOutput(restored.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.decompress(true, false); }));
    REQUIRE(zstd.errorString().empty());

    REQUIRE(readFileContent(restored) == "signed async content");
}

TEST_CASE("JobZstd combines signing and encryption in a single pipeline", "[job_zstd][async][usage][sign][crypto]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const keys = generateKeyPairFiles(scratch.root(), "id.pub", "id.priv");
    job::crypto::JobSecureMem const encKey = makeTestEncryptionKey();

    std::filesystem::path const src = scratch.makeFile("payload.txt", "signed AND encrypted content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstd zstd;
    REQUIRE(zstd.setPublicKeyFile(keys.pub));
    REQUIRE(zstd.setPrivateKeyFile(keys.priv));
    zstd.setPrivateKey(encKey);
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(true, true); }));
    REQUIRE(zstd.errorString().empty());
    REQUIRE(std::filesystem::exists(archive));
    REQUIRE(std::filesystem::exists(scratch.root() / "payload.zst.enc.sig"));

    // No leftover .tmp file -- the sign step's rename-into-place must have
    // fully replaced it, not left a stray intermediate artifact behind.
    REQUIRE_FALSE(std::filesystem::exists(scratch.root() / "payload.zst.enc.tmp"));

    REQUIRE(zstd.setInput(archive.string()));
    REQUIRE(zstd.setOutput(restored.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.decompress(true, true); }));
    REQUIRE(zstd.errorString().empty());

    REQUIRE(readFileContent(restored) == "signed AND encrypted content");
}

TEST_CASE("JobZstd getSignKey/setSignKey round-trip independently of the encryption key", "[job_zstd][usage]")
{
    // m_signKey and m_privateKey are two completely independent slots --
    // one Ed25519-shaped, one secretbox-shaped -- even though nothing in
    // this pass wires m_signKey into an actual pipeline yet.
    JobZstd zstd;

    job::crypto::JobSecureMem signKey(64);
    job::crypto::JobRandom::secureBytes(signKey.data(), signKey.size());

    zstd.setSignKey(signKey);
    REQUIRE(zstd.getSignKey() == signKey);

    zstd.setPrivateKey(makeTestEncryptionKey());
    REQUIRE(zstd.getSignKey() == signKey); // Unaffected by setting the other slot.
}

TEST_CASE("JobZstd getPrivateKey/setPrivateKey round-trip independently of the sign key", "[job_zstd][usage]")
{
    JobZstd zstd;
    job::crypto::JobSecureMem const encKey = makeTestEncryptionKey();

    zstd.setPrivateKey(encKey);
    REQUIRE(zstd.getPrivateKey() == encKey);

    job::crypto::JobSecureMem signKey(64);
    job::crypto::JobRandom::secureBytes(signKey.data(), signKey.size());
    zstd.setSignKey(signKey);
    REQUIRE(zstd.getPrivateKey() == encKey); // Unaffected by setting the other slot.
}


// 2
TEST_CASE("JobZstd plain compress fails with a clear error when input/output are unconfigured", "[job_zstd][async][edge]")
{
    JobZstd zstd;
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(); }));
    REQUIRE_FALSE(zstd.errorString().empty());
}

TEST_CASE("JobZstd encrypted compress fails with a clear error when no encryption key is set", "[job_zstd][async][edge][crypto]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    JobZstd zstd;
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(false, true); }));

    REQUIRE_FALSE(zstd.errorString().empty());
}

TEST_CASE("JobZstd signed compress fails with a clear error when no signing keys are set", "[job_zstd][async][edge][sign]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstd zstd;
    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(true, false); }));

    REQUIRE_FALSE(zstd.errorString().empty());
    // Neither the archive nor a signature should exist -- the pipeline
    // should refuse before ever writing anything, not partially succeed.
    REQUIRE_FALSE(std::filesystem::exists(archive));
    REQUIRE_FALSE(std::filesystem::exists(scratch.root() / "payload.zst.sig"));
}

TEST_CASE("JobZstd signed decompress rejects a signature made with a different keypair", "[job_zstd][async][edge][sign][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    KeyPairFiles const realKeys = generateKeyPairFiles(scratch.root(), "real.pub", "real.priv");
    KeyPairFiles const wrongKeys = generateKeyPairFiles(scratch.root(), "wrong.pub", "wrong.priv");

    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstd signer;
    REQUIRE(signer.setPublicKeyFile(realKeys.pub));
    REQUIRE(signer.setPrivateKeyFile(realKeys.priv));
    REQUIRE(signer.setInput(src.string()));
    REQUIRE(signer.setOutput(archive.string()));
    REQUIRE(runAndWait(signer, [&]() { signer.compress(true, false); }));
    REQUIRE(signer.errorString().empty());

    JobZstd verifier;
    REQUIRE(verifier.setPublicKeyFile(wrongKeys.pub)); // Wrong public key on purpose.
    REQUIRE(verifier.setInput(archive.string()));
    REQUIRE(verifier.setOutput(restored.string()));
    REQUIRE(runAndWait(verifier, [&]() { verifier.decompress(true, false); }));

    REQUIRE_FALSE(verifier.errorString().empty());
    REQUIRE_FALSE(std::filesystem::exists(restored)); // Extraction must never have run.
}

TEST_CASE("JobZstd encrypted decompress fails when decrypted with the wrong key", "[job_zstd][async][edge][crypto][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content only the right key should reveal");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstd encryptor;
    encryptor.setPrivateKey(makeTestEncryptionKey());
    REQUIRE(encryptor.setInput(src.string()));
    REQUIRE(encryptor.setOutput(archive.string()));
    REQUIRE(runAndWait(encryptor, [&]() { encryptor.compress(false, true); }));
    REQUIRE(encryptor.errorString().empty());

    JobZstd decryptor;
    decryptor.setPrivateKey(makeTestEncryptionKey()); // A different, unrelated key.
    REQUIRE(decryptor.setInput(archive.string()));
    REQUIRE(decryptor.setOutput(restored.string()));
    REQUIRE(runAndWait(decryptor, [&]() { decryptor.decompress(false, true); }));

    REQUIRE_FALSE(decryptor.errorString().empty());
}

TEST_CASE("JobZstd isRunning is false before any operation and false again once one completes", "[job_zstd][async][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstd zstd;
    REQUIRE_FALSE(zstd.isRunning());

    REQUIRE(zstd.setInput(src.string()));
    REQUIRE(zstd.setOutput(archive.string()));
    REQUIRE(runAndWait(zstd, [&]() { zstd.compress(); }));

    REQUIRE_FALSE(zstd.isRunning());
}

TEST_CASE("JobZstd's destructor waits for in-flight work rather than tearing down underneath it", "[job_zstd][async][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", std::string(200000, 'x'));
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    {
        JobZstd zstd;
        REQUIRE(zstd.setInput(src.string()));
        REQUIRE(zstd.setOutput(archive.string()));
        REQUIRE(zstd.setCompressionLevel(19)); // Slow enough to likely still be running at scope exit.
        zstd.compress();
        // No wait here on purpose -- destructor runs immediately below.
    }

    REQUIRE(std::filesystem::exists(archive));
}

} // namespace job::zstd