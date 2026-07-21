#pragma once

#include <streambuf>
#include <istream>
#include <string>

#include <job_secure_mem.h>
#include "jobzstd_export.h"
namespace job::zstd {
class JOBZSTD_EXPORT JobZstdDecryptingTransport : public std::streambuf
{
public:
    JobZstdDecryptingTransport(std::streambuf *downstream, const job::crypto::JobSecureMem &key);
    JobZstdDecryptingTransport( const JobZstdDecryptingTransport & ) = delete;
    JobZstdDecryptingTransport &operator=( const JobZstdDecryptingTransport & ) = delete;
    JobZstdDecryptingTransport( JobZstdDecryptingTransport && ) = delete;
    JobZstdDecryptingTransport &operator=( JobZstdDecryptingTransport && ) = delete;

    [[nodiscard]] bool atEnd() const noexcept;
    [[nodiscard]] bool wasTruncated() const noexcept { return m_truncated; }
    [[nodiscard]] bool hadAuthenticationError() const noexcept { return m_authFailed; }
    [[nodiscard]] const std::string &errorString() const noexcept { return m_errorString; }

protected:
    int_type        underflow() override;
    std::streamsize xsgetn(char *s, std::streamsize count) override;

private:
    // func
    [[nodiscard]] bool decodeNextChunk();
    // mem
    std::streambuf              *m_downstream;
    std::istream                m_downstreamIn;
    job::crypto::JobSecureMem   m_key;
    job::crypto::JobSecureMem   m_decryptedChunk;
    bool                        m_finished          = false;
    bool                        m_truncated         = false;
    bool                        m_authFailed        = false;
    std::string                 m_errorString;
};
} // namespace job::zstd