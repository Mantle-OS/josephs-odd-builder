#pragma once

#include <cstdint>
#include <memory>

#include "job_render_fence.h"

namespace job::render {

enum class DmaBufSyncAccess : std::uint8_t
{
    Read,
    Write,
    ReadWrite
};

class JobDmaBufSync
{
public:
    using Ptr  = std::shared_ptr<JobDmaBufSync>;
    using WPtr = std::weak_ptr<JobDmaBufSync>;
    using UPtr = std::unique_ptr<JobDmaBufSync>;

    using NativeHandle = int;

    JobDmaBufSync() = delete;
    ~JobDmaBufSync() = delete;

    JobDmaBufSync(const JobDmaBufSync &) = delete;
    JobDmaBufSync &operator=(const JobDmaBufSync &) = delete;

    JobDmaBufSync(JobDmaBufSync &&) = delete;
    JobDmaBufSync &operator=(JobDmaBufSync &&) = delete;

    [[nodiscard]] static bool beginCpuAccess(NativeHandle fd, DmaBufSyncAccess access) noexcept;
    [[nodiscard]] static bool endCpuAccess(NativeHandle fd, DmaBufSyncAccess access) noexcept;

    [[nodiscard]] static JobRenderFence exportSyncFile(NativeHandle fd, DmaBufSyncAccess access);

    [[nodiscard]] static bool importSyncFile(NativeHandle fd, const JobRenderFence &fence, DmaBufSyncAccess access) noexcept;

private:
    [[nodiscard]] static std::uint64_t accessFlags(DmaBufSyncAccess access) noexcept;
};

}