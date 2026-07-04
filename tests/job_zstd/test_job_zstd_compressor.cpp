// test_job_zstd_compressor.cpp
#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif


#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <cstring>
#include <cstdint>

#include <split_mix64.h>

#include <job_zstd_compressor.h>
#include <job_zstd_io.h>
#include <job_zstd_wire.h>
#include <job_zstd_entry.h>

#include "transient_test_filesystem.h"
namespace job::zstd {

namespace {

// Deliberately NOT using JobZstdDecompressor here see file-level intent
// above. This is a minimal, independent reader of the exact wire format
// JobZstdCompressor writes, built from the same primitives (JobZstdIO,
// job::zstd::utils, entryKindFromMagicString) but with none of the
// decompressor's own logic in the loop.
struct ParsedEntry
{
    JobZstdEntryKind kind = JobZstdEntryKind::File;
    std::string      relPath;
    std::string      symlinkTarget;
    std::string      fileContent; // only meaningful when kind == File
};

struct ParsedArchive
{
    std::string             topTag;
    std::string             flatFileContent; // only meaningful when topTag == magicFileString()
    std::vector<ParsedEntry> entries;         // only populated when topTag == magicDirString()
};

[[nodiscard]] bool parseArchive(const std::string &path, ParsedArchive &out, std::string &errorOut)
{
    std::ifstream src(path, std::ios::binary);
    if (!src) {
        errorOut = "Failed to open archive for parsing: " + path;
        return false;
    }

    JobZstdIO zstd(src.rdbuf());
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
                break; // Nothing further to read -- the entry header is the whole thing.
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

} // namespace


TEST_CASE("JobZstdCompressor compresses a single file with a flat-file header", "[job_zstd][compressor][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "The gate is open, the guard is asleep.");
    std::filesystem::path const dst = scratch.root() / "payload.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    REQUIRE(archive.topTag == JobZstdOptions::magicFileString());
    REQUIRE(archive.flatFileContent == "The gate is open, the guard is asleep.");
}

TEST_CASE("JobZstdCompressor compresses a directory with a per-entry self-describing header", "[job_zstd][compressor][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "top.txt", "top content");
    scratch.makeFile(std::filesystem::path("tree") / "nested" / "deep.txt", "deep content");
    scratch.makeDir(std::filesystem::path("tree") / "empty_one");

    std::filesystem::path const dst = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));
    REQUIRE(archive.topTag == JobZstdOptions::magicDirString());

    auto const topFile = findEntry(archive, "top.txt");
    REQUIRE(topFile.has_value());
    REQUIRE(topFile->kind == JobZstdEntryKind::File);
    REQUIRE(topFile->fileContent == "top content");

    auto const deepFile = findEntry(archive, (std::filesystem::path("nested") / "deep.txt").generic_string());
    REQUIRE(deepFile.has_value());
    REQUIRE(deepFile->fileContent == "deep content");

    auto const nestedDir = findEntry(archive, "nested");
    REQUIRE(nestedDir.has_value());
    REQUIRE(nestedDir->kind == JobZstdEntryKind::Directory);

    auto const emptyDir = findEntry(archive, "empty_one");
    REQUIRE(emptyDir.has_value());
    REQUIRE(emptyDir->kind == JobZstdEntryKind::EmptyDirectory);
}

TEST_CASE("JobZstdCompressor records a symlink entry with its raw target", "[job_zstd][compressor][usage][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    std::filesystem::path const real = scratch.makeFile(std::filesystem::path("tree") / "real.txt", "content");
    scratch.makeSymlink(real, std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const dst = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    auto const link = findEntry(archive, "link.txt");
    REQUIRE(link.has_value());
    REQUIRE(link->kind == JobZstdEntryKind::Symlink);
    REQUIRE(link->symlinkTarget == real.string());
}

TEST_CASE("JobZstdCompressor omits empty directory entries when preserveEmptyDirectories is false", "[job_zstd][compressor][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeDir(std::filesystem::path("tree") / "empty_one");

    std::filesystem::path const dst = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.setPreserveEmptyDirectories(false));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    REQUIRE_FALSE(findEntry(archive, "empty_one").has_value());
}

TEST_CASE("JobZstdCompressor dereferences symlinks into File entries when preserveSymlinks is false", "[job_zstd][compressor][usage][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "real.txt", "real content");
    scratch.makeSymlink(scratch.root() / "tree" / "real.txt", std::filesystem::path("tree") / "link.txt");

    std::filesystem::path const dst = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.setPreserveSymlinks(false));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    auto const link = findEntry(archive, "link.txt");
    REQUIRE(link.has_value());
    REQUIRE(link->kind == JobZstdEntryKind::File);
    REQUIRE(link->fileContent == "real content");
}

TEST_CASE("JobZstdCompressor flattens nested paths to bare filenames when recursiveDirectories is false", "[job_zstd][compressor][usage][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a" / "one.txt", "1");
    scratch.makeFile(std::filesystem::path("tree") / "b" / "two.txt", "2");

    std::filesystem::path const dst = scratch.root() / "tree.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.setRecursiveDirectories(false));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    REQUIRE(findEntry(archive, "one.txt").has_value());
    REQUIRE(findEntry(archive, "two.txt").has_value());
    REQUIRE_FALSE(findEntry(archive, (std::filesystem::path("a") / "one.txt").generic_string()).has_value());
}

TEST_CASE("JobZstdCompressor fires the finished callback on success", "[job_zstd][compressor][usage][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const dst = scratch.root() / "payload.zst";

    bool fired = false;

    JobZstdCompressor compressor;
    compressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    REQUIRE(fired);
}

TEST_CASE("JobZstdCompressor's compression level actually affects output size", "[job_zstd][compressor][usage][compressionlevel]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::string const content(200000, 'A');
    std::filesystem::path const src = scratch.makeFile("payload.txt", content);

    std::filesystem::path const dstLow  = scratch.root() / "low.zst";
    std::filesystem::path const dstHigh = scratch.root() / "high.zst";

    JobZstdCompressor low;
    REQUIRE(low.setInput(src.string()));
    REQUIRE(low.setOutput(dstLow.string()));
    REQUIRE(low.setCompressionLevel(1));
    REQUIRE(low.execute());

    JobZstdCompressor high;
    REQUIRE(high.setInput(src.string()));
    REQUIRE(high.setOutput(dstHigh.string()));
    REQUIRE(high.setCompressionLevel(19));
    REQUIRE(high.execute());

    REQUIRE(std::filesystem::file_size(dstHigh) <= std::filesystem::file_size(dstLow));

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dstHigh.string(), archive, parseError));
    REQUIRE(archive.flatFileContent == content);
}

// 2
TEST_CASE("JobZstdCompressor fails when input is unconfigured", "[job_zstd][compressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    JobZstdCompressor compressor;
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}

TEST_CASE("JobZstdCompressor fails when output is unconfigured", "[job_zstd][compressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}

TEST_CASE("JobZstdCompressor fails when the input path does not exist", "[job_zstd][compressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput((scratch.root() / "nope.txt").string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}

TEST_CASE("JobZstdCompressor creates missing parent directories for the output path", "[job_zstd][compressor][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.makeFile("payload.txt", "content");
    std::filesystem::path const dst = scratch.root() / "does" / "not" / "exist" / "yet" / "out.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(src.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    REQUIRE(std::filesystem::exists(dst));
}

TEST_CASE("JobZstdCompressor produces a valid, empty directory archive when the input directory has nothing in it", "[job_zstd][compressor][edge]")
{
    // The root itself never gets an entry -- only its CHILDREN are candidates
    // for File/Directory/EmptyDirectory/Symlink entries. A root with zero
    // children should still be a well-formed archive, just with a zero
    // entry count, not an error.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("empty_root");
    std::filesystem::path const dst = scratch.root() / "out.zst";

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput(dst.string()));
    REQUIRE(compressor.execute());

    ParsedArchive archive;
    std::string parseError;
    REQUIRE(parseArchive(dst.string(), archive, parseError));

    REQUIRE(archive.topTag == JobZstdOptions::magicDirString());
    REQUIRE(archive.entries.empty());
}

TEST_CASE("JobZstdCompressor propagates a broken symlink error from the underlying walk", "[job_zstd][compressor][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeSymlink(scratch.root() / "tree" / "does_not_exist.txt", std::filesystem::path("tree") / "broken.txt");

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}

TEST_CASE("JobZstdCompressor fails cleanly on a flattening basename collision without hanging", "[job_zstd][compressor][edge][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("tree");
    scratch.makeFile(std::filesystem::path("tree") / "a" / "same.txt", "from a");
    scratch.makeFile(std::filesystem::path("tree") / "b" / "same.txt", "from b");

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(root.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string()));
    REQUIRE(compressor.setRecursiveDirectories(false));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}

TEST_CASE("JobZstdCompressor's finished callback does not fire when compression fails", "[job_zstd][compressor][edge][callback]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    bool fired = false;

    JobZstdCompressor compressor;
    compressor.setOnFinished([&fired]() {
        fired = true;
    });

    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string())); // Input left unconfigured on purpose.
    REQUIRE_FALSE(compressor.execute());

    REQUIRE_FALSE(fired);
}

#ifdef __unix__
TEST_CASE("JobZstdCompressor refuses a FIFO as flat-file input", "[job_zstd][compressor][edge][specialfile]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const fifo = scratch.makeFifo("pipe");

    JobZstdCompressor compressor;
    REQUIRE(compressor.setInput(fifo.string()));
    REQUIRE(compressor.setOutput((scratch.root() / "out.zst").string()));

    REQUIRE_FALSE(compressor.execute());
    REQUIRE_FALSE(compressor.errorString().empty());
}
#endif



// test_job_zstd_compressor.cpp -- append before the closing namespace brace

// ---------------------------------------------------------------------------
// Block three: benchmarks
// ---------------------------------------------------------------------------

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>

namespace {

// Same deterministic-filler idea as JobZstdIO's benchmarks, but writing
// straight to a real file on disk -- JobZstdCompressor operates on paths,
// not streambufs, so the comparison only works if both sides are working
// from the same real bytes sitting in the same kind of place.
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

TEST_CASE("JobZstdCompressor throughput: flat file, directly comparable to the raw JobZstdIO baseline", "[job_zstd][compressor][benchmark]")
{
    // Same 8MiB size, same seed pattern, same levels as JobZstdIO's own
    // "compress and decompress 8 MiB" benchmark -- any gap between these
    // numbers and that one IS the cost of everything JobZstdCompressor
    // adds on top of raw streambuf-to-streambuf compression: real file
    // opens, stat() for size, writing the flat-file header tag, dispatch
    // through execute(). If that gap ever grows unexpectedly, this is the
    // benchmark that would show it.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const src = scratch.root() / "payload.bin";
    writeDeterministicFile(src, 8 * 1024 * 1024, 0xC0FFEE);

    std::filesystem::path const dst = scratch.root() / "payload.zst";

    BENCHMARK("compress 8MiB flat file (level 1, fast)")
    {
        JobZstdCompressor compressor;
        static_cast<void>(compressor.setInput(src.string()));
        static_cast<void>(compressor.setOutput(dst.string()));
        static_cast<void>(compressor.setCompressionLevel(1));
        return compressor.execute();
    };

    BENCHMARK("compress 8MiB flat file (level 3, default)")
    {
        JobZstdCompressor compressor;
        static_cast<void>(compressor.setInput(src.string()));
        static_cast<void>(compressor.setOutput(dst.string()));
        static_cast<void>(compressor.setCompressionLevel(3));
        return compressor.execute();
    };
}

TEST_CASE("JobZstdCompressor throughput: many small files vs one big file, same total bytes", "[job_zstd][compressor][benchmark]")
{
    // Same total payload (8MiB) as the flat-file benchmark above, spread
    // across 64 files instead of one. The delta between this number and
    // that one is the actual cost of the directory walk itself: 64 stat()
    // calls, 64 entry headers, 64 ifstream opens instead of one of each.
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const root = scratch.makeDir("many_files");

    constexpr int kFileCount = 64;
    constexpr std::size_t kFileSize = (8 * 1024 * 1024) / kFileCount;

    for (int i = 0; i < kFileCount; ++i) {
        std::filesystem::path const filePath = root / ("file_" + std::to_string(i) + ".bin");
        writeDeterministicFile(filePath, kFileSize, 0xDEADBEEF + static_cast<std::uint64_t>(i));
    }

    std::filesystem::path const dst = scratch.root() / "many_files.zst";

    BENCHMARK("compress 8MiB across 64 files (level 3)")
    {
        JobZstdCompressor compressor;
        static_cast<void>(compressor.setInput(root.string()));
        static_cast<void>(compressor.setOutput(dst.string()));
        static_cast<void>(compressor.setCompressionLevel(3));
        return compressor.execute();
    };
}

TEST_CASE("JobZstdCompressor throughput: compressible vs incompressible flat file, directly comparable to the raw JobZstdIO baseline", "[job_zstd][compressor][benchmark]")
{
    // Same shape as JobZstdIO's "highly compressible vs incompressible
    // payloads" benchmark, same sizes -- again, the gap versus those
    // numbers is purely file-layer overhead, not a difference in
    // compression behavior itself.
    job::zstd::test::TransientTestFilesystem scratch;

    std::filesystem::path const repetitivePath = scratch.root() / "repetitive.bin";
    {
        std::string const repetitive(4 * 1024 * 1024, 'A');
        std::ofstream out(repetitivePath, std::ios::binary);
        out.write(repetitive.data(), static_cast<std::streamsize>(repetitive.size()));
    }

    std::filesystem::path const randomPath = scratch.root() / "random.bin";
    writeDeterministicFile(randomPath, 4 * 1024 * 1024, 0xFEEDFACE);

    std::filesystem::path const dst = scratch.root() / "out.zst";

    BENCHMARK("compress 4MiB repetitive flat file (level 3)")
    {
        JobZstdCompressor compressor;
        static_cast<void>(compressor.setInput(repetitivePath.string()));
        static_cast<void>(compressor.setOutput(dst.string()));
        static_cast<void>(compressor.setCompressionLevel(3));
        return compressor.execute();
    };

    BENCHMARK("compress 4MiB random flat file (level 3)")
    {
        JobZstdCompressor compressor;
        static_cast<void>(compressor.setInput(randomPath.string()));
        static_cast<void>(compressor.setOutput(dst.string()));
        static_cast<void>(compressor.setCompressionLevel(3));
        return compressor.execute();
    };
}

#endif























} // namespace job::zstd