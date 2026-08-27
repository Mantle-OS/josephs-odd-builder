#include "io_factory.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

#include <io_base.h>

#include "file_io.h"
#include "pty_io.h"

#include "job_arena_pool.h"
#include "job_mem_extent.h"
#include "job_mem_pool.h"
#include "job_mem_size.h"
#include "job_mmap.h"
#include "job_page_pool.h"
#include "job_range_pool.h"
#include "job_size_pool.h"
namespace job::io {

std::shared_ptr<IODevice> IOFactory::createFromType(FactoryType type, const std::string &target)
{
    switch (type) {
    case FactoryType::Pty:
        return std::make_shared<PtyIO>();

    case FactoryType::FileStdIn:
        return std::make_shared<FileIO>("stdin", FileMode::StdIn, false);

    case FactoryType::FileStdOut:
        return std::make_shared<FileIO>("stdout", FileMode::StdOut, true);

    case FactoryType::FileStdErr:
        return std::make_shared<FileIO>("stderr", FileMode::StdErr, true);

    case FactoryType::FileName:
        return std::make_shared<FileIO>(target, FileMode::RegularFile, true);

    case FactoryType::JobFile:
    case FactoryType::SharedMemory:
    case FactoryType::Mmap:
        break;
    }

    throw std::runtime_error("Unsupported FactoryType in IOFactory::createFromType()");
}

std::shared_ptr<IODevice> IOFactory::createFromURI(const std::string &uri)
{
    const auto sep = uri.find(':');

    if (sep == std::string::npos || sep == 0)
        throw std::invalid_argument("Invalid IO URI format: '" + uri + "'");

    std::string type = uri.substr(0, sep);
    const std::string target = sep < uri.length() - 1 ? uri.substr(sep + 1) : "";

    std::transform(type.begin(), type.end(), type.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (type == "pty")
        return createFromType(FactoryType::Pty, target);

    if (type == "file") {
        if (target == "stdout")
            return createFromType(FactoryType::FileStdOut, "");

        if (target == "stderr")
            return createFromType(FactoryType::FileStdErr, "");

        if (target == "stdin")
            return createFromType(FactoryType::FileStdIn, "");

        if (!target.empty())
            return createFromType(FactoryType::FileName, target);

        throw std::invalid_argument("Missing file target for 'file:' URI");
    }

    throw std::runtime_error("Unsupported IO type: '" + type + "'");
}

// JobMemPool::Ptr IOFactory::createMemPool(JobMemPool::Type type, std::size_t size)
// {
//     switch (type) {
//     case JobMemPool::Type::Range:
//         return createRangePool(size);

//     case JobMemPool::Type::Page:
//         return createPagePool(size);

//     case JobMemPool::Type::Arena:
//         return createArenaPool(size);

//     case JobMemPool::Type::Size:
//     case JobMemPool::Type::Unknown:
//         return nullptr;
//     }

//     return nullptr;
// }

// JobMemPool::Ptr IOFactory::createMemPool(JobMemPool::Type type, JobMmap::Ptr mmap)
// {
//     switch (type) {
//     case JobMemPool::Type::Range:
//         return createRangePool(std::move(mmap));

//     case JobMemPool::Type::Page:
//         return createPagePool(std::move(mmap));

//     case JobMemPool::Type::Arena:
//         return createArenaPool(std::move(mmap));

//     case JobMemPool::Type::Size:
//     case JobMemPool::Type::Unknown:
//         return nullptr;
//     }

//     return nullptr;
// }

// JobMemPool::Ptr IOFactory::createMemPool(JobMemPool::Type type, JobMemExtent::Ptr extent)
// {
//     switch (type) {
//     case JobMemPool::Type::Page:
//         return createPagePool(std::move(extent));

//     case JobMemPool::Type::Arena:
//         return createArenaPool(std::move(extent));

//     case JobMemPool::Type::Range:
//     case JobMemPool::Type::Size:
//     case JobMemPool::Type::Unknown:
//         return nullptr;
//     }

//     return nullptr;
// }

// JobRangePool::Ptr IOFactory::createRangePool(std::size_t size)
// {
//     return JobRangePool::createShared(size);
// }

// JobRangePool::Ptr IOFactory::createRangePool(JobMmap::Ptr mmap)
// {
//     return JobRangePool::createShared(std::move(mmap));
// }

// JobPagePool::Ptr IOFactory::createPagePool(std::size_t size, std::size_t pageSize)
// {
//     return JobPagePool::createShared(size, pageSize);
// }

// JobPagePool::Ptr IOFactory::createPagePool(JobMmap::Ptr mmap, std::size_t pageSize)
// {
//     return JobPagePool::createShared(std::move(mmap), pageSize);
// }

// JobPagePool::Ptr IOFactory::createPagePool(JobMemExtent::Ptr extent, std::size_t pageSize)
// {
//     return JobPagePool::createShared(std::move(extent), pageSize);
// }

// JobSizePool::Ptr IOFactory::createSizePool(const JobMemSize &sizeClass, JobPagePool::Ptr pagePool)
// {
//     return JobSizePool::createShared(sizeClass, std::move(pagePool));
// }

// JobArenaPool::Ptr IOFactory::createArenaPool(std::size_t size)
// {
//     return JobArenaPool::createShared(size);
// }

// JobArenaPool::Ptr IOFactory::createArenaPool(JobMmap::Ptr mmap)
// {
//     return JobArenaPool::createShared(std::move(mmap));
// }

// JobArenaPool::Ptr IOFactory::createArenaPool(JobMemExtent::Ptr extent)
// {
//     return JobArenaPool::createShared(std::move(extent));
// }

} // namespace job::io