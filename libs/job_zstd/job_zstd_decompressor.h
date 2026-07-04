#pragma once

#include "job_zstd_options.h"

namespace job::zstd {

class JobZstdDecompressor : public JobZstdOptions
{

public:
    JobZstdDecompressor() = default;
    ~JobZstdDecompressor() override = default;

    virtual bool execute();
    virtual bool decompressFolder();
    virtual bool decompressFile();
    virtual bool decompressEmptyDirectoryArchive();
    virtual bool decompressSymlinkArchive();
};

} // namespace job::zstd