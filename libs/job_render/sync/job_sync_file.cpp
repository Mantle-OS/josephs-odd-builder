#include "job_sync_file.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include <sys/ioctl.h>

#include <linux/sync_file.h>

#ifndef NDEBUG
#include <job_logger.h>
#endif

namespace job::render {

JobRenderFence JobSyncFile::merge(const JobRenderFence &first, const JobRenderFence &second, std::string_view name)
{
    if (!first.valid() || !second.valid())
        return {};

    sync_merge_data data{};

    const std::size_t nameLength = std::min( name.size(), sizeof(data.name) - 1);
    if (nameLength > 0)
        std::memcpy(data.name, name.data(), nameLength);

    data.name[nameLength] = '\0';
    data.fd2 = second.fd();

    if (::ioctl(first.fd(), SYNC_IOC_MERGE, &data) < 0)
        return {};

    if (data.fence < 0)
        return {};

    return JobRenderFence{ data.fence };
}

bool JobSyncFile::info(const JobRenderFence &fence, Info &info)
{
    info = {};
    if (!fence.valid())
        return false;

    sync_file_info fileInfo{};

    // One thing I would watch later is the race between the first and second SYNC_IOC_FILE_INFO calls if the underlying sync_file
    //  were somehow to expose a changing fence count. In normal usage the constituent fence set is fixed, so the two-pass pattern is exactly what the kernel interface expects.
    if (::ioctl(fence.fd(), SYNC_IOC_FILE_INFO, &fileInfo) < 0)
        return false;
    std::vector<sync_fence_info> fenceInfos(fileInfo.num_fences);
    if (!fenceInfos.empty()) {
        fileInfo.sync_fence_info = reinterpret_cast<std::uintptr_t>(fenceInfos.data());

        if (::ioctl(fence.fd(), SYNC_IOC_FILE_INFO, &fileInfo) < 0)
            return false;
    }

    Info result;

    result.name = fileInfo.name;
    result.status = fileInfo.status;
    result.flags = fileInfo.flags;

    result.fences.reserve(fileInfo.num_fences);

    for (std::uint32_t i = 0; i < fileInfo.num_fences; ++i) {
        const auto &source = fenceInfos[i];

        FenceInfo destination;

        destination.objectName = source.obj_name;
        destination.driverName = source.driver_name;

        destination.status = source.status;
        destination.flags = source.flags;
        destination.timestampNs = source.timestamp_ns;

        result.fences.push_back(std::move(destination));
    }

    info = std::move(result);

    return true;
}

JobSyncFile::Info JobSyncFile::info(const JobRenderFence &fence)
{
    Info result;
    if(!info(fence, result)){
#ifndef NDEBUG
        JOB_LOG_DEBUG("[JobSyncFile] unable to get info from the fence, the result is most likley bad.");
#endif
    }
    return result;
}

bool JobSyncFile::setDeadline(const JobRenderFence &fence, Timestamp deadlineNs) noexcept
{
    if (!fence.valid())
        return false;

    sync_set_deadline deadline{};
    deadline.deadline_ns = deadlineNs;
    return ::ioctl(fence.fd(), SYNC_IOC_SET_DEADLINE, &deadline) == 0;
}

}