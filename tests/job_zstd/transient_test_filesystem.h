#pragma once

#include <filesystem>
#include <string>
#include <cstdint>

namespace job::zstd::test {

class TransientTestFilesystem
{
public:
    TransientTestFilesystem();
    ~TransientTestFilesystem();

    TransientTestFilesystem(const TransientTestFilesystem &) = delete;
    TransientTestFilesystem &operator=(const TransientTestFilesystem &) = delete;

    [[nodiscard]] const std::filesystem::path &root() const noexcept;

    std::filesystem::path makeFile(const std::filesystem::path &relPath, const std::string &content);
    std::filesystem::path makeDir(const std::filesystem::path &relPath);
    std::filesystem::path makeSymlink(const std::filesystem::path &target, const std::filesystem::path &relLinkPath);
    std::filesystem::path makeDirSymlink(const std::filesystem::path &targetDir, const std::filesystem::path &relLinkPath);
    std::filesystem::path makeSymlinkCycle(const std::filesystem::path &relLinkPath, const std::filesystem::path &relTargetDir);

#ifdef __unix__
    std::filesystem::path makeFifo(const std::filesystem::path &relPath);
#endif

private:
    std::filesystem::path m_root;
};

[[nodiscard]] bool compareTrees(const std::filesystem::path &lhs, const std::filesystem::path &rhs, std::string &errorOut);

} // namespace job::zstd::test