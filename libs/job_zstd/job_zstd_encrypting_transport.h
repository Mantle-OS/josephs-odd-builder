// job_zstd_encrypting_transport.h
#pragma once

#include <streambuf>
#include <ostream>
#include <string>
#include <cstddef>
#include <vector>

#include <job_secure_mem.h>

namespace job::zstd {
class JobZstdEncryptingTransport : public std::streambuf
{
public:
    static constexpr std::size_t kDefaultChunkSize = 65536;

    JobZstdEncryptingTransport(std::streambuf *downstream, const job::crypto::JobSecureMem &key,
                               std::size_t chunkSize = kDefaultChunkSize);

    // Encrypts and writes whatever partial chunk remains buffered. Must be
    // called exactly once, after the upstream JobZstdIO has already been
    // close()'d -- this is where the final (possibly short) chunk actually
    // gets encrypted and flushed.
    [[nodiscard]] bool finish();

    [[nodiscard]] bool hadEncodeError() const noexcept { return m_encodeFailed; }
    [[nodiscard]] const std::string &errorString() const noexcept { return m_errorString; }

protected:
    std::streamsize xsputn(const char *s, std::streamsize count) override;
    int_type        overflow(int_type ch) override;

private:
    [[nodiscard]] bool encryptAndWriteChunk(const char *data, std::size_t len);
    void flushFullChunks();

private:
    std::streambuf              *m_downstream;
    std::ostream                m_downstreamOut;
    job::crypto::JobSecureMem   m_key;
    std::size_t                 m_chunkSize;
    std::vector<unsigned char>  m_buffer;
    std::size_t                 m_bufferFill        = 0;
    bool                        m_encodeFailed      = false;
    std::string                 m_errorString;
    bool                        m_finished          = false;
};

} // namespace job::zstd