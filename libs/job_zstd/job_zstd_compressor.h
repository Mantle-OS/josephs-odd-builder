#pragma once

#include "job_zstd_options.h"

#include "jobzstd_export.h"

namespace job::zstd {

class JOBZSTD_EXPORT JobZstdCompressor : public JobZstdOptions
{
public:
    JobZstdCompressor() = default;
    ~JobZstdCompressor() override = default;

    JobZstdCompressor(const JobZstdCompressor &) = default;
    JobZstdCompressor(JobZstdCompressor &&) noexcept = default;
    JobZstdCompressor &operator=(const JobZstdCompressor &) = default;
    JobZstdCompressor &operator=(JobZstdCompressor &&) noexcept = default;

    virtual bool execute();
    virtual bool compressFolder();
    virtual bool compressFile();
};

} // namespace job::zstd