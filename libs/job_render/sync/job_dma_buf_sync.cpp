#include "job_dma_buf_sync.h"

#include <sys/ioctl.h>

#include <linux/dma-buf.h>

namespace job::render {

bool JobDmaBufSync::beginCpuAccess(NativeHandle fd, DmaBufSyncAccess access) noexcept
{
    if (fd < 0)
        return false;

    dma_buf_sync sync{};
    sync.flags = accessFlags(access) | DMA_BUF_SYNC_START;

    return ::ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync) == 0;
}

bool JobDmaBufSync::endCpuAccess(NativeHandle fd, DmaBufSyncAccess access) noexcept
{
    if (fd < 0)
        return false;

    dma_buf_sync sync{};
    sync.flags = accessFlags(access) | DMA_BUF_SYNC_END;
    return ::ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync) == 0;
}

JobRenderFence JobDmaBufSync::exportSyncFile(NativeHandle fd, DmaBufSyncAccess access)
{
    if (fd < 0)
        return {};

    dma_buf_export_sync_file exportData{};
    exportData.flags = static_cast<std::uint32_t>(accessFlags(access));

    exportData.fd = -1;

    if (::ioctl(fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &exportData) != 0)
        return {};

    if (exportData.fd < 0)
        return {};

    return JobRenderFence{ exportData.fd };
}

bool JobDmaBufSync::importSyncFile(NativeHandle fd, const JobRenderFence &fence, DmaBufSyncAccess access) noexcept
{
    if (fd < 0 || !fence.valid())
        return false;

    dma_buf_import_sync_file importData{};

    importData.flags = static_cast<std::uint32_t>(accessFlags(access));

    importData.fd = fence.fd();
    return ::ioctl(fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &importData) == 0;
}

std::uint64_t JobDmaBufSync::accessFlags(DmaBufSyncAccess access) noexcept
{
    switch (access) {
    case DmaBufSyncAccess::Read:
        return DMA_BUF_SYNC_READ;

    case DmaBufSyncAccess::Write:
        return DMA_BUF_SYNC_WRITE;

    case DmaBufSyncAccess::ReadWrite:
        return DMA_BUF_SYNC_RW;
    }

    return 0;
}

}