#pragma once

#include <job_zstd_compressor.h>

#include "qzstdoptions.h"

#include "qzstd_export.h"

class QZSTD_EXPORT QZstdCompressor : public job::zstd::JobZstdCompressor
{

public:
    explicit QZstdCompressor();
    explicit QZstdCompressor(QZstdOptions *opts = nullptr);
    ~QZstdCompressor();
    QZstdCompressor(const QZstdCompressor &) = delete;
    QZstdCompressor(QZstdCompressor &&) noexcept = delete;
    QZstdCompressor &operator=(const QZstdCompressor &) = delete;
    QZstdCompressor &operator=(QZstdCompressor &&) noexcept = delete;

    [[nodiscard]] bool compress();
    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);

private:
    void setupOptionConnections() noexcept;
    void disconnectOptionConnections() noexcept;
    QZstdOptions    *m_opts     = nullptr;
    bool            m_ownsOpts  = true;
};