#include "file_io.h"
#include <fcntl.h>

namespace job::io {

FileIO::FileIO(const std::string &path, FileMode mode, bool writeMode) :
    m_filePath{path},
    m_mode{mode},
    m_writeMode{writeMode}
{

}

FileIO::~FileIO()
{
    if (m_open)
        closeDevice();

    if(m_stdOut)
        m_stdOut = nullptr;

    if(m_stdIn)
        m_stdIn = nullptr;
}

bool FileIO::openDevice()
{
    if (m_open)
        return true;

    switch (m_mode) {
    case FileMode::RegularFile:
        if (m_writeMode) {
            m_output.open(m_filePath, std::ios::out | std::ios::binary | std::ios::trunc);
            m_open = m_output.is_open();
        } else {
            m_input.open(m_filePath, std::ios::in | std::ios::binary);
            m_open = m_input.is_open();
        }
        break;

    case FileMode::StdIn:
        m_stdIn = &std::cin;
        m_open = true;
        break;

    case FileMode::StdOut:
        m_stdOut = &std::cout;
        m_open = true;
        break;

    case FileMode::StdErr:
        m_stdOut = &std::cerr;
        m_open = true;
        break;
    }

    return m_open;
}

void FileIO::closeDevice()
{
    std::lock_guard<std::mutex> lock(m_ioMutex);

    if (!m_open)
        return;

    switch (m_mode) {
    case FileMode::RegularFile:
        if (m_writeMode && m_output.is_open()) {
            m_output.flush(); // ensure final flush before close
            m_output.close();
        }
        if (!m_writeMode && m_input.is_open())
            m_input.close();
        break;

    case FileMode::StdIn:
    case FileMode::StdOut:
    case FileMode::StdErr:
        // std::cin/cout/cerr should not be closed
        break;
    }

    m_open = false;
}

bool FileIO::flush()
{
    std::lock_guard<std::mutex> lock(m_ioMutex);
    bool ret = true; // assume success unless proven otherwise

    if (!m_open)
        return true; // no-op success

    switch (m_mode) {
    case FileMode::RegularFile:
        if (m_writeMode && m_output.is_open()) {
            m_output.flush();
            ret = !m_output.fail();
        }
        break;

    case FileMode::StdOut:
    case FileMode::StdErr:
        if (m_stdOut) {
            m_stdOut->flush();
            ret = true;
        }
        break;

    default:
        break;
    }

    return ret;
}

ssize_t FileIO::read(char *buffer, size_t size)
{
    if (!m_open)
        return -1;

    std::lock_guard<std::mutex> lock(m_ioMutex);

    switch (m_mode) {
    case FileMode::RegularFile:
        if (m_input.read(buffer, size))
            return m_input.gcount();
        else
            return m_input.gcount(); // could be 0 at EOF
    case FileMode::StdIn:
        m_stdIn->read(buffer, size);
        return m_stdIn->gcount();
    default:
        return -1; // reading from stdout/stderr is invalid
    }
}

ssize_t FileIO::write(const char *data, size_t size)
{
    if (!m_open)
        return -1;

    std::lock_guard<std::mutex> lock(m_ioMutex);

    switch (m_mode) {
    case FileMode::RegularFile:
        if (m_output.write(data, size)) {
            return size;
        } else {
            return -1;
        }

    case FileMode::StdOut:
    case FileMode::StdErr:
        m_stdOut->write(data, size);
        m_stdOut->flush();
        return size;

    default:
        return -1; // writing to stdin is invalid
    }
}

bool FileIO::isOpen() const
{
    return m_open;
}

std::string FileIO::path() const
{
    return m_filePath;
}
int FileIO::fd() const {
    int ret = -1;
    switch (m_mode) {
    case FileMode::StdIn:
        ret = STDIN_FILENO;
        break;
    case FileMode::StdOut:
        ret = STDOUT_FILENO;
        break;
    case FileMode::StdErr:
        ret = STDERR_FILENO;
        break;
    case FileMode::RegularFile:
        // File streams do not expose file descriptor in a portable way.
        // On Linux, we can use fileno() + rdbuf() workaround, but it's brittle.
        break;
    }
    return ret;
}

void FileIO::setNonBlocking(bool enabled) {
    int fileDescriptor = fd();

    if (fileDescriptor < 0)
        return;

    int flags = fcntl(fileDescriptor, F_GETFL, 0);
    if (flags < 0)
        return;

    if (enabled)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    fcntl(fileDescriptor, F_SETFL, flags);
}

void FileIO::setReadCallback(ReadCallback cb) {
    // Not applicable for sync file I/O; store but do nothing
    m_readCallback = std::move(cb);
}


} // job::io

