#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sodium/crypto_secretbox.h>
#include <job_random.h>

#include <job_zstd_compressor_crypto.h>
#include <job_zstd_decompressor_crypto.h>
#include <job_zstd_io.h>
#include <job_zstd_wire.h>
#include <job_zstd_entry.h>
#include <job_zstd_encrypting_transport.h>
#include "transient_test_filesystem.h"
#include "transient_test_corruption.h"
namespace job::zstd {

namespace {

[[nodiscard]] job::crypto::JobSecureMem makeTestKey()
{
    job::crypto::JobSecureMem key(crypto_secretbox_KEYBYTES);
    job::crypto::JobRandom::secureBytes(key.data(), key.size());
    return key;
}

struct ArchiveEntrySpec
{
    JobZstdEntryKind kind = JobZstdEntryKind::File;
    std::string      relPath;
    std::string      fileContent;
    std::string      symlinkTarget;
};

// Builds a hand-crafted encrypted directory archive, bypassing JobZstdCompressorCrypto entirely. Same reasoning as the plain
// decompressor's tests: this is the only way to construct malicious/malformed archives a real compressor would never write on purpose
// (a ".." escape, an unrecognized tag), now wrapped through JobZstdEncryptingTransport so they arrive as genuinely encrypted bytes.
[[nodiscard]] bool buildEncryptedDirArchive(const std::filesystem::path &path, const std::vector<ArchiveEntrySpec> &entries,
                                            const job::crypto::JobSecureMem &key, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdEncryptingTransport encTransport(dst.rdbuf(), key);
    JobZstdIO zstd(&encTransport);

    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, JobZstdOptions::magicDirString());
    job::zstd::utils::writeU64(out, static_cast<std::uint64_t>(entries.size()));

    for (const auto &e : entries) {
        job::zstd::utils::writeString(out, entryMagicString(e.kind));
        job::zstd::utils::writeString(out, e.relPath);

        switch (e.kind) {
        case JobZstdEntryKind::File:
            job::zstd::utils::writeU64(out, static_cast<std::uint64_t>(e.fileContent.size()));
            if (!e.fileContent.empty())
                out.write(e.fileContent.data(), static_cast<std::streamsize>(e.fileContent.size()));
            break;
        case JobZstdEntryKind::Symlink:
            job::zstd::utils::writeString(out, e.symlinkTarget);
            break;
        case JobZstdEntryKind::Directory:
        case JobZstdEntryKind::EmptyDirectory:
            break;
        }
    }

    if (!zstd.close())
        return false;

    return encTransport.finish();
}

[[nodiscard]] bool buildEncryptedStandaloneEmptyDirArchive(const std::filesystem::path &path, const job::crypto::JobSecureMem &key, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdEncryptingTransport encTransport(dst.rdbuf(), key);
    JobZstdIO zstd(&encTransport);

    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, JobZstdOptions::magicEmptyDirString());

    if (!zstd.close())
        return false;

    return encTransport.finish();
}

[[nodiscard]] bool buildEncryptedStandaloneSymlinkArchive(const std::filesystem::path &path, const std::string &target,
                                                          const job::crypto::JobSecureMem &key, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdEncryptingTransport encTransport(dst.rdbuf(), key);
    JobZstdIO zstd(&encTransport);

    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, JobZstdOptions::magicLinkString());
    job::zstd::utils::writeString(out, target);

    if (!zstd.close())
        return false;

    return encTransport.finish();
}

[[nodiscard]] bool buildEncryptedArchiveWithUnrecognizedTag(const std::filesystem::path &path, const job::crypto::JobSecureMem &key, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdEncryptingTransport encTransport(dst.rdbuf(), key);
    JobZstdIO zstd(&encTransport);

    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, "NOT_A_REAL_TAG");

    if (!zstd.close())
        return false;

    return encTransport.finish();
}

[[nodiscard]] std::string readFileContent(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

} // namespace

TEST_CASE("JobZstdDecompressorCrypto round-trips a flat file through the real encrypting compressor", "[job_zstd][decompressorcrypto][usage][roundtrip]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "The vault is sealed, the key is elsewhere.");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    REQUIRE(readFileContent(restored) == "The vault is sealed, the key is elsewhere.");
}

TEST_CASE("JobZstdDecompressorCrypto round-trips a mixed directory tree through the real encrypting compressor", "[job_zstd][decompressorcrypto][usage][roundtrip]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "top.txt", "top content");
    scratch.makeFile(std::filesystem::path("tree") / "nested" / "deep.txt", "deep content");
    scratch.makeDir(std::filesystem::path("tree") / "empty_one");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "real content");
    scratch.makeSymlink(realFile, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const archive = scratch.root() / "tree.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored_tree";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    std::string compareError;
    REQUIRE(job::zstd::test::compareTrees(root, restored, compareError));
}

TEST_CASE("JobZstdDecompressorCrypto round-trips a flattened encrypted archive back into a flat directory", "[job_zstd][decompressorcrypto][usage][roundtrip][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a" / "one.txt", "1");
    scratch.makeFile(std::filesystem::path("tree") / "b" / "two.txt", "2");

    std::filesystem::path const archive = scratch.root() / "tree.zst.enc";
    std::filesystem::path const restored = scratch.root() / "restored_flat";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.setRecursiveDirectories(false));
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    REQUIRE(readFileContent(restored / "one.txt") == "1");
    REQUIRE(readFileContent(restored / "two.txt") == "2");
    REQUIRE_FALSE(std::filesystem::exists(restored / "a"));
}

TEST_CASE("JobZstdDecompressorCrypto fires the finished callback on success", "[job_zstd][decompressorcrypto][usage][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    bool fired = false;
    JobZstdDecompressorCrypto decompressor;
    decompressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored.txt").string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    REQUIRE(fired);
}

// 2
TEST_CASE("JobZstdDecompressorCrypto hasKeys is false until a correctly-sized key is set", "[job_zstd][decompressorcrypto][edge]")
{
    JobZstdDecompressorCrypto decompressor;
    REQUIRE_FALSE(decompressor.hasKeys());

    decompressor.setDecryptionKey(job::crypto::JobSecureMem(4));
    REQUIRE_FALSE(decompressor.hasKeys());

    decompressor.setDecryptionKey(makeTestKey());
    REQUIRE(decompressor.hasKeys());
}

TEST_CASE("JobZstdDecompressorCrypto execute fails fast with a clear error when no key is set", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(makeTestKey());
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressorCrypto decompressFolder/decompressFile called directly also refuse without a key", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(makeTestKey());
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out").string()));

    REQUIRE_FALSE(decompressor.decompressFolder());
    REQUIRE_FALSE(decompressor.decompressFile());
}

TEST_CASE("JobZstdDecompressorCrypto reports an authentication failure, not a generic error, when the wrong key is used", "[job_zstd][decompressorcrypto][edge][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content only the right key should reveal");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const correctKey = makeTestKey();
    job::crypto::JobSecureMem const wrongKey = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(correctKey);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));
    decompressor.setDecryptionKey(wrongKey);

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
    // Specifically checking the message reflects authentication, not a
    // generic zstd decode complaint -- this is bestErrorMessage()'s whole
    // reason for existing, proven from the outside rather than assumed.
    REQUIRE(decompressor.errorString().find("authentication") != std::string::npos);
}

TEST_CASE("JobZstdDecompressorCrypto reports truncation, not authentication failure, when the archive is simply cut short", "[job_zstd][decompressorcrypto][edge][truncation]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", std::string(50000, 'x'));
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    std::string const fullBytes = readFileContent(archive);
    std::string const chopped = job::zstd::test::truncate(fullBytes, 0.5);
    std::ofstream(archive, std::ios::binary | std::ios::trunc).write(chopped.data(), static_cast<std::streamsize>(chopped.size()));

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressorCrypto refuses an archive with an unrecognized top-level tag", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();
    std::filesystem::path const archive = scratch.root() / "bogus.zst.enc";
    REQUIRE(buildEncryptedArchiveWithUnrecognizedTag(archive, key));

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressorCrypto refuses a directory entry whose path escapes the extraction root", "[job_zstd][decompressorcrypto][edge][security][safejoin]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();
    std::filesystem::path const archive = scratch.root() / "malicious.zst.enc";

    ArchiveEntrySpec evil;
    evil.kind = JobZstdEntryKind::File;
    evil.relPath = "../../../etc/cron.d/evil";
    evil.fileContent = "you should never see this on disk";

    REQUIRE(buildEncryptedDirArchive(archive, {evil}, key));

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "extract_here").string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressorCrypto refuses extraction through a pre-existing symlink already sitting in the output tree", "[job_zstd][decompressorcrypto][edge][security][symlinkcheck]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();

    std::filesystem::path const extractRoot = scratch.makeDir("extract_here");
    std::filesystem::path const realTarget = scratch.makeDir("somewhere_else");
    scratch.makeDirSymlink(realTarget, "extract_here/logs");

    std::filesystem::path const archive = scratch.root() / "innocent.zst.enc";

    ArchiveEntrySpec innocent;
    innocent.kind = JobZstdEntryKind::File;
    innocent.relPath = "logs/output.txt";
    innocent.fileContent = "just some log content";

    REQUIRE(buildEncryptedDirArchive(archive, {innocent}, key));

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(extractRoot.string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(std::filesystem::exists(realTarget / "output.txt"));
}

TEST_CASE("JobZstdDecompressorCrypto refuses a symlink entry when preserveSymlinks is false", "[job_zstd][decompressorcrypto][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "content");
    scratch.makeSymlink(realFile, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const archive = scratch.root() / "tree.zst.enc";
    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored_tree").string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.setPreserveSymlinks(false));

    REQUIRE_FALSE(decompressor.execute());
}

TEST_CASE("JobZstdDecompressorCrypto extracts a standalone encrypted empty-directory archive", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();
    std::filesystem::path const archive = scratch.root() / "empty.zst.enc";
    REQUIRE(buildEncryptedStandaloneEmptyDirArchive(archive, key));

    std::filesystem::path const restored = scratch.root() / "restored_empty_dir";

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    std::error_code isDirEc;
    REQUIRE(std::filesystem::is_directory(restored, isDirEc));
}

TEST_CASE("JobZstdDecompressorCrypto extracts a standalone encrypted symlink archive", "[job_zstd][decompressorcrypto][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();
    std::filesystem::path const realFile = scratch.makeFile("real.txt", "content");
    std::filesystem::path const archive = scratch.root() / "link.zst.enc";
    REQUIRE(buildEncryptedStandaloneSymlinkArchive(archive, realFile.string(), key));

    std::filesystem::path const restoredLink = scratch.root() / "restored_link";

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restoredLink.string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.execute());

    std::error_code symEc;
    REQUIRE(std::filesystem::is_symlink(restoredLink, symEc));
    REQUIRE(readFileContent(restoredLink) == "content");
}

TEST_CASE("JobZstdDecompressorCrypto's standalone symlink archive respects preserveSymlinks and verifyNoSymlinkComponents", "[job_zstd][decompressorcrypto][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();
    std::filesystem::path const realFile = scratch.makeFile("real.txt", "content");
    std::filesystem::path const archive = scratch.root() / "link.zst.enc";
    REQUIRE(buildEncryptedStandaloneSymlinkArchive(archive, realFile.string(), key));

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored_link").string()));
    decompressor.setDecryptionKey(key);
    REQUIRE(decompressor.setPreserveSymlinks(false));

    REQUIRE_FALSE(decompressor.execute());
}

TEST_CASE("JobZstdDecompressorCrypto decompressFolder rejects a flat-file encrypted archive", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";
    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out_dir").string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.decompressFolder());
}

TEST_CASE("JobZstdDecompressorCrypto decompressFile rejects a directory encrypted archive", "[job_zstd][decompressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a.txt", "a");
    std::filesystem::path const archive = scratch.root() / "tree.zst.enc";
    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    JobZstdDecompressorCrypto decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));
    decompressor.setDecryptionKey(key);

    REQUIRE_FALSE(decompressor.decompressFile());
}

TEST_CASE("JobZstdDecompressorCrypto's finished callback does not fire when decryption fails", "[job_zstd][decompressorcrypto][edge][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const correctKey = makeTestKey();
    job::crypto::JobSecureMem const wrongKey = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    compressor.setEncryptionKey(correctKey);
    REQUIRE(compressor.execute());

    bool fired = false;
    JobZstdDecompressorCrypto decompressor;
    decompressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));
    decompressor.setDecryptionKey(wrongKey);
    REQUIRE_FALSE(decompressor.execute());

    REQUIRE_FALSE(fired);
}

} // namespace job::zstd