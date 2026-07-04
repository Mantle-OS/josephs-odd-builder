#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <optional>
#include <system_error>
#include <algorithm>
#include <stdexcept>

#include "job_zstd_options.h"
#include "job_zstd_dir_guard.h"

namespace job::zstd {

enum class JobZstdEntryKind : std::uint8_t
{
    File           = 0,
    Directory      = 1,
    EmptyDirectory = 2,
    Symlink        = 3,
};

struct JobPendingEntry
{
    JobZstdEntryKind      kind = JobZstdEntryKind::File;
    std::filesystem::path relativePath;   // path inside the archive, root-relative
    std::filesystem::path sourcePath;     // where to actually read bytes from (File only)
    std::string           symlinkTarget;  // raw, unresolved readlink() text (Symlink only)
};

[[nodiscard]] inline const std::string &entryMagicString(JobZstdEntryKind kind)
{
    switch (kind) {
    case JobZstdEntryKind::File:
        return JobZstdOptions::magicFileString();
    case JobZstdEntryKind::Directory:
        return JobZstdOptions::magicDirString();
    case JobZstdEntryKind::EmptyDirectory:
        return JobZstdOptions::magicEmptyDirString();
    case JobZstdEntryKind::Symlink:
        return JobZstdOptions::magicLinkString();
    }

    throw std::logic_error("entryMagicString: unhandled JobZstdEntryKind");
}

[[nodiscard]] inline std::optional<JobZstdEntryKind> entryKindFromMagicString(const std::string &tag)
{
    if (tag == JobZstdOptions::magicFileString())
        return JobZstdEntryKind::File;

    if (tag == JobZstdOptions::magicDirString())
        return JobZstdEntryKind::Directory;

    if (tag == JobZstdOptions::magicEmptyDirString())
        return JobZstdEntryKind::EmptyDirectory;

    if (tag == JobZstdOptions::magicLinkString())
        return JobZstdEntryKind::Symlink;

    return std::nullopt;
}




// Walks every component of fullPath, from the filesystem root down to (and
// including) the leaf itself, refusing the moment anything already on disk
// turns out to be a symlink. Applied uniformly to every extraction target --
// archive-derived or caller-chosen, folder entry or standalone output --
// because a symlink planted along the way doesn't care whose idea the path
// was.
//
// Portable on purpose (std::filesystem, not POSIX openat/O_NOFOLLOW) --
// job_zstd builds on the Qt desktop side too, not just Linux.
//
// Note: there's an inherent check-then-use gap between this call and
// whatever creates/opens the target right after it.... something else could
// still swap a symlink in between the two. Fully closing that needs an
// OS-level atomic primitive behind a platform abstraction, which doesn't
// exist here yet. This closes "something already on disk redirects
// extraction," which is the realistic threat; it is not a defense against
// an actively racing concurrent attacker.
[[nodiscard]] inline bool verifyNoSymlinkComponents(const std::filesystem::path &fullPath, std::string &errorOut)
{
    errorOut.clear(); // Don't let a stale message from an earlier call survive into a clean success.

    std::error_code absEc;
    std::filesystem::path const absPath = std::filesystem::absolute(fullPath, absEc).lexically_normal();

    if (absEc) {
        errorOut = "Failed to resolve absolute path for symlink checking: " + fullPath.string();
        return false;
    }

    std::filesystem::path const root             = absPath.root_path();
    std::filesystem::path const relativeFromRoot = absPath.lexically_relative(root);
    std::filesystem::path current                = root;

    for (const auto &part : relativeFromRoot) {
        current /= part;

        std::error_code statusEc;
        std::filesystem::file_status const status = std::filesystem::symlink_status(current, statusEc);

        if (statusEc)
            continue; // Doesn't exist yet -- nothing to be tricked by, it'll be created fresh.

        if (status.type() == std::filesystem::file_type::symlink) {
            errorOut = "Refusing to extract through a pre-existing symlink at: " + current.string();
            return false;
        }
    }

    return true;
}

[[nodiscard]] inline std::optional<std::filesystem::path> safeJoin(
    const std::filesystem::path &baseDir,
    const std::filesystem::path &relativePath)
{
    if (relativePath.is_absolute())
        return std::nullopt; // An archive telling you where on YOUR disk to write is a red flag, not a request.

    std::filesystem::path const base      = std::filesystem::weakly_canonical(baseDir).lexically_normal();
    std::filesystem::path const candidate = (base / relativePath).lexically_normal();

    auto const baseEnd        = base.end();
    auto const mismatchResult = std::mismatch(base.begin(), baseEnd, candidate.begin(), candidate.end());

    if (mismatchResult.first != baseEnd)
        return std::nullopt;

    return candidate;
}

[[nodiscard]] inline bool collectEntriesRecursive(
    const std::filesystem::path &root,
    const std::filesystem::path &currentDir,
    bool preserveEmptyDirectories,
    bool syslink,
    std::vector<JobPendingEntry> &entries,
    std::string &errorOut,
    std::vector<std::filesystem::path> &activeDirs)
{
    std::error_code canonEc;
    std::filesystem::path const canonicalCurrent = std::filesystem::weakly_canonical(currentDir, canonEc);
    if (canonEc) {
        errorOut = "Failed to resolve canonical path for cycle detection: " + currentDir.string();
        return false;
    }

    if (std::find(activeDirs.begin(), activeDirs.end(), canonicalCurrent) != activeDirs.end()) {
        errorOut = "Symlink cycle detected -- refusing to walk back into an ancestor directory: " + currentDir.string();
        return false;
    }

    JobZstdDirGuard const guard(activeDirs, canonicalCurrent);

    std::error_code dirEc;
    std::filesystem::directory_iterator it(currentDir, dirEc);
    if (dirEc) {
        errorOut = "Failed to open directory for traversal: " + currentDir.string() + " (" + dirEc.message() + ")";
        return false;
    }

    for (const auto &child : it) {
        std::filesystem::path const childPath = child.path();
        std::error_code relEc;
        // std::filesystem::path const relPath = std::filesystem::relative(childPath, root, relEc);
        std::filesystem::path const relPath   = childPath.lexically_relative(root);

        if (relEc) {
            errorOut = "Failed to compute relative path for: " + childPath.string();
            return false;
        }

        std::error_code symEc;
        bool const isSymlink = std::filesystem::is_symlink(childPath, symEc);
        if (isSymlink) {
            if (syslink) {
                std::error_code readEc;
                std::filesystem::path const target = std::filesystem::read_symlink(childPath, readEc);
                if (readEc) {
                    errorOut = "Failed to read symbolic link target: " + childPath.string();
                    return false;
                }

                if (!std::filesystem::exists(childPath)) {
                    errorOut = "Refusing to archive a broken symbolic link: " + childPath.string();
                    return false;
                }

                JobPendingEntry pe;
                pe.kind          = JobZstdEntryKind::Symlink;
                pe.relativePath  = relPath;
                pe.symlinkTarget = target.string();
                entries.push_back(std::move(pe));
                continue;
            }

            if (!std::filesystem::exists(childPath)) {
                errorOut = "Cannot dereference a broken symbolic link: " + childPath.string();
                return false;
            }

            std::error_code isDirEc;
            if (std::filesystem::is_directory(childPath, isDirEc)) {
                if (!collectEntriesRecursive(root, childPath, preserveEmptyDirectories, syslink, entries, errorOut, activeDirs))
                    return false;
                continue;
            }
            // Falls through: dereferenced regular file, handled by the File case below.
        }

        if (std::filesystem::is_directory(childPath)) {
            std::error_code emptyEc;
            bool const childIsEmpty = std::filesystem::is_empty(childPath, emptyEc) && !emptyEc;

            if (childIsEmpty) {
                if (preserveEmptyDirectories) {
                    JobPendingEntry pe;
                    pe.kind         = JobZstdEntryKind::EmptyDirectory;
                    pe.relativePath = relPath;
                    entries.push_back(std::move(pe));
                }
            } else {
                JobPendingEntry pe;
                pe.kind         = JobZstdEntryKind::Directory;
                pe.relativePath = relPath;
                entries.push_back(std::move(pe));
            }

            if (!collectEntriesRecursive(root, childPath, preserveEmptyDirectories, syslink, entries, errorOut, activeDirs))
                return false;

            continue;
        }

        std::error_code regEc;
        if (!std::filesystem::is_regular_file(childPath, regEc) || regEc) {
            errorOut = "Refusing to archive a non-regular file (FIFO, device, socket, etc.): " + childPath.string();
            return false;
        }

        JobPendingEntry pe;
        pe.kind         = JobZstdEntryKind::File;
        pe.relativePath = relPath;
        pe.sourcePath   = childPath;
        entries.push_back(std::move(pe));
    }

    return true;
}


[[nodiscard]] inline bool collectEntries(
    const std::filesystem::path &root,
    const std::filesystem::path &currentDir,
    bool preserveEmptyDirectories,
    bool syslink,
    std::vector<JobPendingEntry> &entries,
    std::string &errorOut)
{
    std::vector<std::filesystem::path> activeDirs;
    return collectEntriesRecursive(root, currentDir, preserveEmptyDirectories, syslink, entries, errorOut, activeDirs);
}


[[nodiscard]] inline bool flattenEntries(std::vector<JobPendingEntry> &entries, std::string &errorOut)
{
    std::vector<JobPendingEntry> flat;
    flat.reserve(entries.size());

    for (const auto &e : entries) {
        if (e.kind == JobZstdEntryKind::Directory || e.kind == JobZstdEntryKind::EmptyDirectory)
            continue; // A flat archive has no directory structure to speak of.

        JobPendingEntry copy = e;
        copy.relativePath    = e.relativePath.filename();
        flat.push_back(std::move(copy));
    }

    std::sort(flat.begin(), flat.end(), [](const JobPendingEntry &a, const JobPendingEntry &b) {
                  return a.relativePath < b.relativePath;
    });

    for (std::size_t i = 1; i < flat.size(); ++i) {
        if (flat[i].relativePath == flat[i - 1].relativePath) {
            errorOut = "Flattening produced a filename collision: " + flat[i].relativePath.string();
            return false;
        }
    }

    entries = std::move(flat);
    return true;
}

} // namespace job::zstd