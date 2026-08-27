#include "job_mem_lock.h"

#include <algorithm>
#include <cstddef>
#include <sys/mman.h>
#include <utility>

namespace job::io {

JobMemLock::JobMemLock(void *addr, std::size_t size, std::size_t pageSize) :
    m_addr(addr),
    m_size(size),
    m_pageSize(pageSize)
{
}

JobMemLock::~JobMemLock()
{
    (void)release();
}

JobMemLock::JobMemLock(JobMemLock &&other) noexcept
{
    moveFrom(std::move(other));
}

JobMemLock &JobMemLock::operator=(JobMemLock &&other) noexcept
{
    if (this != &other) {
        if (!release())
            return *this;

        moveFrom(std::move(other));
    }

    return *this;
}

bool JobMemLock::reset(void *addr, std::size_t size, std::size_t pageSize)
{
    if (!release())
        return false;

    m_addr = addr;
    m_size = size;
    m_pageSize = pageSize;

    return true;
}

bool JobMemLock::clear() noexcept
{
    if (!release())
        return false;

    m_addr = nullptr;
    m_size = 0;
    m_pageSize = 0;

    return true;
}

void *JobMemLock::addr() noexcept
{
    return m_addr;
}

const void *JobMemLock::addr() const noexcept
{
    return m_addr;
}

std::size_t JobMemLock::size() const noexcept
{
    return m_size;
}

std::size_t JobMemLock::pageSize() const noexcept
{
    return m_pageSize;
}

bool JobMemLock::valid() const noexcept
{
    return m_addr != nullptr &&
           m_size > 0 &&
           m_pageSize > 0 &&
           JobMemRange::validAlignment(m_pageSize);
}

bool JobMemLock::lock()
{
    if (!valid())
        return false;

    return lock(JobMemRange::fromSize(0, m_size));
}

bool JobMemLock::lock(const JobMemRange &range)
{
    if (!valid() || range.empty() || range.last() > m_size)
        return false;

    const JobMemRange aligned = alignedRange(range);

    if (aligned.empty())
        return false;

    if (isLocked(aligned))
        return true;

    // Reserve before mlock() so bookkeeping cannot require a vector allocation after the OS state has already changed.
    m_lockedRanges.reserve(m_lockedRanges.size() + 1);

    auto *base = static_cast<std::byte *>(m_addr);

    if (::mlock(base + aligned.first(), aligned.size()) != 0)
        return false;

    return addLockedRange(aligned);
}

bool JobMemLock::unlock()
{
    if (!valid())
        return false;

    return release();
}

bool JobMemLock::unlock(const JobMemRange &range)
{
    if (!valid() || range.empty() || range.last() > m_size)
        return false;

    const JobMemRange aligned = alignedRange(range);

    if (aligned.empty())
        return false;

    bool overlapsLockedRange = false;

    for (const auto &locked : m_lockedRanges) {
        if (locked.overlaps(aligned)) {
            overlapsLockedRange = true;
            break;
        }
    }

    if (!overlapsLockedRange)
        return true;

    auto *base = static_cast<std::byte *>(m_addr);

    if (::munlock(base + aligned.first(), aligned.size()) != 0)
        return false;

    removeLockedRange(aligned);

    return true;
}

bool JobMemLock::isLocked() const noexcept
{
    return !m_lockedRanges.empty();
}

bool JobMemLock::isLocked(std::size_t offset) const noexcept
{
    if (!valid() || offset >= m_size)
        return false;

    for (const auto &range : m_lockedRanges) {
        if (range.contains(offset))
            return true;
    }

    return false;
}

bool JobMemLock::isLocked(const JobMemRange &range) const noexcept
{
    if (!valid() || range.empty() || range.last() > m_size)
        return false;

    for (const auto &locked : m_lockedRanges) {
        if (locked.contains(range))
            return true;
    }

    return false;
}

std::size_t JobMemLock::lockedSize() const noexcept
{
    std::size_t total = 0;

    for (const auto &range : m_lockedRanges)
        total += range.size();

    return total;
}

const JobMemLock::Ranges &JobMemLock::lockedRanges() const noexcept
{
    return m_lockedRanges;
}

JobMemRange JobMemLock::alignedRange(const JobMemRange &range) const
{
    const JobMemRange aligned = range.alignedOutward(m_pageSize);

    // mmap() and mlock() operate on native pages, but JobMemLock tracks
    // offsets only inside its supplied memory domain.
    return JobMemRange{
        aligned.first(),
        std::min(aligned.last(), m_size)
    };
}

bool JobMemLock::addLockedRange(const JobMemRange &range)
{
    if (range.empty())
        return false;

    JobMemRange merged = range;

    auto it = m_lockedRanges.begin();

    while (it != m_lockedRanges.end()) {
        if (it->last() < merged.first()) {
            ++it;
            continue;
        }

        if (merged.last() < it->first())
            break;

        if (it->mergeable(merged)) {
            merged = merged.merged(*it);
            it = m_lockedRanges.erase(it);
            continue;
        }

        ++it;
    }

    m_lockedRanges.insert(it, merged);

    return true;
}

void JobMemLock::removeLockedRange(const JobMemRange &range)
{
    if (range.empty() || m_lockedRanges.empty())
        return;

    Ranges remaining;
    remaining.reserve(m_lockedRanges.size() + 1);

    for (const auto &locked : m_lockedRanges) {
        if (!locked.overlaps(range)) {
            remaining.push_back(locked);
            continue;
        }

        if (locked.first() < range.first()) {
            remaining.emplace_back(
                locked.first(),
                std::min(range.first(), locked.last()));
        }

        if (range.last() < locked.last()) {
            remaining.emplace_back(
                std::max(range.last(), locked.first()),
                locked.last());
        }
    }

    m_lockedRanges = std::move(remaining);
}

bool JobMemLock::release() noexcept
{
    if (m_lockedRanges.empty())
        return true;

    if (!valid())
        return false;

    auto *base = static_cast<std::byte *>(m_addr);

    bool success = true;
    std::size_t index = 0;

    while (index < m_lockedRanges.size()) {
        const JobMemRange range = m_lockedRanges[index];

        if (::munlock(base + range.first(), range.size()) == 0) {
            m_lockedRanges.erase(m_lockedRanges.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        success = false;
        ++index;
    }

    return success;
}

void JobMemLock::moveFrom(JobMemLock &&other) noexcept
{
    m_addr = other.m_addr;
    m_size = other.m_size;
    m_pageSize = other.m_pageSize;
    m_lockedRanges = std::move(other.m_lockedRanges);

    other.m_addr = nullptr;
    other.m_size = 0;
    other.m_pageSize = 0;
    other.m_lockedRanges.clear();
}

} // namespace job::io