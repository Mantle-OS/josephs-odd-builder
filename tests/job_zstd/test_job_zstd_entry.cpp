#include <catch2/catch_test_macros.hpp>

#include "job_zstd_entry.h"
#include "transient_test_filesystem.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace job::zstd {

namespace {

[[nodiscard]] bool containsKind(const std::vector<JobPendingEntry> &entries, const std::filesystem::path &relPath, JobZstdEntryKind kind)
{
    return std::any_of(entries.begin(), entries.end(), [&](const JobPendingEntry &e) {
        return e.relativePath == relPath && e.kind == kind;
    });
}

[[nodiscard]] bool containsPath(const std::vector<JobPendingEntry> &entries, const std::filesystem::path &relPath)
{
    return std::any_of(entries.begin(), entries.end(), [&](const JobPendingEntry &e) {
        return e.relativePath == relPath;
    });
}

} // namespace

TEST_CASE("entryMagicString maps each kind to its own distinct tag", "[job_zstd][entry][usage][magic]")
{
    REQUIRE(entryMagicString(JobZstdEntryKind::File)           == JobZstdOptions::magicFileString());
    REQUIRE(entryMagicString(JobZstdEntryKind::Directory)      == JobZstdOptions::magicDirString());
    REQUIRE(entryMagicString(JobZstdEntryKind::EmptyDirectory) == JobZstdOptions::magicEmptyDirString());
    REQUIRE(entryMagicString(JobZstdEntryKind::Symlink)        == JobZstdOptions::magicLinkString());
}

TEST_CASE("entryKindFromMagicString reverses entryMagicString for every kind", "[job_zstd][entry][usage][magic]")
{
    for (JobZstdEntryKind const kind : {JobZstdEntryKind::File, JobZstdEntryKind::Directory, JobZstdEntryKind::EmptyDirectory, JobZstdEntryKind::Symlink}) {
        auto const roundTripped = entryKindFromMagicString(entryMagicString(kind));
        REQUIRE(roundTripped.has_value());
        REQUIRE(*roundTripped == kind);
    }
}

TEST_CASE("safeJoin accepts a well-behaved relative path", "[job_zstd][entry][usage][safejoin]")
{
    job::zstd::test::TransientTestFilesystem scratch;

    auto const result = safeJoin(scratch.root(), std::filesystem::path("nested") / "file.txt");
    REQUIRE(result.has_value());
}

TEST_CASE("collectEntries finds regular files at multiple depths", "[job_zstd][entry][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile("top.txt", "top");
    scratch.makeFile(std::filesystem::path("nested") / "deep.txt", "deep");

    std::vector<JobPendingEntry> entries;
    std::string error;

    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE(error.empty());

    REQUIRE(containsKind(entries, "top.txt", JobZstdEntryKind::File));
    REQUIRE(containsKind(entries, std::filesystem::path("nested") / "deep.txt", JobZstdEntryKind::File));
}

TEST_CASE("collectEntries records empty directories only when preserveEmptyDirectories is true", "[job_zstd][entry][usage]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeDir("empty_one");

    std::vector<JobPendingEntry> withDirs;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, withDirs, error));
    REQUIRE(containsKind(withDirs, "empty_one", JobZstdEntryKind::EmptyDirectory));

    std::vector<JobPendingEntry> withoutDirs;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), false, true, withoutDirs, error));
    REQUIRE_FALSE(containsPath(withoutDirs, "empty_one"));
}

TEST_CASE("collectEntries always records non-empty directories regardless of preserveEmptyDirectories", "[job_zstd][entry][usage]")
{
    // A directory with something in it isn't optional. Only an empty one is ever affected by that flag.
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile(std::filesystem::path("populated") / "file.txt", "hi");

    std::vector<JobPendingEntry> entries;
    std::string error;

    REQUIRE(collectEntries(scratch.root(), scratch.root(), false, true, entries, error));

    REQUIRE(containsKind(entries, "populated", JobZstdEntryKind::Directory));
    REQUIRE(containsKind(entries, std::filesystem::path("populated") / "file.txt", JobZstdEntryKind::File));
}

TEST_CASE("collectEntries preserves symlinks as symlink entries when syslink is true", "[job_zstd][entry][usage][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile("real.txt", "real content");
    scratch.makeSymlink(scratch.root() / "real.txt", "link.txt");

    std::vector<JobPendingEntry> entries;
    std::string error;

    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE(containsKind(entries, "link.txt", JobZstdEntryKind::Symlink));
}

TEST_CASE("collectEntries dereferences file symlinks into File entries when syslink is false", "[job_zstd][entry][usage][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile("real.txt", "real content");
    scratch.makeSymlink(scratch.root() / "real.txt", "link.txt");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, false, entries, error));

    REQUIRE(containsKind(entries, "link.txt", JobZstdEntryKind::File));
    REQUIRE_FALSE(containsKind(entries, "link.txt", JobZstdEntryKind::Symlink));
}

TEST_CASE("collectEntries dereferences a symlinked directory when syslink is false", "[job_zstd][entry][usage][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile(std::filesystem::path("realdir") / "inside.txt", "content");
    scratch.makeDirSymlink(scratch.root() / "realdir", "linkdir");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, false, entries, error));

    REQUIRE(containsKind(entries, std::filesystem::path("linkdir") / "inside.txt", JobZstdEntryKind::File));
}

TEST_CASE("flattenEntries strips directory structure down to bare filenames", "[job_zstd][entry][usage][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile(std::filesystem::path("a") / "one.txt", "1");
    scratch.makeFile(std::filesystem::path("b") / "two.txt", "2");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE(flattenEntries(entries, error));

    REQUIRE(containsKind(entries, "one.txt", JobZstdEntryKind::File));
    REQUIRE(containsKind(entries, "two.txt", JobZstdEntryKind::File));
    REQUIRE_FALSE(containsPath(entries, std::filesystem::path("a") / "one.txt"));
}


// 2
TEST_CASE("entryMagicString throws for an unrecognized enum value", "[job_zstd][entry][edge][magic]")
{
    // There's no legitimate way to construct this through normal code. It exists purely to prove the "every enumerator handled above" safety
    // net in the switch actually fires. Instead of silently returning garbage or reading past the end of something.
    auto const bogus = static_cast<JobZstdEntryKind>(99);
    REQUIRE_THROWS_AS(entryMagicString(bogus), std::logic_error);
}

TEST_CASE("entryKindFromMagicString returns nullopt for an unrecognized tag", "[job_zstd][entry][edge][magic]")
{
    REQUIRE_FALSE(entryKindFromMagicString("NOT_A_REAL_TAG").has_value());
    REQUIRE_FALSE(entryKindFromMagicString("").has_value());
}

TEST_CASE("safeJoin rejects an absolute path outright", "[job_zstd][entry][edge][safejoin]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    auto const result = safeJoin(scratch.root(), std::filesystem::path("/etc/passwd"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("safeJoin rejects a dot-dot escape out of baseDir", "[job_zstd][entry][edge][safejoin][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    auto const result = safeJoin(scratch.root(), std::filesystem::path("../../../etc/cron.d/evil"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("safeJoin rejects a dot-dot escape buried in the middle of the path", "[job_zstd][entry][edge][safejoin][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    auto const result = safeJoin(scratch.root(), std::filesystem::path("nested/../../escape.txt"));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("safeJoin accepts a dot-dot that stays inside baseDir", "[job_zstd][entry][edge][safejoin]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeDir(std::filesystem::path("a") / "b");

    // "a/b/../c" normalizes to "a/c" . still under root(), so this one's allowed. The rule is "stay inside," not "no dots allowed."
    auto const result = safeJoin(scratch.root(), std::filesystem::path("a/b/../c"));
    REQUIRE(result.has_value());
}


TEST_CASE("verifyNoSymlinkComponents does not leave a stale error message behind on success", "[job_zstd][entry][edge][symlinkcheck]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    std::filesystem::path const target = scratch.root() / "nested" / "file.txt";

    std::string error = "leftover from a previous call";
    REQUIRE(verifyNoSymlinkComponents(target, error));
    REQUIRE(error.empty());
}

TEST_CASE("verifyNoSymlinkComponents rejects a path with a symlinked intermediate directory", "[job_zstd][entry][edge][symlinkcheck][security]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeDir("real");
    scratch.makeDirSymlink(scratch.root() / "real", "linked");

    std::filesystem::path const target = scratch.root() / "linked" / "child.txt";

    std::string error;
    REQUIRE_FALSE(verifyNoSymlinkComponents(target, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("verifyNoSymlinkComponents rejects a path whose leaf is itself a symlink", "[job_zstd][entry][edge][symlinkcheck]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile("real.txt", "content");
    std::filesystem::path const link = scratch.makeSymlink(scratch.root() / "real.txt", "link.txt");

    std::string error;
    REQUIRE_FALSE(verifyNoSymlinkComponents(link, error));
}

TEST_CASE("collectEntries fails cleanly on a nonexistent directory", "[job_zstd][entry][edge]")
{
    std::vector<JobPendingEntry> entries;
    std::string error;

    std::filesystem::path const bogus = std::filesystem::temp_directory_path() / "this_had_better_not_exist_9f3a";
    REQUIRE_FALSE(collectEntries(bogus, bogus, true, true, entries, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("collectEntries only marks the innermost genuinely-empty directory in a nested chain", "[job_zstd][entry][edge]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeDir(std::filesystem::path("a") / "b" / "c");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));

    // "a" and "a/b" both contain something (each other), so they're... Directory entries, only "a/b/c" is genuinely empty.
    REQUIRE(containsKind(entries, "a", JobZstdEntryKind::Directory));
    REQUIRE(containsKind(entries, std::filesystem::path("a") / "b", JobZstdEntryKind::Directory));
    REQUIRE(containsKind(entries, std::filesystem::path("a") / "b" / "c", JobZstdEntryKind::EmptyDirectory));
}

TEST_CASE("collectEntries refuses a broken symlink when syslink is true", "[job_zstd][entry][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeSymlink(scratch.root() / "does_not_exist.txt", "broken.txt");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE_FALSE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("collectEntries refuses a broken symlink when dereferencing too", "[job_zstd][entry][edge][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeSymlink(scratch.root() / "does_not_exist.txt", "broken.txt");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE_FALSE(collectEntries(scratch.root(), scratch.root(), true, false, entries, error));
    REQUIRE_FALSE(error.empty());
}

TEST_CASE("collectEntries detects a symlink cycle when dereferencing", "[job_zstd][entry][edge][symlink][security]")
{
    // this test is the reason that guard is not optional.
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeSymlinkCycle(std::filesystem::path("a") / "loop", "a");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE_FALSE(collectEntries(scratch.root(), scratch.root(), true, false, entries, error));
    REQUIRE_FALSE(error.empty());
}

#ifdef __unix__
TEST_CASE("collectEntries refuses a FIFO instead of silently archiving it as a regular file", "[job_zstd][entry][edge][specialfile]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFifo("pipe");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE_FALSE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE_FALSE(error.empty());
}
#endif

TEST_CASE("flattenEntries refuses on a basename collision instead of silently dropping data", "[job_zstd][entry][edge][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile(std::filesystem::path("a") / "same.txt", "from a");
    scratch.makeFile(std::filesystem::path("b") / "same.txt", "from b");

    std::vector<JobPendingEntry> entries;
    std::string collectError;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, collectError));

    std::string flattenError;
    REQUIRE_FALSE(flattenEntries(entries, flattenError));
    REQUIRE_FALSE(flattenError.empty());
}

TEST_CASE("flattenEntries preserves symlink entries, just renamed to their basename", "[job_zstd][entry][edge][flatten][symlink]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeFile("real.txt", "content");
    scratch.makeSymlink(scratch.root() / "real.txt", std::filesystem::path("nested") / "link.txt");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE(flattenEntries(entries, error));

    REQUIRE(containsKind(entries, "link.txt", JobZstdEntryKind::Symlink));
}

TEST_CASE("flattenEntries on an all-directories tree produces an empty entry list", "[job_zstd][entry][edge][flatten]")
{
    job::zstd::test::TransientTestFilesystem scratch;
    scratch.makeDir("empty_one");
    scratch.makeDir(std::filesystem::path("a") / "empty_two");

    std::vector<JobPendingEntry> entries;
    std::string error;
    REQUIRE(collectEntries(scratch.root(), scratch.root(), true, true, entries, error));
    REQUIRE(flattenEntries(entries, error));

    REQUIRE(entries.empty());
}

} // namespace job::zstd