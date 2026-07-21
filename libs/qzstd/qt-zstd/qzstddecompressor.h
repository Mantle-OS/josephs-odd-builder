#pragma once
#include <job_zstd_decompressor.h>
#include "qzstdoptions.h"

#include "qzstd_export.h"

class QZSTD_EXPORT QZstdDecompressor : public job::zstd::JobZstdDecompressor
{
public:
    explicit QZstdDecompressor();
    explicit QZstdDecompressor(QZstdOptions *opts);
    ~QZstdDecompressor();

    QZstdDecompressor(const QZstdDecompressor &) = delete;
    QZstdDecompressor(QZstdDecompressor &&) noexcept = delete;
    QZstdDecompressor &operator=(const QZstdDecompressor &) = delete;
    QZstdDecompressor &operator=(QZstdDecompressor &&) noexcept = delete;

    [[nodiscard]] bool decompress();
    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);

private:
    void setupOptionConnections() noexcept;
    void disconnectOptionConnections() noexcept;
    QZstdOptions *m_opts = nullptr;
    bool m_ownsOpts = true;
};