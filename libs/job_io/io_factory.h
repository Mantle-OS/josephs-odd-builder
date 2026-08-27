#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <io_base.h>

namespace job::io {

class JobArenaPool;
class JobMemExtent;
class JobMemPool;
class JobMemSize;
class JobMmap;
class JobPagePool;
class JobRangePool;
class JobSizePool;

enum class FactoryType : std::uint8_t
{
    Pty,
    FileStdIn,
    FileStdOut,
    FileStdErr,
    FileName,
    JobFile,
    SharedMemory,
    Mmap
};

class IOFactory final
{
public:
    IOFactory() = delete;
    ~IOFactory() = delete;

    IOFactory(const IOFactory &) = delete;
    IOFactory(IOFactory &&) = delete;
    IOFactory &operator=(const IOFactory &) = delete;
    IOFactory &operator=(IOFactory &&) = delete;

    [[nodiscard]] static std::shared_ptr<IODevice> createFromType(FactoryType type, const std::string &target);
    [[nodiscard]] static std::shared_ptr<IODevice> createFromURI(const std::string &uri);

    [[nodiscard]] static std::shared_ptr<JobRangePool> createRangePool(std::size_t size);
    [[nodiscard]] static std::shared_ptr<JobRangePool> createRangePool(std::shared_ptr<JobMmap> mmap);

    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::size_t size);
    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::size_t size, std::size_t pageSize);
    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::shared_ptr<JobMmap> mmap);
    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::shared_ptr<JobMmap> mmap, std::size_t pageSize);
    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::shared_ptr<JobMemExtent> extent);
    [[nodiscard]] static std::shared_ptr<JobPagePool> createPagePool(std::shared_ptr<JobMemExtent> extent, std::size_t pageSize);

    [[nodiscard]] static std::shared_ptr<JobSizePool> createSizePool(const JobMemSize &sizeClass, std::shared_ptr<JobPagePool> pagePool);

    [[nodiscard]] static std::shared_ptr<JobArenaPool> createArenaPool(std::size_t size);
    [[nodiscard]] static std::shared_ptr<JobArenaPool> createArenaPool(std::shared_ptr<JobMmap> mmap);
    [[nodiscard]] static std::shared_ptr<JobArenaPool> createArenaPool(std::shared_ptr<JobMemExtent> extent);
};

} // namespace job::io