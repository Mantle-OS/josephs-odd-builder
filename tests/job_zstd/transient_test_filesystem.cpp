// transient_test_filesystem.cpp
#include "transient_test_filesystem.h"

#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>
#include <atomic>
#include <chrono>

#ifdef __unix__
#include <sys/stat.h>
#endif

#include <job_hash.h>

namespace job::zstd::test {

namespace {

[[nodiscard]] std::filesystem::path uniqueScratchName()
{
    static std::atomic<std::uint64_t> counter{0};
    std::uint64_t const id = counter.fetch_add(1, std::memory_order_relaxed);

    std::ostringstream name;
    name << "job_zstd_test_" << id << "_" << std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::path(name.str());
}

// One recursive pass: walks sourceDir's children and checks each one has a
// matching counterpart under otherRoot. compareTrees() calls this twice
// (lhs-vs-rhs, then rhs-vs-lhs) so an entry missing from either side gets
// caught regardless of which side it's missing from.
void compareDirectionally(
    const std::filesystem::path &sourceRoot,
    const std::filesystem::path &sourceDir,
    const std::filesystem::path &otherRoot,
    std::vector<std::string> &mismatches)
{
    std::error_code dirEc;
    std::filesystem::directory_iterator it(sourceDir, dirEc);
    if (dirEc) {
        mismatches.push_back("Failed to open directory during comparison: " + sourceDir.string());
        return;
    }

    for (const auto &entry : it) {
        // std::filesystem::path const relPath   = std::filesystem::relative(entry.path(), sourceRoot);
        std::filesystem::path const relPath = entry.path().lexically_relative(sourceRoot); // was: std::filesystem::relative(entry.path(), sourceRoot)

        std::filesystem::path const otherPath = otherRoot / relPath;

        std::error_code existsEc;
        if (!std::filesystem::exists(otherPath, existsEc)) {
            mismatches.push_back("Entry present on one side only: " + relPath.string());
            continue;
        }

        std::error_code symEc;
        bool const sourceIsSymlink = std::filesystem::is_symlink(entry.path(), symEc);
        bool const otherIsSymlink  = std::filesystem::is_symlink(otherPath, symEc);

        if (sourceIsSymlink != otherIsSymlink) {
            mismatches.push_back("Symlink-ness differs for: " + relPath.string());
            continue;
        }

        if (sourceIsSymlink) {
            std::error_code readEc;
            std::filesystem::path const sourceTarget = std::filesystem::read_symlink(entry.path(), readEc);
            std::filesystem::path const otherTarget  = std::filesystem::read_symlink(otherPath, readEc);

            if (sourceTarget != otherTarget)
                mismatches.push_back("Symlink target differs for: " + relPath.string());

            continue;
        }

        std::error_code isDirEc;
        bool const sourceIsDir = std::filesystem::is_directory(entry.path(), isDirEc);

        if (sourceIsDir) {
            compareDirectionally(sourceRoot, entry.path(), otherRoot, mismatches);
            continue;
        }

        // BLAKE2b via the already-linked JosephsOddBuilder_Crypto, streamed
        // in 1MB chunks -- avoids pulling two full files into memory just
        // to compare them, which matters once real fixtures start looking
        // like actual package payloads instead of a few bytes of text.
        std::vector<unsigned char> const sourceHash = job::crypto::JobHash::hashFile(entry.path().string());
        std::vector<unsigned char> const otherHash  = job::crypto::JobHash::hashFile(otherPath.string());

        if (sourceHash.empty() || otherHash.empty()) {
            mismatches.push_back("Failed to hash file for comparison: " + relPath.string());
            continue;
        }

        if (sourceHash != otherHash)
            mismatches.push_back("File content differs for: " + relPath.string());
    }
}

} // namespace

TransientTestFilesystem::TransientTestFilesystem() : m_root(std::filesystem::temp_directory_path() / uniqueScratchName())
{
    std::filesystem::create_directories(m_root);
}

TransientTestFilesystem::~TransientTestFilesystem()
{
    std::error_code ec;
    std::filesystem::remove_all(m_root, ec);
}

const std::filesystem::path &TransientTestFilesystem::root() const noexcept
{
    return m_root;
}

std::filesystem::path TransientTestFilesystem::makeFile(const std::filesystem::path &relPath, const std::string &content)
{
    std::filesystem::path const fullPath = m_root / relPath;
    std::filesystem::create_directories(fullPath.parent_path());

    std::ofstream out(fullPath, std::ios::binary);
    out << content;

    return fullPath;
}

std::filesystem::path TransientTestFilesystem::makeDir(const std::filesystem::path &relPath)
{
    std::filesystem::path const fullPath = m_root / relPath;
    std::filesystem::create_directories(fullPath);
    return fullPath;
}

std::filesystem::path TransientTestFilesystem::makeSymlink(const std::filesystem::path &target, const std::filesystem::path &relLinkPath)
{
    std::filesystem::path const fullLinkPath = m_root / relLinkPath;
    std::filesystem::create_directories(fullLinkPath.parent_path());

    std::error_code linkEc;
    std::filesystem::create_symlink(target, fullLinkPath, linkEc);

    return fullLinkPath;
}

std::filesystem::path TransientTestFilesystem::makeDirSymlink(const std::filesystem::path &targetDir, const std::filesystem::path &relLinkPath)
{
    std::filesystem::path const fullLinkPath = m_root / relLinkPath;
    std::filesystem::create_directories(fullLinkPath.parent_path());

    std::error_code linkEc;
    std::filesystem::create_directory_symlink(targetDir, fullLinkPath, linkEc);

    return fullLinkPath;
}

std::filesystem::path TransientTestFilesystem::makeSymlinkCycle(const std::filesystem::path &relLinkPath, const std::filesystem::path &relTargetDir)
{
    std::filesystem::path const targetFullPath = makeDir(relTargetDir);
    return makeDirSymlink(targetFullPath, relLinkPath);
}

#ifdef __unix__
std::filesystem::path TransientTestFilesystem::makeFifo(const std::filesystem::path &relPath)
{
    std::filesystem::path const fullPath = m_root / relPath;
    std::filesystem::create_directories(fullPath.parent_path());

    ::mkfifo(fullPath.c_str(), 0666);

    return fullPath;
}
#endif

bool compareTrees(const std::filesystem::path &lhs, const std::filesystem::path &rhs, std::string &errorOut)
{
    std::vector<std::string> mismatches;

    compareDirectionally(lhs, lhs, rhs, mismatches);
    compareDirectionally(rhs, rhs, lhs, mismatches);

    if (mismatches.empty())
        return true;

    std::ostringstream combined;
    for (const auto &m : mismatches)
        combined << m << "; ";

    errorOut = combined.str();
    return false;
}

} // namespace job::zstd::test