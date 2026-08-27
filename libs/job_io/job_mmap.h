#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "io_base.h"
#include "job_file.h"
#include "job_mem_lock.h"
#include "job_mem_range.h"

namespace job::io {

class JobMmap final : public IODevice
{
public:
    using Ptr  = std::shared_ptr<JobMmap>;
    using WPtr = std::weak_ptr<JobMmap>;
    using UPtr = std::unique_ptr<JobMmap>;

    using Ranges = std::vector<JobMemRange>;

    enum class Backing : std::uint8_t
    {
        Unknown = 0,
        File,
        Anonymous
    };

    //////////////////////////////////////////////////////////
    // Construction
    //////////////////////////////////////////////////////////

    explicit JobMmap(std::filesystem::path filePath, std::size_t prefetch = 0, bool numa = false);

    JobMmap(int fd, std::size_t prefetch = 0, bool numa = false, bool fdOwned = false)
        pre(fd >= 0);

    JobMmap(FILE *fp, std::size_t prefetch = 0, bool numa = false, bool fpOwned = false)
        pre(fp != nullptr);

    explicit JobMmap(std::size_t size, std::size_t prefetch = 0, bool numa = false)
        pre(size > 0);

    ~JobMmap() override;

    JobMmap(const JobMmap &) = delete;
    JobMmap &operator=(const JobMmap &) = delete;
    JobMmap(JobMmap &&other) noexcept;
    JobMmap &operator=(JobMmap &&other) noexcept;

    //////////////////////////////////////////////////////////
    // Factories
    //////////////////////////////////////////////////////////

    [[nodiscard]] static Ptr createShared(std::filesystem::path filePath, std::size_t prefetch = 0, bool numa = false)
    {
        return std::make_shared<JobMmap>(std::move(filePath), prefetch, numa);
    }

    [[nodiscard]] static Ptr createShared(int fd, std::size_t prefetch = 0, bool numa = false, bool fdOwned = false)
        pre(fd >= 0)
    {
        return std::make_shared<JobMmap>(fd, prefetch, numa, fdOwned);
    }

    [[nodiscard]] static Ptr createShared(FILE *fp, std::size_t prefetch = 0, bool numa = false, bool fpOwned = false)
        pre(fp != nullptr)
    {
        return std::make_shared<JobMmap>(fp, prefetch, numa, fpOwned);
    }

    [[nodiscard]] static Ptr createShared(std::size_t size, std::size_t prefetch = 0, bool numa = false)
        pre(size > 0)
    {
        return std::make_shared<JobMmap>(size, prefetch, numa);
    }

    [[nodiscard]] static UPtr createUniq(std::filesystem::path filePath, std::size_t prefetch = 0, bool numa = false)
    {
        return std::make_unique<JobMmap>(std::move(filePath), prefetch, numa);
    }

    [[nodiscard]] static UPtr createUniq(int fd, std::size_t prefetch = 0, bool numa = false, bool fdOwned = false)
        pre(fd >= 0)
    {
        return std::make_unique<JobMmap>(fd, prefetch, numa, fdOwned);
    }

    [[nodiscard]] static UPtr createUniq(FILE *fp, std::size_t prefetch = 0, bool numa = false, bool fpOwned = false)
        pre(fp != nullptr)
    {
        return std::make_unique<JobMmap>(fp, prefetch, numa, fpOwned);
    }

    [[nodiscard]] static UPtr createUniq(std::size_t size, std::size_t prefetch = 0, bool numa = false)
        pre(size > 0)
    {
        return std::make_unique<JobMmap>(size, prefetch, numa);
    }

    //////////////////////////////////////////////////////////
    // Backing
    //////////////////////////////////////////////////////////

    [[nodiscard]] Backing backing() const noexcept;
    [[nodiscard]] bool fileBacked() const noexcept;
    [[nodiscard]] bool anonymous() const noexcept;

    //////////////////////////////////////////////////////////
    // IODevice
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] bool isOpen() const override;
    [[nodiscard]] int fd() const override;

    void setNonBlocking(bool enabled) override;
    void setReadCallback(ReadCallback cb) override;

    [[nodiscard]] bool flush() override;

    [[nodiscard]] IOPermissions permissions() const override;
    void setPermissions(IOPermissions perms) override;

    //////////////////////////////////////////////////////////
    // File
    //////////////////////////////////////////////////////////

    [[nodiscard]] JobFile &file() noexcept;
    [[nodiscard]] const JobFile &file() const noexcept;

    [[nodiscard]] std::size_t fileSize() const;
    [[nodiscard]] std::size_t tell() const;

    [[nodiscard]] bool seek(std::int64_t offset, JobFile::Seek whence);

    //////////////////////////////////////////////////////////
    // Mapping
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] void *addr() noexcept;
    [[nodiscard]] const void *addr() const noexcept;

    [[nodiscard]] std::size_t mappedSize() const noexcept;
    [[nodiscard]] std::size_t mapLength() const noexcept;
    [[nodiscard]] std::size_t pageSize() const noexcept;
    [[nodiscard]] std::size_t alignment() const noexcept;

    [[nodiscard]] const Ranges &mappedRanges() const noexcept;

    [[nodiscard]] bool unmap(const JobMemRange &range) noexcept;

    [[nodiscard]] bool unmap(std::size_t first, std::size_t last) noexcept
        pre(first <= last)
    {
        return unmap(JobMemRange(first, last));
    }

    void unmap() noexcept;

    [[nodiscard]] bool grow(std::size_t newSize)
        pre(newSize > 0);

    //////////////////////////////////////////////////////////
    // Prefetch
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool prefetch();

    [[nodiscard]] bool prefetch(const JobMemRange &range);

    [[nodiscard]] bool prefetch(std::size_t first, std::size_t last)
        pre(first <= last)
    {
        return prefetch(JobMemRange(first, last));
    }

    //////////////////////////////////////////////////////////
    // NUMA
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool numa() const noexcept;
    void setNuma(bool enabled) noexcept;

    //////////////////////////////////////////////////////////
    // Memory locking
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool lock();
    [[nodiscard]] bool lock(const JobMemRange &range);

    [[nodiscard]] bool lock(std::size_t first, std::size_t last)
        pre(first <= last)
    {
        return lock(JobMemRange(first, last));
    }

    [[nodiscard]] bool unlock();
    [[nodiscard]] bool unlock(const JobMemRange &range);

    [[nodiscard]] bool unlock(std::size_t first, std::size_t last)
        pre(first <= last)
    {
        return unlock(JobMemRange(first, last));
    }

    [[nodiscard]] std::size_t lockedSize() const noexcept;
    [[nodiscard]] const Ranges &lockedRanges() const noexcept;

    [[nodiscard]] JobMemLock &memLock() noexcept;
    [[nodiscard]] const JobMemLock &memLock() const noexcept;

private:
    //////////////////////////////////////////////////////////
    // Mapping management
    //////////////////////////////////////////////////////////

    [[nodiscard]] bool initFileMap();
    [[nodiscard]] bool initAnonymousMap(std::size_t size)
        pre(size > 0);

    void destroyMap() noexcept;

    [[nodiscard]] bool validRange(const JobMemRange &range) const noexcept;
    [[nodiscard]] bool mappedRange(const JobMemRange &range) const noexcept;

    static void subtractRange(Ranges &ranges, const JobMemRange &range);
    static void addRange(Ranges &ranges, const JobMemRange &range);
    static void normalizeRanges(Ranges &ranges);

    //////////////////////////////////////////////////////////
    // Shared cleanup / move
    //////////////////////////////////////////////////////////

    void reset() noexcept;
    void moveFrom(JobMmap &&other) noexcept;

    Backing m_backing{Backing::Unknown};

    JobFile m_file;
    JobMemLock m_memLock;

    void *m_addr{nullptr};

    std::size_t m_mapLength{0};
    std::size_t m_pageSize{0};
    std::size_t m_alignment{1};
    std::size_t m_prefetch{0};

    bool m_numa{false};

    Ranges m_mappedRanges;
};

} // namespace job::io