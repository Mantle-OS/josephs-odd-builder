#pragma once

#include <streambuf>
#include <vector>
#include <string>
#include <optional>
#include <cstddef>

#include "job_zstd_options.h"

typedef struct ZSTD_CCtx_s ZSTD_CStream;
typedef struct ZSTD_DCtx_s ZSTD_DStream;

namespace job::zstd {

class JobZstdIO : public std::streambuf
{
public:
    enum class Mode { ReadOnly, WriteOnly };

    explicit JobZstdIO(std::streambuf *transport);
    ~JobZstdIO() override;

    [[nodiscard]] bool setCompressionLevel(int level = JobZstdOptions::kDefaultCompressionLevel);
    [[nodiscard]] int compressionLevel() const noexcept { return m_compressionLevel; }

    [[nodiscard]] bool open(Mode mode);
    [[nodiscard]] bool close();

    [[nodiscard]] bool isOpen() const noexcept { return m_mode.has_value(); }
    [[nodiscard]] bool atEnd() const;
    [[nodiscard]] bool wasTruncated() const noexcept { return m_truncated; }
    [[nodiscard]] bool hadDecodeError() const noexcept { return m_decodeFailed; }
    [[nodiscard]] bool hadEncodeError() const noexcept { return m_encodeFailed; }


    [[nodiscard]] const std::string &errorString() const noexcept { return m_errorString; }

protected:
    int_type        underflow() override;
    int_type        overflow(int_type ch) override;
    int             sync() override;
    std::streamsize xsputn(const char *s, std::streamsize count) override;
    std::streamsize xsgetn(char *s, std::streamsize count) override;
    std::streamsize showmanyc() override;

private:
    // func
    bool            initCompression();
    bool            initDecompression();
    bool            flushCompressionWindow();
    std::size_t     fillInputBuffer();

    // members
    std::streambuf          *m_transport            = nullptr;
    std::optional<Mode>     m_mode;
    int                     m_compressionLevel      = JobZstdOptions::kDefaultCompressionLevel;
    ZSTD_CStream            *m_cStream              = nullptr;
    ZSTD_DStream            *m_dStream              = nullptr;
    std::vector<char>       m_inBuffer;
    std::vector<char>       m_outBuffer;
    std::size_t             m_inBufferPos           = 0;
    std::size_t             m_inBufferSize          = 0;
    bool                    m_decompressionFinished = false;
    bool                    m_truncated             = false;
    bool                    m_decodeFailed          = false;
    bool                    m_encodeFailed          = false;
    std::string             m_errorString;
};

} // namespace job::zstd