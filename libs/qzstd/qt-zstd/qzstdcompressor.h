#pragma once

#include <job_zstd_compressor.h>

#include "qzstdoptions.h"

class QZstdCompressor : public job::zstd::JobZstdCompressor
{

public:
    explicit QZstdCompressor();
    QZstdCompressor(QZstdOptions *opts = nullptr);
    ~QZstdCompressor();
    QZstdCompressor(const QZstdCompressor &) = delete;
    QZstdCompressor(QZstdCompressor &&) noexcept = delete;
    QZstdCompressor &operator=(const QZstdCompressor &) = delete;
    QZstdCompressor &operator=(QZstdCompressor &&) noexcept = delete;

    [[nodiscard]] bool compress() noexcept;
    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);
    void freeOptions();

private:
    void setupOptionConnections() noexcept;
    void disconnectOptionConnections() noexcept;
    QZstdOptions    *m_opts     = nullptr;
    bool            m_ownsOpts  = true;
};