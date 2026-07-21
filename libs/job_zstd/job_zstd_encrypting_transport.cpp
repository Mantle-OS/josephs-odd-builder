#include "job_zstd_encrypting_transport.h"

#include <cstring>
#include <algorithm>

#include <sodium/utils.h>

#include "job_zstd_wire.h"
#include "job_secret_box.h"

namespace job::zstd {
JobZstdEncryptingTransport::JobZstdEncryptingTransport(std::streambuf *downstream,
                                                       const job::crypto::JobSecureMem &key,
                                                       std::size_t chunkSize) :
    m_downstream(downstream),
    m_downstreamOut(downstream),
    m_key(key),
    m_chunkSize(chunkSize),
    m_buffer(chunkSize)
{
    if (m_chunkSize == 0) {
        // A zero chunk size can never fill, which means the buffering loop in xsputn() would spin forever copying zero bytes per iteration.
        m_encodeFailed = true;
        m_errorString = "JobZstdEncryptingTransport: chunk size must be greater than zero.";
    }
}

// Encrypts exactly [data, data+len) as one chunk and writes
// [nonce][length-prefixed ciphertext] downstream, using the same length-prefixed framing the archive format already uses elsewhere.
// free length-guard hardening on the read side, and one less wire format to invent.
// Neither the nonce nor the ciphertext is secret once produced, so no scrubbing happens on those two.
// Only the plaintext copy made to satisfy JobSecretBox::encrypt()'s vector<unsigned char> signature gets
// zeroed before it goes out of scope (The danger zone maverick).
bool JobZstdEncryptingTransport::encryptAndWriteChunk(const char *data, std::size_t len)
{
    std::vector<unsigned char> plainChunk(
        reinterpret_cast<const unsigned char *>(data),
        reinterpret_cast<const unsigned char *>(data) + len);

    std::vector<unsigned char> cipherBytes;
    std::vector<unsigned char> nonceBytes;

    bool const encrypted = job::crypto::JobSecretBox::encrypt(plainChunk, m_key, cipherBytes, nonceBytes);

    sodium_memzero(plainChunk.data(), plainChunk.size());

    if (!encrypted) {
        m_encodeFailed = true;
        m_errorString = "Chunk encryption failed -- check the encryption key.";
        return false;
    }

    std::string const nonceStr(nonceBytes.begin(), nonceBytes.end());
    std::string const cipherStr(cipherBytes.begin(), cipherBytes.end());

    job::zstd::utils::writeString(m_downstreamOut, nonceStr);
    job::zstd::utils::writeString(m_downstreamOut, cipherStr);

    if (!m_downstreamOut) {
        m_encodeFailed = true;
        m_errorString = "Downstream write failed while flushing an encrypted chunk.";
        return false;
    }

    return true;
}


// The whole reason a fixed-capacity vector was chosen over a growable
// buffer: after every chunk flush, the ENTIRE capacity gets zeroed, not
// just the bytes that were logically "in use." A growable buffer's spare
// capacity after erase() would otherwise sit there holding stale plaintext
// indefinitely. There's no spare-capacity concept here at all here.
// The buffer is always either mid-fill or fully scrubbed.
std::streamsize JobZstdEncryptingTransport::xsputn(const char *s, std::streamsize count)
{
    if (count <= 0 || m_encodeFailed || m_finished)
        return 0;

    std::streamsize written = 0;

    while (written < count) {
        std::size_t const spaceLeft = m_chunkSize - m_bufferFill;
        std::streamsize const toCopy = std::min<std::streamsize>(count - written, static_cast<std::streamsize>(spaceLeft));

        std::memcpy(m_buffer.data() + m_bufferFill, s + written, static_cast<std::size_t>(toCopy));
        m_bufferFill += static_cast<std::size_t>(toCopy);
        written += toCopy;

        if (m_bufferFill == m_chunkSize) {
            bool const ok = encryptAndWriteChunk(reinterpret_cast<const char *>(m_buffer.data()), m_bufferFill);

            // Scrubbed unconditionally, a chunk that failed to make it downstream is not a reason to leave its plaintext
            // sitting around any longer than one that succeeded.
            sodium_memzero(m_buffer.data(), m_buffer.size());
            m_bufferFill = 0;

            if (!ok) {
                // Only THIS call's contribution to the doomed chunk gets un-reported.
                return written - toCopy;
            }
        }
    }

    return written;
}

JobZstdEncryptingTransport::int_type JobZstdEncryptingTransport::overflow(int_type ch)
{
    if (traits_type::eq_int_type(ch, traits_type::eof()))
        return traits_type::not_eof(ch);

    char const c = traits_type::to_char_type(ch);
    return (xsputn(&c, 1) == 1) ? ch : traits_type::eof();
}

bool JobZstdEncryptingTransport::finish()
{
    if (m_finished)
        return !m_encodeFailed;

    m_finished = true;

    if (m_encodeFailed)
        return false;

    if (m_bufferFill > 0) {
        bool const ok = encryptAndWriteChunk(reinterpret_cast<const char *>(m_buffer.data()), m_bufferFill);
        sodium_memzero(m_buffer.data(), m_buffer.size());
        m_bufferFill = 0;

        if (!ok)
            return false;
    }

    m_downstreamOut.flush();

    if (!m_downstreamOut) {
        m_encodeFailed = true;
        m_errorString = "Downstream flush failed during finish().";
        return false;
    }

    return true;
}

} // namespace job::zstd