// test_job_zstd_compressor_crypto.cpp
#include <catch2/catch_test_macros.hpp>

#include "job_zstd_compressor_crypto.h"
#include "job_zstd_io.h"
#include "job_zstd_wire.h"
#include "job_zstd_entry.h"
#include "job_zstd_decrypting_transport.h"
#include "job_random.h"
#include "transient_test_filesystem.h"

#include <sodium/crypto_secretbox.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>

namespace job::zstd {

namespace {

[[nodiscard]] job::crypto::JobSecureMem makeTestKey()
{
    job::crypto::JobSecureMem key(crypto_secretbox_KEYBYTES);
    job::crypto::JobRandom::secureBytes(key.data(), key.size());
    return key;
}

// Mirrors test_job_zstd_compressor.cpp's ParsedEntry/ParsedArchive exactly,
// with one addition: the source bytes are decrypted via
// JobZstdDecryptingTransport BEFORE being handed to JobZstdIO.
// Deliberately not using JobZstdDecompressorCrypto here
struct ParsedEntry
{
    JobZstdEntryKind kind = JobZstdEntryKind::File;
    std::string      relPath;
    std::string      symlinkTarget;
    std::string      fileContent;
};

struct ParsedArchive
{
    std::string              topTag;
    std::string              flatFileContent;
    std::vector<ParsedEntry> entries;
};

[[nodiscard]] bool decryptAndParseArchive(const std::string &path, const job::crypto::JobSecureMem &key,
                                          ParsedArchive &out, std::string &errorOut)
{
    std::ifstream src(path, std::ios::binary);
    if (!src) {
        errorOut = "Failed to open encrypted archive for parsing: " + path;
        return false;
    }

    JobZstdDecryptingTransport decTransport(src.rdbuf(), key);

    JobZstdIO zstd(&decTransport);
    if (!zstd.open(JobZstdIO::Mode::ReadOnly)) {
        errorOut = zstd.errorString();
        return false;
    }

    std::istream in(&zstd);

    if (!job::zstd::utils::readString(in, out.topTag)) {
        errorOut = "Failed to read top-level archive tag.";
        static_cast<void>(zstd.close());
        return false;
    }

    if (out.topTag == JobZstdOptions::magicDirString()) {
        std::uint64_t entryCount = 0;
        if (!job::zstd::utils::readU64(in, entryCount)) {
            errorOut = "Failed to read entry count.";
            static_cast<void>(zstd.close());
            return false;
        }

        for (std::uint64_t i = 0; i < entryCount; ++i) {
            std::string entryTag;
            std::string relPath;

            if (!job::zstd::utils::readString(in, entryTag) || !job::zstd::utils::readString(in, relPath)) {
                errorOut = "Truncated entry header at index " + std::to_string(i);
                static_cast<void>(zstd.close());
                return false;
            }

            auto const kindOpt = entryKindFromMagicString(entryTag);
            if (!kindOpt) {
                errorOut = "Unrecognized entry tag: " + entryTag;
                static_cast<void>(zstd.close());
                return false;
            }

            ParsedEntry entry;
            entry.kind    = *kindOpt;
            entry.relPath = relPath;

            switch (*kindOpt) {
            case JobZstdEntryKind::Symlink:
                if (!job::zstd::utils::readString(in, entry.symlinkTarget)) {
                    errorOut = "Truncated symlink target for: " + relPath;
                    static_cast<void>(zstd.close());
                    return false;
                }
                break;

            case JobZstdEntryKind::File: {
                std::uint64_t size = 0;
                if (!job::zstd::utils::readU64(in, size)) {
                    errorOut = "Truncated file size for: " + relPath;
                    static_cast<void>(zstd.close());
                    return false;
                }

                entry.fileContent.resize(size);
                if (size > 0 && !in.read(entry.fileContent.data(), static_cast<std::streamsize>(size))) {
                    errorOut = "Truncated file payload for: " + relPath;
                    static_cast<void>(zstd.close());
                    return false;
                }
                break;
            }

            case JobZstdEntryKind::Directory:
            case JobZstdEntryKind::EmptyDirectory:
                break;
            }

            out.entries.push_back(std::move(entry));
        }
    } else if (out.topTag == JobZstdOptions::magicFileString()) {
        std::ostringstream content;
        content << in.rdbuf();
        out.flatFileContent = content.str();
    } else {
        errorOut = "Unrecognized top-level archive tag: " + out.topTag;
        static_cast<void>(zstd.close());
        return false;
    }

    bool const ok = zstd.close();
    if (!ok && errorOut.empty())
        errorOut = zstd.errorString();

    return ok;
}

[[nodiscard]] std::optional<ParsedEntry> findEntry(const ParsedArchive &archive, const std::string &relPath)
{
    for (const auto &entry : archive.entries) {
        if (entry.relPath == relPath)
            return entry;
    }
    return std::nullopt;
}

[[nodiscard]] std::string readRawFile(const std::string &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

} // namespace

TEST_CASE("JobZstdCompressorCrypto encrypts a single file, recoverable with the correct key", "[job_zstd][compressorcrypto][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "The vault is sealed, the key is elsewhere.");
    std::filesystem::path const dst = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.hasKeys());
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(decryptAndParseArchive(dst.string(), key, archive, parseError));

    REQUIRE(archive.topTag == JobZstdOptions::magicFileString());
    REQUIRE(archive.flatFileContent == "The vault is sealed, the key is elsewhere.");
}

TEST_CASE("JobZstdCompressorCrypto encrypts a mixed directory tree", "[job_zstd][compressorcrypto][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "top.txt", "top content");
    scratch.makeFile(std::filesystem::path("tree") / "nested" / "deep.txt", "deep content");
    scratch.makeDir(std::filesystem::path("tree") / "empty_one");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "real content");
    scratch.makeSymlink(realFile, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const dst = scratch.root() / "tree.zst.enc";
    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(decryptAndParseArchive(dst.string(), key, archive, parseError));
    REQUIRE(archive.topTag == JobZstdOptions::magicDirString());

    auto const topFile = findEntry(archive, "top.txt");
    REQUIRE(topFile.has_value());
    REQUIRE(topFile->fileContent == "top content");

    auto const emptyDir = findEntry(archive, "empty_one");
    REQUIRE(emptyDir.has_value());
    REQUIRE(emptyDir->kind == JobZstdEntryKind::EmptyDirectory);

    auto const link = findEntry(archive, "link.txt");
    REQUIRE(link.has_value());
    REQUIRE(link->kind == JobZstdEntryKind::Symlink);
    REQUIRE(link->symlinkTarget == realFile.string());
}

TEST_CASE("JobZstdCompressorCrypto's compression still benefits from compress-then-encrypt ordering", "[job_zstd][compressorcrypto][usage][compressionlevel]")
{
    // Highly compressible plaintext should still produce a meaningfully
    // smaller encrypted archive than incompressible plaintext of the same
    // size -- proof the compression step is genuinely happening BEFORE
    // encryption, not being defeated by encrypting first (which was the
    // bug we explicitly chose to fix rather than replicate from the Qt
    // original).
    job::zstd::test::TransientTestFilesystem scratch;
    job::crypto::JobSecureMem const key = makeTestKey();

    std::string const repetitive(200000, 'A');
    std::filesystem::path const repetitiveSrc = scratch.makeFile("repetitive.txt", repetitive);
    std::filesystem::path const repetitiveDst = scratch.root() / "repetitive.zst.enc";

    JobZstdCompressorCrypto repetitiveCompressor;
    REQUIRE(repetitiveCompressor.setInput(repetitiveSrc.string()));
    REQUIRE(repetitiveCompressor.setOutput(repetitiveDst.string()));
    repetitiveCompressor.setEncryptionKey(key);
    REQUIRE(repetitiveCompressor.execute());

    std::string incompressible(200000, '\0');
    job::crypto::JobRandom::secureBytes(incompressible.data(), incompressible.size());
    std::filesystem::path const randomSrc = scratch.makeFile("random.bin", incompressible);
    std::filesystem::path const randomDst = scratch.root() / "random.zst.enc";

    JobZstdCompressorCrypto randomCompressor;
    REQUIRE(randomCompressor.setInput(randomSrc.string()));
    REQUIRE(randomCompressor.setOutput(randomDst.string()));
    randomCompressor.setEncryptionKey(key);
    REQUIRE(randomCompressor.execute());

    REQUIRE(std::filesystem::file_size(repetitiveDst) < std::filesystem::file_size(randomDst));
}

TEST_CASE("JobZstdCompressorCrypto's output does not contain the plaintext in the clear", "[job_zstd][compressorcrypto][usage][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::string const secret = "correct-horse-battery-staple-do-not-leak-me";
    std::filesystem::path const src = scratch.makeFile("secret.txt", secret);
    std::filesystem::path const dst = scratch.root() / "secret.zst.enc";

    job::crypto::JobSecureMem const key = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    compressor.setEncryptionKey(key);
    REQUIRE(compressor.execute());

    std::string const rawOutput = readRawFile(dst.string());
    REQUIRE(rawOutput.find(secret) == std::string::npos);
}

TEST_CASE("JobZstdCompressorCrypto fires the finished callback on success", "[job_zstd][compressorcrypto][usage][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const dst = scratch.root() / "payload.zst.enc";

    bool fired = false;
    JobZstdCompressorCrypto compressor;
    compressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    compressor.setEncryptionKey(makeTestKey());
    REQUIRE(compressor.execute());

    REQUIRE(fired);
}

// 2
TEST_CASE("JobZstdCompressorCrypto hasKeys is false until a correctly-sized key is set", "[job_zstd][compressorcrypto][edge]")
{
    JobZstdCompressorCrypto compressor;
    REQUIRE_FALSE(compressor.hasKeys());

    job::crypto::JobSecureMem const wrongSize(4);
    compressor.setEncryptionKey(wrongSize);
    REQUIRE_FALSE(compressor.hasKeys());

    compressor.setEncryptionKey(makeTestKey());
    REQUIRE(compressor.hasKeys());
}

TEST_CASE("JobZstdCompressorCrypto execute fails fast with a clear error when no key is set", "[job_zstd][compressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());

    // And no output should have been produced at all -- this is a fail-fast
    // check that happens before any file I/O, not a failure partway through.
    REQUIRE_FALSE(std::filesystem::exists(scratch.root() / "out.zst.enc"));
}

TEST_CASE("JobZstdCompressorCrypto compressFolder called directly also refuses without a key", "[job_zstd][compressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a.txt", "a");

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));

    REQUIRE_FALSE(compressor.compressFolder());
}

TEST_CASE("JobZstdCompressorCrypto compressFile called directly also refuses without a key", "[job_zstd][compressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));

    REQUIRE_FALSE(compressor.compressFile());
}

TEST_CASE("JobZstdCompressorCrypto fails when the input path does not exist", "[job_zstd][compressorcrypto][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput((scratch.root() / "nope.txt").string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));
    compressor.setEncryptionKey(makeTestKey());

    REQUIRE_FALSE(compressor.execute());
}

TEST_CASE("JobZstdCompressorCrypto's archive cannot be decrypted with the wrong key", "[job_zstd][compressorcrypto][edge][security]")
{
    // Proves the confidentiality property end to end -- the archive isn't
    // just SHAPED like an encrypted file, it genuinely refuses to open
    // under a key that wasn't used to seal it.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content only the right key should reveal");
    std::filesystem::path const dst = scratch.root() / "payload.zst.enc";

    job::crypto::JobSecureMem const correctKey = makeTestKey();
    job::crypto::JobSecureMem const wrongKey = makeTestKey();

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    compressor.setEncryptionKey(correctKey);
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE_FALSE(decryptAndParseArchive(dst.string(), wrongKey, archive, parseError));
    REQUIRE_FALSE(parseError.empty());
}

TEST_CASE("JobZstdCompressorCrypto's finished callback does not fire when no key is set", "[job_zstd][compressorcrypto][edge][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");

    bool fired = false;
    JobZstdCompressorCrypto compressor;
    compressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));
    REQUIRE_FALSE(compressor.execute());

    REQUIRE_FALSE(fired);
}

#ifdef __unix__
TEST_CASE("JobZstdCompressorCrypto refuses a FIFO as flat-file input", "[job_zstd][compressorcrypto][edge][specialfile]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const fifo = scratch.makeFifo("pipe");

    JobZstdCompressorCrypto compressor;
    REQUIRE(compressor.setInput(fifo.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst.enc").string()));
    compressor.setEncryptionKey(makeTestKey());

    REQUIRE_FALSE(compressor.execute());
}
#endif

} // namespace job::zstd