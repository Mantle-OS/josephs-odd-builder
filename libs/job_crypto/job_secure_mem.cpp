#include "job_secure_mem.h"

#include <iostream>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <sodium.h>

#include "job_crypto_init.h"

namespace job::crypto {

JobSecureMem::JobSecureMem(size_t size)
{
    if (!JobCryptoInit::isInitialized()) {
        if (JobCryptoInit::initialize()) {
#ifndef NDEBUG
            std::clog << "[jobcrypto::JobSecureMem] Lazy initialized libsodium engine runtime natively.\n";
#endif
        } else {
            std::cerr << "[jobcrypto::JobSecureMem] CRITICAL ERROR: Runtime initialization failure. Memory unavailable.\n";
            m_data = nullptr;
            m_size = 0;
            return;
        }
    }

    if (size > 0) {
        if (allocate(size)) {
#ifndef NDEBUG
            std::clog << "[jobcrypto::JobSecureMem] Secured " << size << " byte page allocation row.\n";
#endif
        } else {
            std::cerr << "[jobcrypto::JobSecureMem] ERROR: Failed to lock down memory row for size: " << size << "\n";
        }
    }
}

JobSecureMem::JobSecureMem(const JobSecureMem &other)
{
    if (other.m_data && other.m_size > 0)
        if (allocate(other.m_size))
            copyFrom(other.m_data, other.m_size);
}

JobSecureMem &JobSecureMem::operator=(const JobSecureMem &other)
{
    if (this != &other) {
        JobSecureMem temp(other);
        this->swap(temp);
    }
    return *this;
}

JobSecureMem::JobSecureMem(JobSecureMem &&other) noexcept :
    m_data(other.m_data),
    m_size(other.m_size)
{
    other.m_data = nullptr;
    other.m_size = 0;
}

JobSecureMem &JobSecureMem::operator=(JobSecureMem &&other) noexcept
{
    if (this != &other) {
        free();
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

JobSecureMem::~JobSecureMem()
{
    free();
}

void JobSecureMem::swap(JobSecureMem &other) noexcept
{
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
}

bool JobSecureMem::allocate(size_t size) noexcept
{
    free();

    if (size == 0)
        return true;


    m_data = static_cast<unsigned char *>(sodium_malloc(size));
    if (!m_data) {
        m_size = 0;
        return false;
    }

    m_size = size;
    sodium_mlock(m_data, size);
    sodium_memzero(m_data, size);
    return true;
}

void JobSecureMem::copyFrom(const void *src, size_t len)
{
    if (len == 0)
        return;

    if (!m_data || len > m_size)
        throw std::runtime_error("JobSecureMem: copy exceeds buffer footprint boundaries.");

    std::memcpy(m_data, src, len);
}

unsigned char *JobSecureMem::data() noexcept
{
    return m_data;
}

const unsigned char *JobSecureMem::data() const noexcept
{
    return m_data;
}

size_t JobSecureMem::size() const noexcept
{
    return m_size;
}

void JobSecureMem::clear() noexcept
{
    if (m_data){
        sodium_memzero(m_data, m_size);
        free();
    }
    m_size = 0;
}

void JobSecureMem::free() noexcept
{
    if (m_data) {
        sodium_memzero(m_data, m_size);
        sodium_free(m_data);
        m_data = nullptr;
        m_size = 0;
    }
}

std::string JobSecureMem::toString() const
{
#if defined(JOB_SECUREMEM_ALLOW_STRING) && !defined(NDEBUG)
    if (!m_data || m_size == 0)
        return {};
    return std::string(reinterpret_cast<const char*>(m_data), m_size);
#else
    throw std::runtime_error("JobSecureMem::toString() disabled in production builds");
#endif
}

std::string JobSecureMem::toBase64(int variant) const
{
    if (!m_data || !m_size) return {};

    size_t used = m_size;
    while (used > 0 && m_data[used - 1] == 0) {
        --used;
    }

    if (used == 0)
        return {};

    size_t b64_len = sodium_base64_encoded_len(used, variant);
    std::string out(b64_len, '\0');
    sodium_bin2base64(out.data(), out.size(), m_data, used, variant);

    if (!out.empty() && out.back() == '\0')
        out.pop_back();

    return out;
}

bool JobSecureMem::fromBase64(const std::string &encoded, int variant)
{
    if (encoded.empty())
        return false;

    size_t const max_dec = (encoded.size() * 3) / 4 + 3;
    std::vector<unsigned char> temp(max_dec);

    size_t dec_len = 0;
    if (sodium_base642bin(temp.data(), temp.size(),
                          encoded.c_str(), encoded.size(),
                          nullptr, &dec_len, nullptr, variant) != 0) {
        return false;
    }

    if (!allocate(dec_len)) {
        return false;
    }
    if (dec_len > 0 && m_data)
        std::memcpy(m_data, temp.data(), dec_len);
    return true;
}

std::string JobSecureMem::fromBase64toString(const std::string &encoded, int variant) const
{
    if (encoded.empty())
        return {};

    size_t const max_dec = (encoded.size() * 3) / 4 + 3;
    JobSecureMem decodedBuffer(max_dec); // Allocated inside locked, non-swappable space

    size_t outLen = 0;
    if (sodium_base642bin(decodedBuffer.data(), decodedBuffer.size(),
                          encoded.c_str(), encoded.size(),
                          nullptr, &outLen, nullptr, variant) == 0)
    {
        return std::string(reinterpret_cast<char*>(decodedBuffer.data()), outLen);
    }
    return {};
}

bool JobSecureMem::operator==(const JobSecureMem &other) const noexcept
{
    if (m_size != other.m_size) return false;
    if (m_data == other.m_data) return true;
    return sodium_memcmp(m_data, other.m_data, m_size) == 0;
}

bool JobSecureMem::operator!=(const JobSecureMem &other) const noexcept
{
    return !(*this == other);
}

} // namespace job::crypto