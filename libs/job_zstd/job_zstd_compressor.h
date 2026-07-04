#pragma once

#include "job_zstd_options.h"

namespace job::zstd {

class JobZstdCompressor : public JobZstdOptions
{
public:
    JobZstdCompressor() = default;
    ~JobZstdCompressor() override = default;
    virtual bool execute();
    virtual bool compressFolder();
    virtual bool compressFile();
};

} // namespace job::zstd