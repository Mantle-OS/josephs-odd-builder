#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
#include  <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

// core
#include <split_mix64.h>

#include <job_zstd_compressor.h>
#include <job_zstd_decompressor.h>
#include <job_zstd_io.h>
#include <job_zstd_wire.h>
#include <job_zstd_entry.h>

#include "transient_test_filesystem.h"
#include "transient_test_corruption.h"

namespace job::zstd {

namespace {

struct ArchiveEntrySpec
{
    JobZstdEntryKind kind = JobZstdEntryKind::File;
    std::string      relPath;
    std::string      fileContent;   // only used when kind == File
    std::string      symlinkTarget; // only used when kind == Symlink
};

[[nodiscard]] bool buildDirArchive(const std::filesystem::path &path, const std::vector<ArchiveEntrySpec> &entries, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(level))
        return false;

    if (!zstd.open(JobZstdIO::Mode::WriteOnly))
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

    return zstd.close();
}

// Standalone single-value archives decompressEmptyDirectoryArchive() and
// decompressSymlinkArchive() are private, dispatched to only via execute()
// sniffing a top-level tag. JobZstdCompressor never actually produces
// these as a TOP-LEVEL tag on its own (only File or Directory), so this is
// the only way to exercise those two code paths at all.
[[nodiscard]] bool buildStandaloneEmptyDirArchive(const std::filesystem::path &path, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, JobZstdOptions::magicEmptyDirString());

    return zstd.close();
}

[[nodiscard]] bool buildStandaloneSymlinkArchive(const std::filesystem::path &path, const std::string &target, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, JobZstdOptions::magicLinkString());
    job::zstd::utils::writeString(out, target);

    return zstd.close();
}

[[nodiscard]] bool buildArchiveWithUnrecognizedTag(const std::filesystem::path &path, int level = 3)
{
    std::ofstream dst(path, std::ios::binary | std::ios::trunc);
    if (!dst)
        return false;

    JobZstdIO zstd(dst.rdbuf());
    if (!zstd.setCompressionLevel(level) || !zstd.open(JobZstdIO::Mode::WriteOnly))
        return false;

    std::ostream out(&zstd);
    job::zstd::utils::writeString(out, "NOT_A_REAL_TAG");

    return zstd.close();
}

[[nodiscard]] std::string readFileContent(const std::filesystem::path &path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream content;
    content << in.rdbuf();
    return content.str();
}

} // namespace

TEST_CASE("JobZstdDecompressor round-trips a flat file through the real compressor", "[job_zstd][decompressor][usage][roundtrip]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "The gate is open, the guard is asleep.");
    std::filesystem::path const archive = scratch.root() / "payload.zst";
    std::filesystem::path const restored = scratch.root() / "restored.txt";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    REQUIRE(decompressor.execute());

    REQUIRE(readFileContent(restored) == "The gate is open, the guard is asleep.");
}

TEST_CASE("JobZstdDecompressor round-trips a mixed directory tree through the real compressor", "[job_zstd][decompressor][usage][roundtrip]")
{
    // This is the scenario the whole format redesign was for: files at
    // multiple depths, an empty directory, and a symlink, all coming back
    // structurally identical to what went in.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "top.txt", "top content");
    scratch.makeFile(std::filesystem::path("tree") / "nested" / "deep.txt", "deep content");
    scratch.makeDir(std::filesystem::path("tree") / "empty_one");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "real content");
    scratch.makeSymlink(realFile, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const archive = scratch.root() / "tree.zst";
    std::filesystem::path const restored = scratch.root() / "restored_tree";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    REQUIRE(decompressor.execute());

    std::string compareError;
    bool const treesMatch = job::zstd::test::compareTrees(root, restored, compareError);
    // WARN("compareTrees error: " << compareError);
    REQUIRE(treesMatch);
}

TEST_CASE("JobZstdDecompressor round-trips a flattened archive back into a flat directory", "[job_zstd][decompressor][usage][roundtrip][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a" / "one.txt", "1");
    scratch.makeFile(std::filesystem::path("tree") / "b" / "two.txt", "2");

    std::filesystem::path const archive = scratch.root() / "tree.zst";
    std::filesystem::path const restored = scratch.root() / "restored_flat";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.setRecursiveDirectories(false));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    REQUIRE(decompressor.execute());

    REQUIRE(readFileContent(restored / "one.txt") == "1");
    REQUIRE(readFileContent(restored / "two.txt") == "2");
    REQUIRE_FALSE(std::filesystem::exists(restored / "a"));
    REQUIRE_FALSE(std::filesystem::exists(restored / "b"));
}

TEST_CASE("JobZstdDecompressor extracts a dereferenced symlink as real file content", "[job_zstd][decompressor][usage][roundtrip][symlink]")
{
    // Compressed with preserveSymlinks=false, so the archive never recorded
    // a Symlink entry at all -- link.txt should come back as an ordinary
    // file, not a symlink, regardless of what the decompressor's own
    // preserveSymlinks() is set to.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "real.txt", "real content");
    scratch.makeSymlink(scratch.root() / "tree" / "real.txt", std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const archive = scratch.root() / "tree.zst";
    std::filesystem::path const restored = scratch.root() / "restored_tree";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.setPreserveSymlinks(false));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    REQUIRE(decompressor.execute());

    std::error_code symEc;
    REQUIRE_FALSE(std::filesystem::is_symlink(restored / "link.txt", symEc));
    REQUIRE(readFileContent(restored / "link.txt") == "real content");
}

TEST_CASE("JobZstdDecompressor fires the finished callback on success", "[job_zstd][decompressor][usage][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    bool fired = false;
    JobZstdDecompressor decompressor;
    decompressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored.txt").string()));
    REQUIRE(decompressor.execute());

    REQUIRE(fired);
}

// ---------------------------------------------------------------------------
// Block two: edge cases
// ---------------------------------------------------------------------------

TEST_CASE("JobZstdDecompressor fails when input is unconfigured", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor fails when the input archive does not exist", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput((scratch.root() / "nope.zst").string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor fails when the input path is a directory", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const dir = scratch.makeDir("not_an_archive");

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(dir.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
}

TEST_CASE("JobZstdDecompressor refuses an archive with an unrecognized top-level tag", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const archive = scratch.root() / "bogus.zst";
    REQUIRE(buildArchiveWithUnrecognizedTag(archive));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor fails on a truncated archive", "[job_zstd][decompressor][edge][truncation]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", std::string(50000, 'x'));
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    std::string const fullBytes = readFileContent(archive);
    std::string const chopped = job::zstd::test::truncate(fullBytes, 0.5);

    std::ofstream(archive, std::ios::binary | std::ios::trunc).write(chopped.data(), static_cast<std::streamsize>(chopped.size()));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor fails on a corrupted archive", "[job_zstd][decompressor][edge][decodeerror]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", std::string(50000, 'x'));
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    std::string const fullBytes = readFileContent(archive);
    std::string const corrupted = job::zstd::test::flipBit(fullBytes, fullBytes.size() / 2);

    std::ofstream(archive, std::ios::binary | std::ios::trunc).write(corrupted.data(), static_cast<std::streamsize>(corrupted.size()));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor refuses a directory entry whose path escapes the extraction root", "[job_zstd][decompressor][edge][security][safejoin]")
{
    // The zip-slip shape -- an entry that LOOKS like a normal relative path
    // right up until you resolve the ".." components. safeJoin() inside
    // decompressFolder() is the thing that has to catch this.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const archive = scratch.root() / "malicious.zst";

    ArchiveEntrySpec evil;
    evil.kind = JobZstdEntryKind::File;
    evil.relPath = "../../../etc/cron.d/evil";
    evil.fileContent = "you should never see this on disk";

    REQUIRE(buildDirArchive(archive, {evil}));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "extract_here").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor refuses extraction through a pre-existing symlink already sitting in the output tree", "[job_zstd][decompressor][edge][security][symlinkcheck]")
{
    // Different attack shape from the one above: this archive is perfectly
    // innocent ("logs/output.txt" is a completely ordinary relative path).
    // The danger is entirely on-disk, planted before extraction ever runs --
    // verifyNoSymlinkComponents() is what has to catch this, not safeJoin().
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const extractRoot = scratch.makeDir("extract_here");
    std::filesystem::path const realTarget = scratch.makeDir("somewhere_else");
    scratch.makeDirSymlink(realTarget, "extract_here/logs");

    std::filesystem::path const archive = scratch.root() / "innocent.zst";

    ArchiveEntrySpec innocent;
    innocent.kind = JobZstdEntryKind::File;
    innocent.relPath = "logs/output.txt";
    innocent.fileContent = "just some log content";

    REQUIRE(buildDirArchive(archive, {innocent}));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(extractRoot.string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());

    // And the real target of the symlink better not have been written
    // through -- that's the whole point of refusing rather than just
    // failing after the fact.
    REQUIRE_FALSE(std::filesystem::exists(realTarget / "output.txt"));
}

TEST_CASE("JobZstdDecompressor refuses a symlink entry when preserveSymlinks is false", "[job_zstd][decompressor][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "content");
    scratch.makeSymlink(realFile, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const archive = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute()); // Compressed normally -- the archive DOES contain a Symlink entry.

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored_tree").string()));
    REQUIRE(decompressor.setPreserveSymlinks(false));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor extracts a standalone empty-directory archive", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const archive = scratch.root() / "empty.zst";
    REQUIRE(buildStandaloneEmptyDirArchive(archive));

    std::filesystem::path const restored = scratch.root() / "restored_empty_dir";

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restored.string()));
    REQUIRE(decompressor.execute());

    std::error_code isDirEc;
    REQUIRE(std::filesystem::is_directory(restored, isDirEc));
}

TEST_CASE("JobZstdDecompressor extracts a standalone symlink archive", "[job_zstd][decompressor][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const realFile = scratch.makeFile("real.txt", "content");
    std::filesystem::path const archive = scratch.root() / "link.zst";
    REQUIRE(buildStandaloneSymlinkArchive(archive, realFile.string()));

    std::filesystem::path const restoredLink = scratch.root() / "restored_link";

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput(restoredLink.string()));
    REQUIRE(decompressor.execute());

    std::error_code symEc;
    REQUIRE(std::filesystem::is_symlink(restoredLink, symEc));
    REQUIRE(readFileContent(restoredLink) == "content");
}

TEST_CASE("JobZstdDecompressor's standalone symlink archive respects preserveSymlinks", "[job_zstd][decompressor][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const realFile = scratch.makeFile("real.txt", "content");
    std::filesystem::path const archive = scratch.root() / "link.zst";
    REQUIRE(buildStandaloneSymlinkArchive(archive, realFile.string()));

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "restored_link").string()));
    REQUIRE(decompressor.setPreserveSymlinks(false));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(decompressor.errorString().empty());
}

TEST_CASE("JobZstdDecompressor decompressFolder rejects a flat-file archive", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out_dir").string()));

    REQUIRE_FALSE(decompressor.decompressFolder());
}

TEST_CASE("JobZstdDecompressor decompressFile rejects a directory archive", "[job_zstd][decompressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a.txt", "a");
    std::filesystem::path const archive = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(archive.string()));
    REQUIRE(compressor.execute());

    JobZstdDecompressor decompressor;
    REQUIRE(decompressor.setInput(archive.string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.decompressFile());
}

TEST_CASE("JobZstdDecompressor's finished callback does not fire when decompression fails", "[job_zstd][decompressor][edge][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    bool fired = false;
    JobZstdDecompressor decompressor;
    decompressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(decompressor.setInput((scratch.root() / "nope.zst").string()));
    REQUIRE(decompressor.setOutput((scratch.root() / "out.txt").string()));

    REQUIRE_FALSE(decompressor.execute());
    REQUIRE_FALSE(fired);
}

#ifdef JOB_TEST_BENCHMARKS
namespace {

void writeDeterministicFile(const std::filesystem::path &path, std::size_t size, std::uint64_t seed)
{
    std::string content(size, '\0');
    job::core::SplitMix64 rng(seed);
    std::size_t offset = 0;

    while (offset + sizeof(std::uint64_t) <= content.size()) {
        std::uint64_t const value = rng.next();
        std::memcpy(content.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
    }

    if (offset < content.size()) {
        std::uint64_t const value = rng.next();
        std::memcpy(content.data() + offset, &value, content.size() - offset);
    }

    std::ofstream out(path, std::ios::binary);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

} // namespace

TEST_CASE("JobZstdDecompressor throughput: flat file, directly comparable to the raw JobZstdIO baseline", "[job_zstd][decompressor][benchmark]")
{
    // Same 8MiB / level 3 shape as JobZstdIO's and JobZstdCompressor's own
    // "compress and decompress 8 MiB" benchmarks -- the gap versus those
    // numbers is the cost of everything JobZstdDecompressor adds on top of
    // raw decompression: execute()'s sniff-and-reopen dance, the real file
    // opens, the flat-file tag check.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.root() / "payload.bin";
    writeDeterministicFile(src, 8 * 1024 * 1024, 0xC0FFEE);

    std::filesystem::path const archive = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    compressor.setInput(src.string());
    compressor.setOutput(archive.string());
    compressor.setCompressionLevel(3);
    compressor.execute();

    std::filesystem::path const restored = scratch.root() / "restored.bin";

    BENCHMARK("decompress 8MiB flat file (via execute(), includes the sniff pass)")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.execute();
    };

    BENCHMARK("decompress 8MiB flat file (via decompressFile() directly, no sniff pass)")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.decompressFile();
    };
}

TEST_CASE("JobZstdDecompressor throughput: many small files vs one big file, same total bytes", "[job_zstd][decompressor][benchmark]")
{
    // Mirrors JobZstdCompressor's own "many small files" benchmark -- same
    // 8MiB total, same 64-file split, so the compress-side and decompress-
    // side per-file overhead can be compared against each other directly.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("many_files");

    constexpr int kFileCount = 64;
    constexpr std::size_t kFileSize = (8 * 1024 * 1024) / kFileCount;

    for (int i = 0; i < kFileCount; ++i) {
        std::filesystem::path const filePath = root / ("file_" + std::to_string(i) + ".bin");
        writeDeterministicFile(filePath, kFileSize, 0xDEADBEEF + static_cast<std::uint64_t>(i));
    }

    std::filesystem::path const archive = scratch.root() / "many_files.zst";

    JobZstdCompressor compressor;
    compressor.setInput(root.string());
    compressor.setOutput(archive.string());
    compressor.setCompressionLevel(3);
    compressor.execute();

    std::filesystem::path const restored = scratch.root() / "restored_many";

    BENCHMARK("decompress 8MiB across 64 files (level 3)")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.execute();
    };
}

TEST_CASE("JobZstdDecompressor throughput: execute()'s sniff pass overhead in isolation", "[job_zstd][decompressor][benchmark][execute]")
{
    // execute() always pays for a full extra open+read(tag)+close cycle
    // before dispatching to the real decompress*() function -- this
    // isolates just that cost, independent of payload size, by using a
    // deliberately tiny payload where the sniff pass is a meaningful
    // fraction of total work rather than noise.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("tiny.txt", "small payload");
    std::filesystem::path const archive = scratch.root() / "tiny.zst";

    JobZstdCompressor compressor;
    compressor.setInput(src.string());
    compressor.setOutput(archive.string());
    compressor.execute();

    std::filesystem::path const restored = scratch.root() / "restored_tiny.txt";

    BENCHMARK("decompress tiny payload via execute() (sniff + real decode)")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.execute();
    };

    BENCHMARK("decompress tiny payload via decompressFile() directly (no sniff)")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.decompressFile();
    };
}

TEST_CASE("JobZstdDecompressor throughput: symlink-heavy directory extraction", "[job_zstd][decompressor][benchmark][symlink]")
{
    // Unique to the decompressor: every Symlink entry pays for
    // verifyNoSymlinkComponents()'s full root-to-leaf filesystem walk, on
    // top of the actual create_symlink() call. This measures whether that
    // cost stays trivial at a realistic symlink count or starts showing up.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("symlink_tree");
    std::filesystem::path const realFile = scratch.makeFile(std::filesystem::path("symlink_tree") / "real.txt", "shared target content");

    constexpr int kSymlinkCount = 200;
    for (int i = 0; i < kSymlinkCount; ++i)
        scratch.makeSymlink(realFile, std::filesystem::path("symlink_tree") / ("link_" + std::to_string(i) + ".txt"));

    std::filesystem::path const archive = scratch.root() / "symlink_tree.zst";

    JobZstdCompressor compressor;
    compressor.setInput(root.string());
    compressor.setOutput(archive.string());
    compressor.execute();

    std::filesystem::path const restored = scratch.root() / "restored_symlinks";

    BENCHMARK("decompress 200 symlinks, each re-verified via verifyNoSymlinkComponents()")
    {
        JobZstdDecompressor decompressor;
        static_cast<void>(decompressor.setInput(archive.string()));
        static_cast<void>(decompressor.setOutput(restored.string()));
        return decompressor.execute();
    };
}

#endif




} // namespace job::zstd