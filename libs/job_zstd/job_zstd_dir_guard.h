#pragma once

#include <vector>
#include <filesystem>
#include <utility>

#include "jobzstd_export.h"

namespace job::zstd {
class JOBZSTD_EXPORT JobZstdDirGuard
{
public:
    JobZstdDirGuard(std::vector<std::filesystem::path> &stack, std::filesystem::path canonicalDir) :
        m_stack(stack)
    {
        m_stack.push_back(std::move(canonicalDir));
    }

    ~JobZstdDirGuard()
    {
        m_stack.pop_back();
    }

    JobZstdDirGuard(JobZstdDirGuard &&) = delete;
    JobZstdDirGuard &operator=(JobZstdDirGuard &&) = delete;

    JobZstdDirGuard(const JobZstdDirGuard &) = delete;
    JobZstdDirGuard &operator=(const JobZstdDirGuard &) = delete;

private:
    std::vector<std::filesystem::path> &m_stack;
};
}
