#pragma once

#include <cstddef>
#include <deque>
#include <memory>

#include "job_present_entry.h"

namespace job::render {

class JobPresentQueue
{
public:
    using Ptr  = std::shared_ptr<JobPresentQueue>;
    using WPtr = std::weak_ptr<JobPresentQueue>;
    using UPtr = std::unique_ptr<JobPresentQueue>;

    using Entry = JobPresentEntry;
    using Entries = std::deque<Entry>;

    JobPresentQueue() = default;
    ~JobPresentQueue() = default;

    JobPresentQueue(const JobPresentQueue &) = delete;
    JobPresentQueue &operator=(const JobPresentQueue &) = delete;

    JobPresentQueue(JobPresentQueue &&) noexcept = default;
    JobPresentQueue &operator=(JobPresentQueue &&) noexcept = default;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobPresentQueue>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobPresentQueue>();
    }

    [[nodiscard]] bool push(Entry entry);

    [[nodiscard]] Entry *front() noexcept
    {
        if (m_entries.empty())
            return nullptr;

        return &m_entries.front();
    }

    [[nodiscard]] const Entry *front() const noexcept
    {
        if (m_entries.empty())
            return nullptr;

        return &m_entries.front();
    }

    [[nodiscard]] Entry *back() noexcept
    {
        if (m_entries.empty())
            return nullptr;

        return &m_entries.back();
    }

    [[nodiscard]] const Entry *back() const noexcept
    {
        if (m_entries.empty())
            return nullptr;

        return &m_entries.back();
    }

    [[nodiscard]] Entry pop() noexcept;

    void clear() noexcept
    {
        m_entries.clear();
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_entries.empty();
    }

    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_entries.size();
    }

private:
    Entries m_entries;
};

}