#include "mock_io.h"

#include <algorithm>
// FIXME this was all wrote before we had a threading lib
namespace job::io {

MockIO::MockIO() = default;

bool MockIO::openDevice()
{
    std::lock_guard lock(m_mutex);
    m_isOpen = true;
    return true;
}

void MockIO::closeDevice()
{
    std::lock_guard lock(m_mutex);
    m_isOpen = false;
    m_readBuffer.clear();
    m_writeBuffer.clear();
}

bool MockIO::isOpen() const
{
    std::lock_guard lock(m_mutex);
    return m_isOpen;
}

ssize_t MockIO::read(char *buffer, size_t maxlen)
{
    std::lock_guard lock(m_mutex);
    if (!m_isOpen || m_readBuffer.empty())
        return 0;

    size_t count = std::min(maxlen, m_readBuffer.size());
    for (size_t i = 0; i < count; ++i) {
        buffer[i] = m_readBuffer.front();
        m_readBuffer.pop_front();
    }
    return static_cast<ssize_t>(count);
}

ssize_t MockIO::write(const char *data, size_t len)
{
    std::lock_guard lock(m_mutex);
    if (!m_isOpen || data == nullptr || len == 0)
        return -1;

    m_writeBuffer.append(data, len);
    return static_cast<ssize_t>(len);
}

bool MockIO::readable() const
{
    std::lock_guard lock(m_mutex);
    return m_isOpen && !m_readBuffer.empty();
}

bool MockIO::writable() const
{
    std::lock_guard lock(m_mutex);
    return m_isOpen;
}

void MockIO::setReadBuffer(const std::string &data)
{
    std::lock_guard lock(m_mutex);
    m_readBuffer.clear();
    m_readBuffer.insert(m_readBuffer.end(), data.begin(), data.end());
}

void MockIO::appendReadBuffer(const std::string &data)
{
    std::lock_guard lock(m_mutex);
    m_readBuffer.insert(m_readBuffer.end(), data.begin(), data.end());
}

void MockIO::clearReadBuffer()
{
    std::lock_guard lock(m_mutex);
    m_readBuffer.clear();
}

std::string MockIO::takeWriteBuffer()
{
    std::lock_guard lock(m_mutex);
    std::string result = std::move(m_writeBuffer);
    m_writeBuffer.clear();
    return result;
}

std::string MockIO::peekWriteBuffer() const
{
    std::lock_guard lock(m_mutex);
    return m_writeBuffer;
}

} // namespace job::io
// CHECKPOINT: v1
