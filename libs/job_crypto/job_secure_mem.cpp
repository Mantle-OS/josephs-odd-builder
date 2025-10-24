#include "job_secure_mem.h"
#include <iostream>
#include <sodium.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include "job_crypto_init.h"
namespace job::crypto {

JobSecureMem::JobSecureMem(size_t size)
{
    // Ensure libsodium is ready before any allocation
    if (!JobCryptoInit::isInitialized()) {
        if (JobCryptoInit::initialize()) {
            std::cout << "[JobSecureMem] libsodium initialized (lazy init).\n";
        } else {
            std::cerr << "[JobSecureMem] ERROR: libsodium initialization failed — "
                         "secure memory unavailable.\n";
            m_data = nullptr;
            m_size = 0;
            return;
        }
    }

    if (allocate(size)) {
        std::cout << "[JobSecureMem] Allocated " << size << " bytes of secure memory.\n";
    } else {
        std::cerr << "[JobSecureMem] Failed to allocate secure memory (" << size << " bytes).\n";
    }
}

JobSecureMem::JobSecureMem(const JobSecureMem &other)
{
    if (other.m_data && other.m_size > 0) {
        if (allocate(other.m_size)) {
            std::cout << "[JobSecureMem] Allocated " << other.m_size << " bytes of secure memory.\n";
        } else {
            std::cerr << "[JobSecureMem] Failed to allocate secure memory (" << other.m_size << " bytes).\n";
        }
        copyFrom(other.m_data, other.m_size);
    }
}

JobSecureMem &JobSecureMem::operator=(const JobSecureMem &other)
{
    if (this != &other) {
        if (m_data)
            sodium_free(m_data);

        m_data = nullptr;
        m_size = 0;

        if (other.m_data && other.m_size > 0) {
            if (allocate(other.m_size)) {
                std::cout << "[JobSecureMem] Allocated " << other.m_size << " bytes of secure memory.\n";
            } else {
                std::cerr << "[JobSecureMem] Failed to allocate secure memory (" << other.m_size << " bytes).\n";
            }
            copyFrom(other.m_data, other.m_size);
        }
    }
    return *this;
}

JobSecureMem::JobSecureMem(JobSecureMem &&other) noexcept
    : m_data(other.m_data), m_size(other.m_size)
{
    other.m_data = nullptr;
    other.m_size = 0;
}

JobSecureMem &JobSecureMem::operator=(JobSecureMem &&other) noexcept
{
    if (this != &other) {
        if (m_data)
            sodium_free(m_data);

        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

JobSecureMem::~JobSecureMem()
{
    if (m_data)
        sodium_free(m_data);
}


bool JobSecureMem::allocate(size_t size)
{
    if (m_data)
        sodium_free(m_data);

    if (size == 0) {
        m_data = nullptr;
        m_size = 0;
        return true;
    }

    m_data = static_cast<unsigned char *>(sodium_malloc(size));
    if (!m_data)
        return false;

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
        throw std::runtime_error("JobSecureMem: copy exceeds buffer");

    std::memcpy(m_data, src, len);
}

unsigned char *JobSecureMem::data() noexcept { return m_data; }
const unsigned char *JobSecureMem::data() const noexcept{ return m_data; }

size_t JobSecureMem::size() const noexcept { return m_size; }

void JobSecureMem::clear() noexcept
{
    if (m_data)
        sodium_memzero(m_data, m_size);
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

#if defined(JOB_SECUREMEM_ALLOW_STRING)

// Supported encodings for string conversion
enum class SecureStringEncoding {
    Base64,
    Hex,
    Printable // fallback for human-readable ASCII
};

static constexpr SecureStringEncoding kDefaultEncoding = SecureStringEncoding::Base64;

std::string JobSecureMem::toString() const
{
    if (!m_data || m_size == 0)
        return {};

    std::string out;

    switch (kDefaultEncoding) {
    case SecureStringEncoding::Base64: {
        size_t encoded_len = sodium_base64_encoded_len(m_size, sodium_base64_VARIANT_ORIGINAL);
        out.resize(encoded_len, '\0');
        sodium_bin2base64(out.data(), out.size(), m_data, m_size, sodium_base64_VARIANT_ORIGINAL);
        out.resize(std::strlen(out.c_str()));
        break;
    }
    case SecureStringEncoding::Hex: {
        size_t encoded_len = m_size * 2 + 1;
        out.resize(encoded_len, '\0');
        sodium_bin2hex(out.data(), out.size(), m_data, m_size);
        out.resize(std::strlen(out.c_str()));
        break;
    }
    case SecureStringEncoding::Printable: {
        out.reserve(m_size);
        for (size_t i = 0; i < m_size; ++i)
            out.push_back(std::isprint(m_data[i]) ? static_cast<char>(m_data[i]) : '.');
        break;
    }
    }

    return out;
}

#else

std::string JobSecureMem::toString() const
{
    throw std::runtime_error("JobSecureMem::toString() disabled in secure builds");
}

#endif // JOB_SECUREMEM_ALLOW_STRING

bool JobSecureMem::operator==(const JobSecureMem &other) const noexcept
{
    if (m_size != other.m_size)
        return false;
    return sodium_memcmp(m_data, other.m_data, m_size) == 0;
}

bool JobSecureMem::operator!=(const JobSecureMem &other) const noexcept
{
    return !(*this == other);
}

std::string JobSecureMem::toBase64(int variant) const
{
    if (!m_data || !m_size)
        return {};

    // find actual used bytes if buffer partially filled
    size_t used = m_size;
    while (used > 0 && m_data[used - 1] == 0)
        --used;

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

    // Max decoded size is roughly (n * 3)/4. Add a small cushion.
    const size_t max_dec = (encoded.size() * 3) / 4 + 3;
    std::vector<unsigned char> temp(max_dec);

    size_t dec_len = 0;
    if (sodium_base642bin(temp.data(), temp.size(),
                          encoded.c_str(), encoded.size(),
                          /*ignore*/ nullptr, &dec_len, /*bits*/ nullptr,
                          variant) != 0) {
        return false;
    }

    // Ensure we have enough room; if not, resize securely
    if (dec_len > m_size) {
        if (!allocate(dec_len))  // frees old, allocates new, zeros it
            return false;
    } else if (!m_data) {
        // If m_size is zero or no buffer yet, allocate exactly dec_len
        if (!allocate(dec_len))
            return false;
    }

    std::memcpy(m_data, temp.data(), dec_len);
    // If you want m_size to always reflect "used", adopt this:
    // (since we don't track "used length" separately)
    if (dec_len != m_size) {
        // shrink to fit so size() reflects actual decoded size
        // NOTE: sodium_malloc has no "realloc", so reallocate:
        JobSecureMem tmp(dec_len);
        std::memcpy(tmp.m_data, m_data, dec_len);
        *this = std::move(tmp);
    }
    return true;
}
std::string JobSecureMem::fromBase64toString(const std::string &encoded, int variant) const
{
    unsigned char decoded[512];
    size_t outLen = 0;

    if (sodium_base642bin(decoded, sizeof(decoded),
                          encoded.c_str(), encoded.size(),
                          nullptr, &outLen, nullptr,
                          variant) == 0)
    {
        return std::string(reinterpret_cast<char*>(decoded),
                           reinterpret_cast<char*>(decoded) + outLen);
    }
    return {};
}
} // namespace job::crypto
