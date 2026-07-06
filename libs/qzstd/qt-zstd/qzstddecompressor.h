#pragma once
#include <job_zstd_decompressor.h>
#include "qzstdoptions.h"

class QZstdDecompressor : public job::zstd::JobZstdDecompressor
{
public:
    explicit QZstdDecompressor();
    QZstdDecompressor(QZstdOptions *opts);
    ~QZstdDecompressor();

    QZstdDecompressor(const QZstdDecompressor &) = delete;
    QZstdDecompressor(QZstdDecompressor &&) noexcept = delete;
    QZstdDecompressor &operator=(const QZstdDecompressor &) = delete;
    QZstdDecompressor &operator=(QZstdDecompressor &&) noexcept = delete;

    [[nodiscard]] bool decompress() noexcept;
    [[nodiscard]] QZstdOptions *options() const;
    void setOptions(QZstdOptions *other);

private:
    void setupOptionConnections() noexcept;
    void disconnectOptionConnections() noexcept;
    QZstdOptions *m_opts = nullptr; // owned unless m_ownsOpts is true (somone called setOptions)
    bool m_ownsOpts = true;

};