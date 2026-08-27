#include "job_file.h"

#include <cerrno>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace job::io {

//////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////

JobFile::JobFile(std::filesystem::path path, Access access, OpenMode openMode) :
    m_path(std::move(path)),
    m_access(access),
    m_openMode(openMode)
{
}

JobFile::JobFile(int fd, bool owned)
{
    if (!initFd(fd, owned))
        reset();
}

JobFile::JobFile(FILE *fp, bool owned)
{
    if (!initFp(fp, owned))
        reset();
}

JobFile::~JobFile()
{
    reset();
}

JobFile::JobFile(JobFile &&other) noexcept
{
    moveFrom(std::move(other));
}

JobFile &JobFile::operator=(JobFile &&other) noexcept
{
    if (this != &other) {
        reset();
        moveFrom(std::move(other));
    }

    return *this;
}

//////////////////////////////////////////////////////////
// IODevice
//////////////////////////////////////////////////////////

bool JobFile::openDevice()
{
    if (isOpen())
        return true;

    if (!hasPath())
        return false;

    return openPath();
}

void JobFile::closeDevice()
{
    switch (m_handle) {
    case Handle::FileDescriptor:
        closeFd();
        break;

    case Handle::FilePointer:
        closeFp();
        break;

    case Handle::None:
        break;
    }

    m_fd = -1;
    m_fp = nullptr;
    m_handle = Handle::None;
    m_owned = false;
}

ssize_t JobFile::read(char *buffer, std::size_t maxlen)
{
    if (!isOpen() || buffer == nullptr)
        return -1;

    if (m_access == Access::WriteOnly)
        return -1;

    if (maxlen == 0)
        return 0;

    const ssize_t bytesRead = ::read(m_fd, buffer, maxlen);

    if (bytesRead > 0 && m_readCallback)
        m_readCallback(buffer, static_cast<std::size_t>(bytesRead));

    return bytesRead;
}

ssize_t JobFile::write(const char *data, std::size_t len)
{
    if (!isOpen() || data == nullptr)
        return -1;

    if (m_access == Access::ReadOnly)
        return -1;

    if (len == 0)
        return 0;

    return ::write(m_fd, data, len);
}

bool JobFile::isOpen() const
{
    return m_fd >= 0;
}

int JobFile::fd() const
{
    return m_fd;
}

void JobFile::setNonBlocking(bool enabled)
{
    m_nonBlocking = enabled;

    if (!isOpen())
        return;

    const int flags = ::fcntl(m_fd, F_GETFL, 0);
    if (flags < 0)
        return;

    const int newFlags = enabled ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;

    if (::fcntl(m_fd, F_SETFL, newFlags) != 0)
        m_nonBlocking = !enabled;
}

void JobFile::setReadCallback(ReadCallback cb)
{
    m_readCallback = std::move(cb);
}

bool JobFile::flush()
{
    if (!isOpen())
        return true;

    if (m_handle == Handle::FilePointer && m_fp != nullptr)
        return ::fflush(m_fp) == 0;

    // POSIX ::write() has no JobFile-owned userspace buffer to flush.
    // Durability through fsync() is intentionally a separate concern.
    return true;
}

IOPermissions JobFile::permissions() const
{
    return m_permissions;
}

void JobFile::setPermissions(IOPermissions perms)
{
    if (isOpen() && ::fchmod(m_fd, toMode(perms)) != 0)
        return;

    IODevice::setPermissions(perms);
}

//////////////////////////////////////////////////////////
// File identity
//////////////////////////////////////////////////////////

const std::filesystem::path &JobFile::path() const noexcept
{
    return m_path;
}

std::string JobFile::pathString() const
{
    return m_path.string();
}

JobFile::Access JobFile::access() const noexcept
{
    return m_access;
}

JobFile::OpenMode JobFile::openMode() const noexcept
{
    return m_openMode;
}

JobFile::Handle JobFile::handle() const noexcept
{
    return m_handle;
}

bool JobFile::owned() const noexcept
{
    return m_owned;
}

bool JobFile::hasPath() const noexcept
{
    return !m_path.empty();
}

FILE *JobFile::fp() noexcept
{
    return m_fp;
}

const FILE *JobFile::fp() const noexcept
{
    return m_fp;
}

//////////////////////////////////////////////////////////
// File positioning / size
//////////////////////////////////////////////////////////

std::size_t JobFile::size() const
{
    if (!isOpen())
        return 0;

    struct stat info {};

    if (::fstat(m_fd, &info) != 0)
        return 0;

    if (info.st_size < 0)
        return 0;

    return static_cast<std::size_t>(info.st_size);
}

std::size_t JobFile::tell() const
{
    if (!isOpen())
        return std::numeric_limits<std::size_t>::max();

    const off_t offset = ::lseek(m_fd, 0, SEEK_CUR);

    if (offset < 0)
        return std::numeric_limits<std::size_t>::max();

    return static_cast<std::size_t>(offset);
}

bool JobFile::seek(std::int64_t offset, Seek whence)
{
    if (!isOpen())
        return false;

    int nativeWhence = SEEK_SET;

    switch (whence) {
    case Seek::Begin:
        nativeWhence = SEEK_SET;
        break;

    case Seek::Current:
        nativeWhence = SEEK_CUR;
        break;

    case Seek::End:
        nativeWhence = SEEK_END;
        break;
    }

    return ::lseek(m_fd, static_cast<off_t>(offset), nativeWhence) >= 0;
}

//////////////////////////////////////////////////////////
// Bulk IO
//////////////////////////////////////////////////////////

ssize_t JobFile::readAll(std::vector<std::uint8_t> &output)
{
    output.clear();

    if (!isOpen() || m_access == Access::WriteOnly)
        return -1;

    constexpr std::size_t kChunkSize = 64 * 1024;

    std::uint8_t buffer[kChunkSize];
    ssize_t total = 0;

    while (true) {
        const ssize_t bytesRead = read(reinterpret_cast<char *>(buffer), sizeof(buffer));

        if (bytesRead < 0) {
            if (errno == EINTR)
                continue;

            output.clear();
            return -1;
        }

        if (bytesRead == 0)
            break;

        if (bytesRead > std::numeric_limits<ssize_t>::max() - total) {
            output.clear();
            errno = EOVERFLOW;
            return -1;
        }

        output.insert(output.end(), buffer, buffer + bytesRead);
        total += bytesRead;
    }

    return total;
}

ssize_t JobFile::readAll(std::string &output)
{
    output.clear();

    if (!isOpen() || m_access == Access::WriteOnly)
        return -1;

    constexpr std::size_t kChunkSize = 64 * 1024;

    char buffer[kChunkSize];
    ssize_t total = 0;

    while (true) {
        const ssize_t bytesRead = read(buffer, sizeof(buffer));

        if (bytesRead < 0) {
            if (errno == EINTR)
                continue;

            output.clear();
            return -1;
        }

        if (bytesRead == 0)
            break;

        if (bytesRead > std::numeric_limits<ssize_t>::max() - total) {
            output.clear();
            errno = EOVERFLOW;
            return -1;
        }

        output.append(buffer, static_cast<std::size_t>(bytesRead));
        total += bytesRead;
    }

    return total;
}

//////////////////////////////////////////////////////////
// Open / close
//////////////////////////////////////////////////////////

bool JobFile::openPath()
{
    if (!hasPath())
        return false;

    if (m_access == Access::ReadOnly &&
        (m_openMode == OpenMode::Truncate || m_openMode == OpenMode::Append))
        return false;

    const int flags = openFlags();

    int fileDescriptor = -1;

    if ((flags & O_CREAT) != 0)
        fileDescriptor = ::open(m_path.c_str(), flags, toMode(m_permissions));
    else
        fileDescriptor = ::open(m_path.c_str(), flags);

    if (fileDescriptor < 0)
        return false;

    m_fd = fileDescriptor;
    m_fp = nullptr;
    m_handle = Handle::FileDescriptor;
    m_owned = true;

    if (m_nonBlocking) {
        const int currentFlags = ::fcntl(m_fd, F_GETFL, 0);

        if (currentFlags < 0 || ::fcntl(m_fd, F_SETFL, currentFlags | O_NONBLOCK) != 0) {
            closeDevice();
            return false;
        }
    }

    if (!updatePermissions()) {
        closeDevice();
        return false;
    }

    return true;
}

bool JobFile::initFd(int fd, bool owned)
{
    reset();

    m_fd = fd;
    m_fp = nullptr;
    m_handle = Handle::FileDescriptor;
    m_owned = owned;

    const int flags = ::fcntl(m_fd, F_GETFL, 0);

    if (flags < 0) {
        m_fd = -1;
        m_handle = Handle::None;
        m_owned = false;
        return false;
    }

    m_nonBlocking = (flags & O_NONBLOCK) != 0;

    switch (flags & O_ACCMODE) {
    case O_RDONLY:
        m_access = Access::ReadOnly;
        break;

    case O_WRONLY:
        m_access = Access::WriteOnly;
        break;

    case O_RDWR:
        m_access = Access::ReadWrite;
        break;
    }

    return updatePermissions();
}

bool JobFile::initFp(FILE *fp, bool owned)
{
    const int fileDescriptor = ::fileno(fp);

    if (fileDescriptor < 0)
        return false;

    reset();

    m_fd = fileDescriptor;
    m_fp = fp;
    m_handle = Handle::FilePointer;
    m_owned = owned;

    const int flags = ::fcntl(m_fd, F_GETFL, 0);

    if (flags < 0) {
        m_fd = -1;
        m_fp = nullptr;
        m_handle = Handle::None;
        m_owned = false;
        return false;
    }

    m_nonBlocking = (flags & O_NONBLOCK) != 0;

    switch (flags & O_ACCMODE) {
    case O_RDONLY:
        m_access = Access::ReadOnly;
        break;

    case O_WRONLY:
        m_access = Access::WriteOnly;
        break;

    case O_RDWR:
        m_access = Access::ReadWrite;
        break;
    }

    return updatePermissions();
}

void JobFile::closeFd() noexcept
{
    if (m_fd >= 0 && m_owned)
        ::close(m_fd);
}

void JobFile::closeFp() noexcept
{
    if (m_fp != nullptr && m_owned)
        ::fclose(m_fp);
}

//////////////////////////////////////////////////////////
// POSIX helpers
//////////////////////////////////////////////////////////

int JobFile::openFlags() const noexcept
{
    int flags = 0;

    switch (m_access) {
    case Access::ReadOnly:
        flags |= O_RDONLY;
        break;

    case Access::WriteOnly:
        flags |= O_WRONLY;
        break;

    case Access::ReadWrite:
        flags |= O_RDWR;
        break;
    }

    switch (m_openMode) {
    case OpenMode::OpenExisting:
        break;

    case OpenMode::Create:
        flags |= O_CREAT;
        break;

    case OpenMode::Truncate:
        flags |= O_CREAT | O_TRUNC;
        break;

    case OpenMode::Append:
        flags |= O_CREAT | O_APPEND;
        break;
    }

#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif

    return flags;
}

bool JobFile::updatePermissions() noexcept
{
    if (!isOpen())
        return false;

    struct stat info {};

    if (::fstat(m_fd, &info) != 0)
        return false;

    m_permissions = static_cast<IOPermissions>(info.st_mode & 07777);
    return true;
}

//////////////////////////////////////////////////////////
// Shared cleanup / move
//////////////////////////////////////////////////////////

void JobFile::reset() noexcept
{
    closeDevice();

    m_path.clear();

    m_access = Access::ReadOnly;
    m_openMode = OpenMode::OpenExisting;

    m_nonBlocking = false;
    m_readCallback = nullptr;
}

void JobFile::moveFrom(JobFile &&other) noexcept
{
    m_path = std::move(other.m_path);

    m_fd = other.m_fd;
    m_fp = other.m_fp;

    m_access = other.m_access;
    m_openMode = other.m_openMode;
    m_handle = other.m_handle;

    m_owned = other.m_owned;
    m_nonBlocking = other.m_nonBlocking;

    m_readCallback = std::move(other.m_readCallback);

    m_permissions = other.m_permissions;
    m_permissionsCallback = other.m_permissionsCallback;

    other.m_fd = -1;
    other.m_fp = nullptr;

    other.m_access = Access::ReadOnly;
    other.m_openMode = OpenMode::OpenExisting;
    other.m_handle = Handle::None;

    other.m_owned = false;
    other.m_nonBlocking = false;

    other.m_readCallback = nullptr;

    other.m_permissions = IOPermissions::DefaultFile;
    other.m_permissionsCallback = nullptr;
}

} // namespace job::io