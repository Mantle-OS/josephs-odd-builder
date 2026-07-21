#include "job_zstd_io.h"
#include <zstd.h>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <limits>



namespace job::zstd {

JobZstdIO::JobZstdIO(std::streambuf *transport) : m_transport(transport)
{
}

JobZstdIO::~JobZstdIO()
{
    if(!close()){
        /// Oh fuck !
    }
}

bool JobZstdIO::setCompressionLevel(int level)
{
    if (isOpen())
        return false; // Too late, the door's already locked.

    int const minLevel = JobZstdOptions::minCompressionLevel();
    int const maxLevel = JobZstdOptions::maxCompressionLevel();

    m_compressionLevel = (level >= minLevel && level <= maxLevel)
                             ? level
                             : JobZstdOptions::kDefaultCompressionLevel;
    return true;
}

bool JobZstdIO::open(Mode mode)
{
    if (isOpen())
        return false;

    if (!m_transport){
        m_errorString = "Target transport streambuf is null.";
        return false;
    }

    m_decompressionFinished = false;
    m_truncated             = false;
    m_decodeFailed          = false;
    m_encodeFailed          = false;


    if (mode == Mode::ReadOnly) {
        if (!initDecompression())
            return false;

        char *end = m_outBuffer.data() + m_outBuffer.size();
        setg(end, end, end);
    } else {
        if (!initCompression())
            return false;

        setp(nullptr, nullptr);
    }

    m_mode = mode;
    return true;
}

bool JobZstdIO::close()
{
    if (!isOpen())
        return true;

    bool ok = true;

    if (*m_mode == Mode::WriteOnly){
        bool const flushOk = flushCompressionWindow();
        ok = flushOk && !m_encodeFailed;
    } else if (m_truncated || m_decodeFailed){
        ok = false;
    }


    if (m_cStream) {
        ZSTD_freeCStream(m_cStream);
        m_cStream = nullptr;
    }

    if (m_dStream) {
        ZSTD_freeDStream(m_dStream);
        m_dStream = nullptr;
    }

    m_inBuffer.clear();
    m_outBuffer.clear();
    m_inBufferPos = 0;
    m_inBufferSize = 0;
    m_decompressionFinished = false;

    setg(nullptr, nullptr, nullptr);
    setp(nullptr, nullptr);
    m_mode.reset();

    return ok;
}

bool JobZstdIO::initCompression()
{
    m_cStream = ZSTD_createCStream();
    if (!m_cStream) {
        m_errorString = "Failed to allocate ZSTD_CStream.";
        return false;
    }

    size_t const initResult = ZSTD_initCStream(m_cStream, m_compressionLevel);
    if (ZSTD_isError(initResult)) {
        m_errorString = ZSTD_getErrorName(initResult);
        ZSTD_freeCStream(m_cStream);
        m_cStream = nullptr;
        return false;
    }

    m_outBuffer.resize(ZSTD_CStreamOutSize());
    return true;
}

bool JobZstdIO::initDecompression()
{
    m_dStream = ZSTD_createDStream();
    if (!m_dStream) {
        m_errorString = "Failed to allocate ZSTD_DStream.";
        return false;
    }

    size_t const initResult = ZSTD_initDStream(m_dStream);
    if (ZSTD_isError(initResult)) {
        m_errorString = ZSTD_getErrorName(initResult);
        ZSTD_freeDStream(m_dStream);
        m_dStream = nullptr;
        return false;
    }

    m_inBuffer.resize(ZSTD_DStreamInSize());
    m_outBuffer.resize(ZSTD_DStreamOutSize());
    return true;
}

std::size_t JobZstdIO::fillInputBuffer()
{
    m_inBufferPos = 0;

    std::streamsize const bytesRead = m_transport->sgetn(
        m_inBuffer.data(),
        static_cast<std::streamsize>(m_inBuffer.size())
        );

    m_inBufferSize = (bytesRead > 0) ?
                         static_cast<std::size_t>(bytesRead) :
                         0;
    return m_inBufferSize;
}

// The slow lane: single-char / formatted extraction (operator>>, get()).
// Everything that actually matters for throughput goes through xsgetn() instead.
JobZstdIO::int_type JobZstdIO::underflow()
{
    if (gptr() < egptr())
        return traits_type::to_int_type(*gptr());

    if (!m_dStream || m_decompressionFinished || m_truncated)
        return traits_type::eof();

    ZSTD_outBuffer output{ m_outBuffer.data(), m_outBuffer.size(), 0 };

    while (output.pos < output.size) {
        if (m_inBufferPos >= m_inBufferSize) {
            if (fillInputBuffer() == 0) {
                if (!m_decompressionFinished) {
                    m_truncated = true;
                    m_errorString = "Transport ended before zstd frame was complete (truncated stream).";
                }
                break;
            }
        }

        std::size_t const remainingInput = m_inBufferSize - m_inBufferPos;
        ZSTD_inBuffer input{ m_inBuffer.data() + m_inBufferPos, remainingInput, 0 };

        std::size_t const decompressResult = ZSTD_decompressStream(m_dStream, &output, &input);
        m_inBufferPos += input.pos;

        if (ZSTD_isError(decompressResult)) {
            m_errorString = ZSTD_getErrorName(decompressResult);
            m_decodeFailed = true;
            ZSTD_freeDStream(m_dStream);
            m_dStream = nullptr;
            return traits_type::eof();
        }

        if (decompressResult == 0) {
            m_decompressionFinished = true;
            break;
        }
    }

    if (output.pos == 0)
        return traits_type::eof(); // Covers both a clean finish and the truncated case flagged above.

    assert(output.pos <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
    setg(m_outBuffer.data(), m_outBuffer.data(), m_outBuffer.data() + output.pos);
    return traits_type::to_int_type(*gptr());
}

// The fast lane: bulk reads (istream::read(), sgetn()). Decompresses straight
// into the caller's buffer, no detour through m_outBuffer, no extra memcpy.
std::streamsize JobZstdIO::xsgetn(char *s, std::streamsize count)
{
    if (count <= 0 || !m_dStream || !m_mode || *m_mode != Mode::ReadOnly || m_truncated)
        return 0;

    std::streamsize totalCopied = 0;

    if (gptr() < egptr()) {
        std::streamsize const avail = static_cast<std::streamsize>(egptr() - gptr());
        std::streamsize const n     = std::min(avail, count);

        assert(n <= static_cast<std::streamsize>(std::numeric_limits<int>::max()));
        std::memcpy(s, gptr(), static_cast<std::size_t>(n));
        gbump(static_cast<int>(n));
        totalCopied += n;
    }

    if (totalCopied == count || m_decompressionFinished)
        return totalCopied;

    ZSTD_outBuffer output{ s + totalCopied, static_cast<std::size_t>(count - totalCopied), 0 };

    while (output.pos < output.size) {
        if (m_inBufferPos >= m_inBufferSize) {
            if (fillInputBuffer() == 0) {
                if (!m_decompressionFinished) {
                    m_truncated = true;
                    m_errorString = "Transport ended before zstd frame was complete (truncated stream).";
                }
                break;
            }
        }

        std::size_t const remainingInput = m_inBufferSize - m_inBufferPos;
        ZSTD_inBuffer input{ m_inBuffer.data() + m_inBufferPos, remainingInput, 0 };

        std::size_t const decompressResult = ZSTD_decompressStream(m_dStream, &output, &input);
        m_inBufferPos += input.pos;

        if (ZSTD_isError(decompressResult)) {
            m_errorString = ZSTD_getErrorName(decompressResult);
            m_decodeFailed = true;
            ZSTD_freeDStream(m_dStream);
            m_dStream = nullptr;
            return totalCopied + static_cast<std::streamsize>(output.pos);
        }

        if (decompressResult == 0) {
            m_decompressionFinished = true;
            break;
        }
    }

    // On truncation, bytes decompressed before the cutoff are still real and still returned.
    // CALLER MUST CHECK wasTruncated() -- a short read here does not by itself mean clean EOF.
    return totalCopied + static_cast<std::streamsize>(output.pos);
}

std::streamsize JobZstdIO::xsputn(const char *s, std::streamsize count)
{
    if (count <= 0 || !m_cStream || !m_mode || *m_mode != Mode::WriteOnly)
        return 0;

    ZSTD_inBuffer input{ s, static_cast<std::size_t>(count), 0 };

    while (input.pos < input.size) {
        ZSTD_outBuffer output{ m_outBuffer.data(), m_outBuffer.size(), 0 };

        std::size_t const compressResult = ZSTD_compressStream(m_cStream, &output, &input);
        if (ZSTD_isError(compressResult)) {
            m_errorString = ZSTD_getErrorName(compressResult);
            // FIXME LATER  m_decodeFailed = true;
            ZSTD_freeCStream(m_cStream);
            m_cStream = nullptr;
            return static_cast<std::streamsize>(input.pos);
        }

        if (output.pos > 0) {
            std::streamsize const written = m_transport->sputn(
                m_outBuffer.data(), static_cast<std::streamsize>(output.pos));

            if (written != static_cast<std::streamsize>(output.pos)) {
                m_errorString  = "Transport streambuf write short-count or failure.";
                m_encodeFailed = true;
                return static_cast<std::streamsize>(input.pos);
            }
        }
    }

    return static_cast<std::streamsize>(input.pos);
}

JobZstdIO::int_type JobZstdIO::overflow(int_type ch)
{
    if (traits_type::eq_int_type(ch, traits_type::eof()))
        return traits_type::not_eof(ch);

    char const c = traits_type::to_char_type(ch);
    return (xsputn(&c, 1) == 1) ?
               ch :
               traits_type::eof();
}

int JobZstdIO::sync()
{
    if (!m_transport)
        return -1;

    return m_transport->pubsync();
}

std::streamsize JobZstdIO::showmanyc()
{
    if (!m_mode || *m_mode != Mode::ReadOnly)
        return -1;

    if (m_truncated)
        return -1; // Error state -- don't advertise bytes we can't actually stand behind.

    if (gptr() && egptr() && gptr() < egptr())
        return egptr() - gptr();

    return 0;
}

// Deliberately does NOT force ZSTD_flushStream on every write -- that would
// fragment compressed blocks and tank the ratio for no real benefit..... I think
// Real finalization happens exactly once, in close().
// AKA Measure twice, flush once.
bool JobZstdIO::flushCompressionWindow()
{
    if (!m_cStream || !m_transport)
        return false;

    std::size_t remaining = 0;

    do {
        ZSTD_outBuffer output{ m_outBuffer.data(), m_outBuffer.size(), 0 };
        remaining = ZSTD_endStream(m_cStream, &output);

        if (ZSTD_isError(remaining)) {
            m_errorString = ZSTD_getErrorName(remaining);
            ZSTD_freeCStream(m_cStream);
            m_cStream = nullptr;
            return false;
        }

        if (output.pos > 0) {
            std::streamsize const written = m_transport->sputn(
                m_outBuffer.data(), static_cast<std::streamsize>(output.pos));

            if (written != static_cast<std::streamsize>(output.pos)) {
                m_errorString = "Transport streambuf write failure during final flush.";
                m_encodeFailed = true;
                return false;
            }
        }
    } while (remaining > 0);

    return true;
}

bool JobZstdIO::atEnd() const
{
    if (!m_mode)
        return true; // Closed: nothing left to give or take.

    if (*m_mode == Mode::WriteOnly)
        return true;

    if (m_truncated || m_decodeFailed)
        return false; // An error state, not a clean end -- check wasTruncated()/errorString().

    if (!m_decompressionFinished)
        return false;

    if (m_inBufferPos < m_inBufferSize)
        return false;

    if (!gptr() || !egptr())
        return true; // Buffers already torn down; nothing pending.

    return gptr() >= egptr();
}

} // namespace job::zstd