#include "pty_dot.h"

namespace job::io {

PtyDot::PtyDot(std::string path,
               Role role,
               Format format,
               bool enabled) :
    m_enabled(enabled),
    m_role(role),
    m_format(format),
    m_path(std::move(path))
{
}

bool PtyDot::enabled() const noexcept
{
    return m_enabled;
}

PtyDot::Role PtyDot::role() const noexcept
{
    return m_role;
}

PtyDot::Format PtyDot::format() const noexcept
{
    return m_format;
}

std::string_view PtyDot::path() const noexcept
{
    return m_path;
}

const std::vector<unsigned char> &PtyDot::hash() const noexcept
{
    return m_hash;
}

bool PtyDot::hasHash() const noexcept
{
    return !m_hash.empty();
}

void PtyDot::setEnabled(bool enabled) noexcept
{
    m_enabled = enabled;
}

void PtyDot::setRole(Role role) noexcept
{
    m_role = role;
}

void PtyDot::setFormat(Format format) noexcept
{
    m_format = format;
}

void PtyDot::setPath(std::string path)
{
    if (m_path == path)
        return;

    m_path = std::move(path);
    m_hash.clear();
}

void PtyDot::setHash(std::vector<unsigned char> hash)
{
    m_hash = std::move(hash);
}

void PtyDot::clearHash() noexcept
{
    m_hash.clear();
}

} // namespace job::io