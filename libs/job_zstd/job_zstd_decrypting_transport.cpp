#include "job_zstd_decrypting_transport.h"

#include <cstring>
#include <cstddef>
#include <algorithm>
#include <vector>

#include <sodium/crypto_secretbox.h>

#include "job_zstd_wire.h"
#include "job_secret_box.h"

namespace job::zstd {
JobZstdDecryptingTransport::JobZstdDecryptingTransport(std::streambuf *downstream,
                                                       const job::crypto::JobSecureMem &key):
    m_downstream(downstream),
    m_downstreamIn(downstream),
    m_key(key)
{

}

// Well this was fun .....
bool JobZstdDecryptingTransport::decodeNextChunk()
{
    if (m_finished)
        return true;

    for (;;) {
        if (m_downstreamIn.peek() == std::istream::traits_type::eof()) {
            m_finished = true;
            return true;
        }

        std::string nonceStr;
        std::string cipherStr;

        if (!job::zstd::utils::readString(m_downstreamIn, nonceStr) || !job::zstd::utils::readString(m_downstreamIn, cipherStr)) {
            m_truncated = true;
            m_errorString = "Encrypted transport ended before a chunk frame was complete (truncated stream).";
            return false;
        }

        if (nonceStr.size() != crypto_secretbox_NONCEBYTES) {
            m_authFailed = true;
            m_errorString = "Encrypted chunk has a malformed nonce -- cannot be trusted as authentic.";
            return false;
        }

        std::vector<unsigned char> const nonceBytes(nonceStr.begin(), nonceStr.end());
        std::vector<unsigned char> const cipherBytes(cipherStr.begin(), cipherStr.end());

        if (!job::crypto::JobSecretBox::decrypt(cipherBytes, m_key, nonceBytes, m_decryptedChunk)) {
            m_authFailed = true;
            m_errorString = "Encrypted chunk failed authentication -- tampered, corrupted, or the wrong key.";
            return false;
        }

        if (!m_decryptedChunk.empty()) {
            if (m_downstreamIn.peek() == std::istream::traits_type::eof())
                m_finished = true;

            return true;
        }

        // Decrypted fine but produced nothing.
        // loop instead of assuming the stream is over. Our own encoder never writes this shape, but a different producer might.
    }
}

JobZstdDecryptingTransport::int_type JobZstdDecryptingTransport::underflow()
{
    if (gptr() < egptr())
        return traits_type::to_int_type(*gptr());

    if (m_truncated || m_authFailed)
        return traits_type::eof();

    if (m_finished)
        return traits_type::eof(); // Already known, from a previous call... no need to ask again.

    if (!decodeNextChunk())
        return traits_type::eof();

    if (m_decryptedChunk.empty())
        return traits_type::eof(); // This call found nothing (and may have just set m_finished).

    char *base = reinterpret_cast<char *>(m_decryptedChunk.data());
    setg(base, base, base + m_decryptedChunk.size());
    return traits_type::to_int_type(*gptr());
}

std::streamsize JobZstdDecryptingTransport::xsgetn(char *s, std::streamsize count)
{
    if (count <= 0)
        return 0;

    std::streamsize totalCopied = 0;

    if (gptr() < egptr()) {
        std::streamsize const avail = static_cast<std::streamsize>(egptr() - gptr());
        std::streamsize const n = std::min(avail, count);
        std::memcpy(s, gptr(), static_cast<std::size_t>(n));
        gbump(static_cast<int>(n));
        totalCopied += n;
    }

    while (totalCopied < count) {
        if (m_truncated || m_authFailed)
            break;

        if (m_finished)
            break;

        if (!decodeNextChunk())
            break;

        if (m_decryptedChunk.empty())
            break;

        std::streamsize const remaining = count - totalCopied;
        std::streamsize const take = std::min(remaining, static_cast<std::streamsize>(m_decryptedChunk.size()));

        std::memcpy(s + totalCopied, reinterpret_cast<const char *>(m_decryptedChunk.data()), static_cast<std::size_t>(take));
        totalCopied += take;

        if (static_cast<std::size_t>(take) < m_decryptedChunk.size()) {
            char *base = reinterpret_cast<char *>(m_decryptedChunk.data());
            setg(base, base + take, base + m_decryptedChunk.size());
            break;
        }
    }

    return totalCopied;
}

bool JobZstdDecryptingTransport::atEnd() const noexcept
{
    if (m_truncated || m_authFailed)
        return false; // an error state, not a clean end

    if (!gptr() || !egptr())
        return m_finished;

    return m_finished && gptr() >= egptr();
}

} // namespace job::zstd