#include "job_mmap.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <limits>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace job::io {

//////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////

JobMmap::JobMmap(std::filesystem::path filePath, std::size_t prefetch, bool numa) :
    m_backing(Backing::File),
    m_file(std::move(filePath), JobFile::Access::ReadWrite, JobFile::OpenMode::OpenExisting),
    m_prefetch(prefetch),
    m_numa(numa)
{
    (void)openDevice();
}

JobMmap::JobMmap(int fd, std::size_t prefetch, bool numa, bool fdOwned) :
    m_backing(Backing::File),
    m_file(fd, fdOwned),
    m_prefetch(prefetch),
    m_numa(numa)
{
    (void)openDevice();
}

JobMmap::JobMmap(FILE *fp, std::size_t prefetch, bool numa, bool fpOwned) :
    m_backing(Backing::File),
    m_file(fp, fpOwned),
    m_prefetch(prefetch),
    m_numa(numa)
{
    (void)openDevice();
}

JobMmap::JobMmap(std::size_t size, std::size_t prefetch, bool numa) :
    m_backing(Backing::Anonymous),
    m_mapLength(size),
    m_prefetch(prefetch),
    m_numa(numa)
{
    (void)initAnonymousMap(size);
}

JobMmap::~JobMmap()
{
    reset();
}

JobMmap::JobMmap(JobMmap &&other) noexcept
{
    moveFrom(std::move(other));
}

JobMmap &JobMmap::operator=(JobMmap &&other) noexcept
{
    if (this != &other) {
        reset();
        moveFrom(std::move(other));
    }

    return *this;
}

//////////////////////////////////////////////////////////
// Backing
//////////////////////////////////////////////////////////

JobMmap::Backing JobMmap::backing() const noexcept
{
    return m_backing;
}

bool JobMmap::fileBacked() const noexcept
{
    return m_backing == Backing::File;
}

bool JobMmap::anonymous() const noexcept
{
    return m_backing == Backing::Anonymous;
}

//////////////////////////////////////////////////////////
// IODevice
//////////////////////////////////////////////////////////

bool JobMmap::openDevice()
{
    if (isValid())
        return true;

    switch (m_backing) {
    case Backing::File:
        if (!m_file.isOpen() && !m_file.openDevice())
            return false;

        return initFileMap();

    case Backing::Anonymous:
        if (m_mapLength == 0)
            return false;

        return initAnonymousMap(m_mapLength);

    case Backing::Unknown:
        return false;
    }

    return false;
}

void JobMmap::closeDevice()
{
    unmap();

    if (fileBacked())
        m_file.closeDevice();
}

ssize_t JobMmap::read(char *buffer, std::size_t maxlen)
{
    if (!fileBacked())
        return -1;

    return m_file.read(buffer, maxlen);
}

ssize_t JobMmap::write(const char *data, std::size_t len)
{
    if (!fileBacked())
        return -1;

    return m_file.write(data, len);
}

bool JobMmap::isOpen() const
{
    if (anonymous())
        return isValid();

    if (fileBacked())
        return m_file.isOpen();

    return false;
}

int JobMmap::fd() const
{
    if (!fileBacked())
        return -1;

    return m_file.fd();
}

void JobMmap::setNonBlocking(bool enabled)
{
    if (fileBacked())
        m_file.setNonBlocking(enabled);
}

void JobMmap::setReadCallback(ReadCallback cb)
{
    if (fileBacked())
        m_file.setReadCallback(std::move(cb));
}

bool JobMmap::flush()
{
    if (!fileBacked())
        return true;

    return m_file.flush();
}

IOPermissions JobMmap::permissions() const
{
    if (!fileBacked())
        return m_permissions;

    return m_file.permissions();
}

void JobMmap::setPermissions(IOPermissions perms)
{
    if (!fileBacked()) {
        IODevice::setPermissions(perms);
        return;
    }

    m_file.setPermissions(perms);
}

//////////////////////////////////////////////////////////
// File
//////////////////////////////////////////////////////////

JobFile &JobMmap::file() noexcept
{
    return m_file;
}

const JobFile &JobMmap::file() const noexcept
{
    return m_file;
}

std::size_t JobMmap::fileSize() const
{
    if (!fileBacked())
        return 0;

    return m_file.size();
}

std::size_t JobMmap::tell() const
{
    if (!fileBacked())
        return std::numeric_limits<std::size_t>::max();

    return m_file.tell();
}

bool JobMmap::seek(std::int64_t offset, JobFile::Seek whence)
{
    if (!fileBacked())
        return false;

    return m_file.seek(offset, whence);
}

//////////////////////////////////////////////////////////
// Mapping
//////////////////////////////////////////////////////////

bool JobMmap::isValid() const noexcept
{
    return m_addr != nullptr && !m_mappedRanges.empty();
}

void *JobMmap::addr() noexcept
{
    return m_addr;
}

const void *JobMmap::addr() const noexcept
{
    return m_addr;
}

std::size_t JobMmap::mappedSize() const noexcept
{
    std::size_t total = 0;

    for (const auto &range : m_mappedRanges)
        total += range.size();

    return total;
}

std::size_t JobMmap::mapLength() const noexcept
{
    return m_mapLength;
}

std::size_t JobMmap::pageSize() const noexcept
{
    return m_pageSize;
}

std::size_t JobMmap::alignment() const noexcept
{
    return m_alignment;
}

const JobMmap::Ranges &JobMmap::mappedRanges() const noexcept
{
    return m_mappedRanges;
}

bool JobMmap::unmap(const JobMemRange &range) noexcept
{
    if (!isValid() || range.empty() || !validRange(range))
        return false;

    if (!mappedRange(range))
        return false;

    const JobMemRange aligned = range.alignedInward(m_pageSize);

    // The requested range does not completely contain a native page.
    // Do not unmap bytes outside the user's requested range.
    if (aligned.empty())
        return true;

    if (!mappedRange(aligned))
        return false;

    if (!m_memLock.unlock(aligned))
        return false;

    auto *base = static_cast<std::byte *>(m_addr);

    if (::munmap(base + aligned.first(), aligned.size()) != 0)
        return false;

    subtractRange(m_mappedRanges, aligned);

    return true;
}

void JobMmap::unmap() noexcept
{
    if (m_addr == nullptr)
        return;

    (void)m_memLock.unlock();

    auto *base = static_cast<std::byte *>(m_addr);

    for (const auto &range : m_mappedRanges)
        (void)::munmap(base + range.first(), range.size());

    m_mappedRanges.clear();

    (void)m_memLock.clear();

    m_addr = nullptr;
}

bool JobMmap::grow(std::size_t newSize)
{
    if (!isValid())
        return false;

    if (newSize == m_mapLength)
        return true;

    if (newSize < m_mapLength)
        return false;

    // mremap of a fragmented virtual address range is not meaningful for
    // this abstraction. Grow only a complete contiguous mapping.
    if (m_mappedRanges.size() != 1)
        return false;

    if (m_mappedRanges.front() != JobMemRange{0, m_mapLength})
        return false;

    // Preserve simple and deterministic semantics for now. Growing a locked
    // mapping would require preserving/reapplying lock state after mremap.
    if (m_memLock.isLocked())
        return false;

    if (fileBacked()) {
        if (m_file.access() != JobFile::Access::ReadWrite)
            return false;

        if (::ftruncate(m_file.fd(), static_cast<off_t>(newSize)) != 0)
            return false;
    }

#ifdef __linux__
    void *newAddr = ::mremap(m_addr, m_mapLength, newSize, MREMAP_MAYMOVE);

    if (newAddr == MAP_FAILED) {
        if (fileBacked())
            (void)::ftruncate(m_file.fd(), static_cast<off_t>(m_mapLength));

        return false;
    }
#else
    return false;
#endif

    m_addr = newAddr;
    m_mapLength = newSize;

    m_mappedRanges.clear();
    m_mappedRanges.emplace_back(0, newSize);

    if (!m_memLock.reset(m_addr, m_mapLength, m_pageSize))
        return false;

    if (m_prefetch > 0)
        (void)prefetch(JobMemRange{0, std::min(m_prefetch, m_mapLength)});

    return true;
}

//////////////////////////////////////////////////////////
// Prefetch
//////////////////////////////////////////////////////////

bool JobMmap::prefetch()
{
    if (!isValid())
        return false;

    if (m_prefetch > 0)
        return prefetch(JobMemRange{0, std::min(m_prefetch, m_mapLength)});

    bool success = true;

    for (const auto &range : m_mappedRanges) {
        if (!prefetch(range))
            success = false;
    }

    return success;
}

bool JobMmap::prefetch(const JobMemRange &range)
{
    if (!isValid() || range.empty() || !validRange(range))
        return false;

    if (!mappedRange(range))
        return false;

    JobMemRange aligned = range.alignedOutward(m_pageSize);

    if (aligned.last() > m_mapLength)
        aligned = JobMemRange{aligned.first(), m_mapLength};

    if (aligned.empty() || !mappedRange(aligned))
        return false;

    auto *base = static_cast<std::byte *>(m_addr);

    return ::madvise(base + aligned.first(), aligned.size(), MADV_WILLNEED) == 0;
}

//////////////////////////////////////////////////////////
// NUMA
//////////////////////////////////////////////////////////

bool JobMmap::numa() const noexcept
{
    return m_numa;
}

void JobMmap::setNuma(bool enabled) noexcept
{
    m_numa = enabled;
}

//////////////////////////////////////////////////////////
// Memory locking
//////////////////////////////////////////////////////////

bool JobMmap::lock()
{
    if (!isValid())
        return false;

    // A fragmented mapping contains unmapped holes. JobMemLock::lock()
    // operates over its complete memory domain, so only use whole-map lock
    // when the complete mapping still exists.
    if (m_mappedRanges.size() != 1)
        return false;

    if (m_mappedRanges.front() != JobMemRange{0, m_mapLength})
        return false;

    return m_memLock.lock();
}

bool JobMmap::lock(const JobMemRange &range)
{
    if (!isValid() || !mappedRange(range))
        return false;

    return m_memLock.lock(range);
}

bool JobMmap::unlock()
{
    return m_memLock.unlock();
}

bool JobMmap::unlock(const JobMemRange &range)
{
    return m_memLock.unlock(range);
}

std::size_t JobMmap::lockedSize() const noexcept
{
    return m_memLock.lockedSize();
}

const JobMmap::Ranges &JobMmap::lockedRanges() const noexcept
{
    return m_memLock.lockedRanges();
}

JobMemLock &JobMmap::memLock() noexcept
{
    return m_memLock;
}

const JobMemLock &JobMmap::memLock() const noexcept
{
    return m_memLock;
}

//////////////////////////////////////////////////////////
// Mapping management
//////////////////////////////////////////////////////////

bool JobMmap::initFileMap()
{
    if (!fileBacked() || !m_file.isOpen())
        return false;

    const std::size_t size = m_file.size();

    if (size == 0)
        return false;

    const long nativePageSize = ::sysconf(_SC_PAGESIZE);

    if (nativePageSize <= 0)
        return false;

    m_pageSize = static_cast<std::size_t>(nativePageSize);
    m_alignment = m_pageSize;

    int protection = 0;

    switch (m_file.access()) {
    case JobFile::Access::ReadOnly:
        protection = PROT_READ;
        break;

    case JobFile::Access::WriteOnly:
        return false;

    case JobFile::Access::ReadWrite:
        protection = PROT_READ | PROT_WRITE;
        break;
    }

    void *mapped = ::mmap(nullptr, size, protection, MAP_SHARED, m_file.fd(), 0);

    if (mapped == MAP_FAILED)
        return false;

    m_addr = mapped;
    m_mapLength = size;

    m_mappedRanges.clear();
    m_mappedRanges.emplace_back(0, size);

    if (!m_memLock.reset(m_addr, m_mapLength, m_pageSize)) {
        (void)::munmap(m_addr, m_mapLength);
        m_addr = nullptr;
        m_mappedRanges.clear();
        return false;
    }

    if (m_prefetch > 0)
        (void)prefetch(JobMemRange{0, std::min(m_prefetch, m_mapLength)});

    return true;
}

bool JobMmap::initAnonymousMap(std::size_t size)
{
    const long nativePageSize = ::sysconf(_SC_PAGESIZE);

    if (nativePageSize <= 0)
        return false;

    m_pageSize = static_cast<std::size_t>(nativePageSize);
    m_alignment = m_pageSize;

    void *mapped = ::mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);

    if (mapped == MAP_FAILED)
        return false;

    m_addr = mapped;
    m_mapLength = size;

    m_mappedRanges.clear();
    m_mappedRanges.emplace_back(0, size);

    if (!m_memLock.reset(m_addr, m_mapLength, m_pageSize)) {
        (void)::munmap(m_addr, m_mapLength);
        m_addr = nullptr;
        m_mappedRanges.clear();
        return false;
    }

    if (m_prefetch > 0)
        (void)prefetch(JobMemRange{0, std::min(m_prefetch, m_mapLength)});

    return true;
}

void JobMmap::destroyMap() noexcept
{
    unmap();
}

bool JobMmap::validRange(const JobMemRange &range) const noexcept
{
    if (range.empty())
        return false;

    return range.last() <= m_mapLength;
}

bool JobMmap::mappedRange(const JobMemRange &range) const noexcept
{
    if (!validRange(range))
        return false;

    for (const auto &mapped : m_mappedRanges) {
        if (mapped.contains(range))
            return true;
    }

    return false;
}

void JobMmap::subtractRange(Ranges &ranges, const JobMemRange &range)
{
    if (range.empty() || ranges.empty())
        return;

    Ranges remaining;
    remaining.reserve(ranges.size() + 1);

    for (const auto &mapped : ranges) {
        if (!mapped.overlaps(range)) {
            remaining.push_back(mapped);
            continue;
        }

        if (mapped.first() < range.first())
            remaining.emplace_back(mapped.first(), std::min(mapped.last(), range.first()));

        if (range.last() < mapped.last())
            remaining.emplace_back(std::max(mapped.first(), range.last()), mapped.last());
    }

    ranges = std::move(remaining);
    normalizeRanges(ranges);
}

void JobMmap::addRange(Ranges &ranges, const JobMemRange &range)
{
    if (range.empty())
        return;

    ranges.push_back(range);
    normalizeRanges(ranges);
}

void JobMmap::normalizeRanges(Ranges &ranges)
{
    if (ranges.size() < 2)
        return;

    std::sort(ranges.begin(), ranges.end(), [](const JobMemRange &a, const JobMemRange &b) {
        if (a.first() != b.first())
            return a.first() < b.first();

        return a.last() < b.last();
    });

    Ranges normalized;
    normalized.reserve(ranges.size());

    normalized.push_back(ranges.front());

    for (std::size_t i = 1; i < ranges.size(); ++i) {
        const JobMemRange &current = ranges[i];
        JobMemRange &last = normalized.back();

        if (last.mergeable(current))
            last = last.merged(current);
        else
            normalized.push_back(current);
    }

    ranges = std::move(normalized);
}

//////////////////////////////////////////////////////////
// Shared cleanup / move
//////////////////////////////////////////////////////////

void JobMmap::reset() noexcept
{
    destroyMap();
    m_file.closeDevice();

    m_backing = Backing::Unknown;

    m_addr = nullptr;

    m_mapLength = 0;
    m_pageSize = 0;
    m_alignment = 1;
    m_prefetch = 0;

    m_numa = false;

    m_mappedRanges.clear();

    m_permissions = IOPermissions::DefaultFile;
    m_permissionsCallback = nullptr;
}

void JobMmap::moveFrom(JobMmap &&other) noexcept
{
    m_backing = other.m_backing;

    m_file = std::move(other.m_file);
    m_memLock = std::move(other.m_memLock);

    m_addr = other.m_addr;

    m_mapLength = other.m_mapLength;
    m_pageSize = other.m_pageSize;
    m_alignment = other.m_alignment;
    m_prefetch = other.m_prefetch;

    m_numa = other.m_numa;

    m_mappedRanges = std::move(other.m_mappedRanges);

    m_permissions = other.m_permissions;
    m_permissionsCallback = other.m_permissionsCallback;

    other.m_backing = Backing::Unknown;

    other.m_addr = nullptr;

    other.m_mapLength = 0;
    other.m_pageSize = 0;
    other.m_alignment = 1;
    other.m_prefetch = 0;

    other.m_numa = false;

    other.m_mappedRanges.clear();

    other.m_permissions = IOPermissions::DefaultFile;
    other.m_permissionsCallback = nullptr;
}

} // namespace job::io