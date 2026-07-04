#pragma once
#include <vector>
#include <filesystem>
namespace job::zstd {
class JobZstdDirGuard
{
public:
    JobZstdDirGuard(std::vector<std::filesystem::path> &stack, std::filesystem::path canonicalDir) : m_stack(stack)
    {
        m_stack.push_back(std::move(canonicalDir));
    }

    ~JobZstdDirGuard()
    {
        m_stack.pop_back();
    }

    JobZstdDirGuard(const JobZstdDirGuard &) = delete;
    JobZstdDirGuard &operator=(const JobZstdDirGuard &) = delete;

private:
    std::vector<std::filesystem::path> &m_stack;
};
}
