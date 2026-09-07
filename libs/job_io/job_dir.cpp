#include "job_dir.h"

#include <cerrno>
#include <system_error>

#include <unistd.h>

namespace job::io {

JobDir::JobDir(Path path) :
    m_path(std::move(path))
{
}

const JobDir::Path &JobDir::path() const noexcept
{
    return m_path;
}

void JobDir::setPath(Path path)
{
    m_path = std::move(path);
}

bool JobDir::empty() const noexcept
{
    return m_path.empty();
}

bool JobDir::exists() const noexcept
{
    return exists(m_path);
}

bool JobDir::isDirectory() const noexcept
{
    return isDirectory(m_path);
}

bool JobDir::isEmpty() const noexcept
{
    return isEmpty(m_path);
}

bool JobDir::isReadable() const noexcept
{
    if (m_path.empty())
        return false;

    return ::access(m_path.c_str(), R_OK) == 0;
}

bool JobDir::isWritable() const noexcept
{
    if (m_path.empty())
        return false;

    return ::access(m_path.c_str(), W_OK) == 0;
}

bool JobDir::isExecutable() const noexcept
{
    if (m_path.empty())
        return false;

    return ::access(m_path.c_str(), X_OK) == 0;
}

bool JobDir::isAbsolute() const noexcept
{
    return !m_path.empty() && m_path.is_absolute();
}

bool JobDir::isRelative() const noexcept
{
    return !m_path.empty() && m_path.is_relative();
}

bool JobDir::create()
{
    return create(m_path);
}

bool JobDir::create(IOPermissions permissions)
{
    return create(m_path, permissions);
}

bool JobDir::createParents()
{
    return createParents(m_path);
}

bool JobDir::createParents(IOPermissions permissions)
{
    return createParents(m_path, permissions);
}

bool JobDir::remove()
{
    return remove(m_path);
}

bool JobDir::removeRecursive()
{
    return removeRecursive(m_path);
}

JobDir::PathList JobDir::entries() const
{
    PathList result;

    if (!isDirectory())
        return result;

    std::error_code ec;

    for (std::filesystem::directory_iterator it(m_path, ec), end; !ec && it != end; it.increment(ec))
        result.push_back(it->path());

    if (ec)
        result.clear();

    return result;
}

JobDir::PathList JobDir::files() const
{
    PathList result;

    if (!isDirectory())
        return result;

    std::error_code ec;

    for (std::filesystem::directory_iterator it(m_path, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && !ec)
            result.push_back(it->path());

        if (ec)
            break;
    }

    if (ec)
        result.clear();

    return result;
}

JobDir::PathList JobDir::directories() const
{
    PathList result;

    if (!isDirectory())
        return result;

    std::error_code ec;

    for (std::filesystem::directory_iterator it(m_path, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_directory(ec) && !ec)
            result.push_back(it->path());

        if (ec)
            break;
    }

    if (ec)
        result.clear();

    return result;
}

IOPermissions JobDir::permissions() const noexcept
{
    if (m_path.empty())
        return IOPermissions::None;

    struct stat info {};

    if (::stat(m_path.c_str(), &info) == -1)
        return IOPermissions::None;

    return static_cast<IOPermissions>(info.st_mode & permissions::kPermissionMask);
}

bool JobDir::setPermissions(IOPermissions permissions) noexcept
{
    if (m_path.empty())
        return false;

    return ::chmod(m_path.c_str(), toMode(permissions)) == 0;
}

bool JobDir::hasPermissions(IOPermissions permissions) const noexcept
{
    if (m_path.empty())
        return false;

    struct stat info {};

    if (::stat(m_path.c_str(), &info) == -1)
        return false;

    const PermissionBits current = info.st_mode & permissions::kPermissionMask;
    return current == toMode(permissions);
}

bool JobDir::exists(const Path &path) noexcept
{
    if (path.empty())
        return false;

    std::error_code ec;
    const bool result = std::filesystem::exists(path, ec);

    return result && !ec;
}

bool JobDir::isDirectory(const Path &path) noexcept
{
    if (path.empty())
        return false;

    std::error_code ec;
    const bool result = std::filesystem::is_directory(path, ec);

    return result && !ec;
}

bool JobDir::isEmpty(const Path &path) noexcept
{
    if (!isDirectory(path))
        return false;

    std::error_code ec;
    const bool result = std::filesystem::is_empty(path, ec);

    return result && !ec;
}

bool JobDir::create(const Path &path)
{
    if (path.empty())
        return false;

    std::error_code ec;

    if (std::filesystem::exists(path, ec))
        return !ec && std::filesystem::is_directory(path, ec) && !ec;

    return std::filesystem::create_directory(path, ec) && !ec;
}

bool JobDir::create(const Path &path, IOPermissions permissions)
{
    if (path.empty())
        return false;

    std::error_code ec;

    if (std::filesystem::exists(path, ec))
        return !ec && std::filesystem::is_directory(path, ec) && !ec;

    if (!std::filesystem::create_directory(path, ec) || ec)
        return false;

    /*
     * mkdir/create_directory is subject to the process umask. Force the
     * requested final mode only for the directory created by this call.
     */
    if (::chmod(path.c_str(), toMode(permissions)) == -1) {
        std::filesystem::remove(path, ec);
        return false;
    }

    return true;
}

bool JobDir::createParents(const Path &path)
{
    if (path.empty())
        return false;

    std::error_code ec;

    if (std::filesystem::exists(path, ec))
        return !ec && std::filesystem::is_directory(path, ec) && !ec;

    return std::filesystem::create_directories(path, ec) && !ec;
}

bool JobDir::createParents(const Path &path, IOPermissions permissions)
{
    if (path.empty())
        return false;

    std::error_code ec;

    if (std::filesystem::exists(path, ec))
        return !ec && std::filesystem::is_directory(path, ec) && !ec;

    Path current = path.root_path();

    for (const auto &component : path.relative_path()) {
        current /= component;

        if (std::filesystem::exists(current, ec)) {
            if (ec || !std::filesystem::is_directory(current, ec) || ec)
                return false;

            continue;
        }

        if (!std::filesystem::create_directory(current, ec) || ec)
            return false;

        if (::chmod(current.c_str(), toMode(permissions)) == -1)
            return false;
    }

    return true;
}

bool JobDir::remove(const Path &path)
{
    if (path.empty())
        return false;

    std::error_code ec;
    const bool removed = std::filesystem::remove(path, ec);

    return removed && !ec;
}

bool JobDir::removeRecursive(const Path &path)
{
    if (path.empty())
        return false;

    std::error_code ec;
    const std::uintmax_t removed = std::filesystem::remove_all(path, ec);

    return removed > 0 && !ec;
}

} // namespace job::io