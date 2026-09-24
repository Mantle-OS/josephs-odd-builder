#pragma once

#include <cstdint>
#include <memory>

#include "job_render_fence.h"

namespace job::render {

enum class DrmSyncobjType : std::uint8_t
{
    Binary,
    Timeline
};

class JobDrmSyncobj
{
public:
    using Ptr  = std::shared_ptr<JobDrmSyncobj>;
    using WPtr = std::weak_ptr<JobDrmSyncobj>;
    using UPtr = std::unique_ptr<JobDrmSyncobj>;

    using Handle        = std::uint32_t;
    using Point         = std::uint64_t;
    using Flags         = std::uint32_t;
    using DeadlineNs    = std::int64_t;
    using NativeHandle  = int;

    JobDrmSyncobj() = default;
    ~JobDrmSyncobj();

    JobDrmSyncobj(const JobDrmSyncobj &) = delete;
    JobDrmSyncobj &operator=(const JobDrmSyncobj &) = delete;

    JobDrmSyncobj(JobDrmSyncobj &&other) noexcept;
    JobDrmSyncobj &operator=(JobDrmSyncobj &&other) noexcept;

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobDrmSyncobj>();
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobDrmSyncobj>();
    }

    [[nodiscard]] bool create(
        int drmFd,
        DrmSyncobjType type,
        bool signaled = false);

    [[nodiscard]] bool importSyncobjFd(int drmFd, DrmSyncobjType type, NativeHandle fd);

    void reset() noexcept;

    [[nodiscard]] bool signal() noexcept;

    [[nodiscard]] bool resetFence() noexcept;

    [[nodiscard]] bool wait(DeadlineNs deadlineNs, Flags flags = 0) const noexcept;

    [[nodiscard]] bool signal(Point point) noexcept;

    [[nodiscard]] bool wait(Point point, DeadlineNs deadlineNs, Flags flags = 0) const noexcept;

    [[nodiscard]] bool query(Point &point) const noexcept;

    [[nodiscard]] bool transfer(JobDrmSyncobj &destination, Point sourcePoint, Point destinationPoint, Flags flags = 0) const noexcept;

    [[nodiscard]] bool importSyncFile(const JobRenderFence &fence);

    [[nodiscard]] bool importSyncFile(const JobRenderFence &fence, Point point);

    [[nodiscard]] JobRenderFence exportSyncFile() const;

    [[nodiscard]] JobRenderFence exportSyncFile(Point point) const;

    [[nodiscard]] NativeHandle exportSyncobjFd() const noexcept;

    [[nodiscard]] constexpr Handle handle() const noexcept
    {
        return m_handle;
    }

    [[nodiscard]] constexpr int drmFd() const noexcept
    {
        return m_drmFd;
    }

    [[nodiscard]] constexpr DrmSyncobjType type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr bool binary() const noexcept
    {
        return m_type == DrmSyncobjType::Binary;
    }

    [[nodiscard]] constexpr bool timeline() const noexcept
    {
        return m_type == DrmSyncobjType::Timeline;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return m_drmFd >= 0 && m_handle != 0;
    }

private:
    [[nodiscard]] bool createTemporaryBinary(JobDrmSyncobj &syncobj) const;

    int             m_drmFd{-1}; // BORROWED
    Handle          m_handle{};
    DrmSyncobjType  m_type{DrmSyncobjType::Binary};
};

}