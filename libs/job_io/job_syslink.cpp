#include "job_syslink.h"

#include <system_error>

namespace job::io {

JobSysLink::JobSysLink(Path path) :
    m_path(std::move(path))
{
}

const JobSysLink::Path &JobSysLink::path() const noexcept
{
    return m_path;
}

void JobSysLink::setPath(Path path)
{
    m_path = std::move(path);
}

bool JobSysLink::empty() const noexcept
{
    return m_path.empty();
}

bool JobSysLink::exists() const noexcept
{
    return exists(m_path);
}

bool JobSysLink::isSysLink() const noexcept
{
    return isSysLink(m_path);
}

bool JobSysLink::isDangling() const noexcept
{
    return isDangling(m_path);
}

JobSysLink::Path JobSysLink::target() const
{
    return target(m_path);
}

JobSysLink::Path JobSysLink::resolvedTarget() const
{
    return resolvedTarget(m_path);
}

JobSysLink::Path JobSysLink::canonicalTarget() const
{
    return canonicalTarget(m_path);
}

bool JobSysLink::create(const Path &target)
{
    return create(m_path, target);
}

bool JobSysLink::createFileLink(const Path &target)
{
    return createFileLink(m_path, target);
}

bool JobSysLink::createDirectoryLink(const Path &target)
{
    return createDirectoryLink(m_path, target);
}

bool JobSysLink::remove()
{
    return remove(m_path);
}

bool JobSysLink::exists(const Path &path) noexcept
{
    if (path.empty())
        return false;

    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, ec);

    if (ec)
        return false;

    /*
     * symlink_status() examines the directory entry itself rather than
     * following the link. This therefore remains true for a dangling symlink.
     */
    return status.type() != std::filesystem::file_type::not_found;
}

bool JobSysLink::isSysLink(const Path &path) noexcept
{
    if (path.empty())
        return false;

    std::error_code ec;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, ec);

    return !ec && std::filesystem::is_symlink(status);
}

bool JobSysLink::isDangling(const Path &path) noexcept
{
    if (!isSysLink(path))
        return false;

    std::error_code ec;

    /*
     * status() follows the symlink. If the link itself exists but its target
     * cannot be resolved to an existing filesystem entry, the link dangles.
     */
    const std::filesystem::file_status status = std::filesystem::status(path, ec);

    if (ec)
        return true;

    return status.type() == std::filesystem::file_type::not_found;
}

JobSysLink::Path JobSysLink::target(const Path &path)
{
    if (!isSysLink(path))
        return {};

    std::error_code ec;
    Path result = std::filesystem::read_symlink(path, ec);

    if (ec)
        return {};

    return result;
}

JobSysLink::Path JobSysLink::resolvedTarget(const Path &path)
{
    if (!isSysLink(path))
        return {};

    const Path linkTarget = target(path);

    if (linkTarget.empty())
        return {};

    /*
     * Absolute targets are already anchored.
     *
     * Relative symlink targets are interpreted relative to the directory
     * containing the symlink, not relative to the process working directory.
     */
    Path result;

    if (linkTarget.is_absolute())
        result = linkTarget;
    else
        result = path.parent_path() / linkTarget;

    return result.lexically_normal();
}

JobSysLink::Path JobSysLink::canonicalTarget(const Path &path)
{
    if (!isSysLink(path))
        return {};

    std::error_code ec;
    Path result = std::filesystem::canonical(path, ec);

    if (ec)
        return {};

    return result;
}

bool JobSysLink::create(const Path &path, const Path &target)
{
    /*
     * POSIX symbolic links themselves do not encode "file" versus
     * "directory". create_symlink() therefore provides the generic operation.
     *
     * create_directory_symlink() exists for platforms where the distinction
     * matters and is exposed separately below.
     */
    return createFileLink(path, target);
}

bool JobSysLink::createFileLink(const Path &path, const Path &target)
{
    if (path.empty() || target.empty())
        return false;

    /*
     * Never replace an existing filesystem entry implicitly. In particular,
     * exists() uses symlink_status() semantics so dangling symlinks are also
     * considered occupied.
     */
    if (exists(path))
        return false;

    std::error_code ec;
    std::filesystem::create_symlink(target, path, ec);

    return !ec;
}

bool JobSysLink::createDirectoryLink(const Path &path, const Path &target)
{
    if (path.empty() || target.empty())
        return false;

    if (exists(path))
        return false;

    std::error_code ec;
    std::filesystem::create_directory_symlink(target, path, ec);

    return !ec;
}

bool JobSysLink::remove(const Path &path)
{
    if (path.empty() || !isSysLink(path))
        return false;

    std::error_code ec;

    /*
     * remove() removes the symbolic-link directory entry itself. It does not
     * follow the link and therefore never removes the target.
     */
    const bool removed = std::filesystem::remove(path, ec);

    return removed && !ec;
}

} // namespace job::io