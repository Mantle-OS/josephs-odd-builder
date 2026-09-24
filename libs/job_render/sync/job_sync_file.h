#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "job_render_fence.h"

namespace job::render {

class JobSyncFile
{
public:
    using Ptr  = std::shared_ptr<JobSyncFile>;
    using WPtr = std::weak_ptr<JobSyncFile>;
    using UPtr = std::unique_ptr<JobSyncFile>;

    using Status        = std::int32_t;
    using Timestamp     = std::uint64_t;

    struct FenceInfo
    {
        std::string     objectName;
        std::string     driverName;
        Status          status{};
        std::uint32_t   flags{};
        Timestamp       timestampNs{};

        [[nodiscard]] constexpr bool active() const noexcept
        {
            return status == 0;
        }

        [[nodiscard]] constexpr bool signaled() const noexcept
        {
            return status == 1;
        }

        [[nodiscard]] constexpr bool error() const noexcept
        {
            return status < 0;
        }

        bool operator==(const FenceInfo &) const noexcept = default;
    };

    using FenceInfos = std::vector<FenceInfo>;

    struct Info
    {
        std::string name;

        Status status{};
        std::uint32_t flags{};

        FenceInfos fences;

        [[nodiscard]] constexpr bool active() const noexcept
        {
            return status == 0;
        }

        [[nodiscard]] constexpr bool signaled() const noexcept
        {
            return status == 1;
        }

        [[nodiscard]] constexpr bool error() const noexcept
        {
            return status < 0;
        }

        [[nodiscard]] std::size_t fenceCount() const noexcept
        {
            return fences.size();
        }

        bool operator==(const Info &) const noexcept = default;
    };

    JobSyncFile() = delete;
    ~JobSyncFile() = delete;

    JobSyncFile(const JobSyncFile &) = delete;
    JobSyncFile &operator=(const JobSyncFile &) = delete;

    JobSyncFile(JobSyncFile &&) = delete;
    JobSyncFile &operator=(JobSyncFile &&) = delete;

    [[nodiscard]] static JobRenderFence merge(const JobRenderFence &first, const JobRenderFence &second, std::string_view name = {});

    [[nodiscard]] static bool info(const JobRenderFence &fence, Info &info);
    [[nodiscard]] static Info info(const JobRenderFence &fence);

    [[nodiscard]] static bool setDeadline(const JobRenderFence &fence, Timestamp deadlineNs) noexcept;
};

}