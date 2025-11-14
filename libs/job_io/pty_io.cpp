#include "pty_io.h"

#include <errno.h>
#include <cstring>
#include <utility>

#include <fcntl.h>

#include <sys/wait.h>
#include <poll.h>

#include <job_logger.h>
namespace job::io {

PtyIO::PtyIO()
{
    m_termios = makeDefaultTermios();
}

PtyIO::~PtyIO()
{
    closeDevice();
}

bool PtyIO::openDevice()
{
    if (m_state != State::Closed)
        return false;

    m_state.store(State::Opening);

    m_masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);
    if (m_masterFd == -1) {
        setError(Error::OpenFailed);
        return false;
    }

    if (::grantpt(m_masterFd) == -1) {
        setError(Error::GrantFailed);
        ::close(m_masterFd);
        m_masterFd = -1;
        return false;
    }

    if (::unlockpt(m_masterFd) == -1) {
        setError(Error::UnlockFailed);
        ::close(m_masterFd);
        m_masterFd = -1;
        return false;
    }

    char *name = ::ptsname(m_masterFd);
    if (!name) {
        setError(Error::SlaveOpenFailed);
        ::close(m_masterFd);
        m_masterFd = -1;
        return false;
    }

    m_slaveName = name;

    m_slaveFd = ::open(name, O_RDWR | O_NOCTTY);
    if (m_slaveFd == -1) {
        setError(Error::SlaveOpenFailed);
        ::close(m_masterFd);
        m_masterFd = -1;
        return false;
    }

    ::tcsetattr(m_slaveFd, TCSANOW, &m_termios);
    ::ioctl(m_masterFd, TIOCSWINSZ, &m_winsize);

    setNonBlocking(true);

    // Look mama the events are happening
    try {
        m_loop = std::make_shared<threads::JobIoAsyncThread>();
    } catch (const std::exception& e) {
        JOB_LOG_ERROR("[PtyIO] Failed to create JobIoAsyncThread: {}", e.what());
        setError(Error::LoopError);
        ::close(m_masterFd);
        ::close(m_slaveFd);
        m_masterFd = -1;
        m_slaveFd = -1;
        return false;
    }

    // if (!m_loop->registerFD(m_masterFd, EPOLLIN, [this](uint32_t e) { onEvents(e); })) {
    if (!m_loop->registerFD(m_masterFd, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET, [this](uint32_t e) { onEvents(e); })) {

        JOB_LOG_ERROR("[PtyIO] Failed to register FD with event loop!");
        setError(Error::LoopError);
        ::close(m_masterFd);
        ::close(m_slaveFd);
        m_masterFd = -1;
        m_slaveFd = -1;
        return false;
    }
    m_loop->start();

    m_state.store(State::Open);
    return true;
}

void PtyIO::closeDevice()
{
    if (m_state == State::Closed)
        return;

    m_state.store(State::Closing);

    if (m_loop) {
        if (m_masterFd != -1)
            m_loop->unregisterFD(m_masterFd);
        m_loop->stop();
        m_loop.reset();
    }

    if (m_masterFd != -1) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }

    if (m_slaveFd != -1) {
        ::close(m_slaveFd);
        m_slaveFd = -1;
    }

    m_slaveName.clear();
    m_childPid = -1;
    m_state.store(State::Closed);
}


ssize_t PtyIO::read(char *buffer, size_t maxlen)
{
    if (m_masterFd == -1)
        return -1;

    ssize_t n = ::read(m_masterFd, buffer, maxlen);

    // "Dude where's my data"
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return 0;

    return n;
}

ssize_t PtyIO::write(const char *data, size_t len)
{
    if (m_masterFd == -1)
        return -1;

    ssize_t written = ::write(m_masterFd, data, len);
    if (written == -1)
        setError(Error::WriteError);

    return written;
}

int PtyIO::fd() const
{
    return m_masterFd;
}

bool PtyIO::isOpen() const
{
    return m_state == State::Open || m_state == State::Running;
}

bool PtyIO::startShell(const std::string &shellPath)
{
    if (!isOpen())
        return false;

    pid_t pid = fork();
    if (pid < 0) {
        setError(Error::ForkFailed);
        return false;
    }

    if (pid == 0) {
        setsid();
        if (ioctl(m_slaveFd, TIOCSCTTY, 0) < 0)
            _exit(1);

        dup2(m_slaveFd, STDIN_FILENO);
        dup2(m_slaveFd, STDOUT_FILENO);
        dup2(m_slaveFd, STDERR_FILENO);

        close(m_slaveFd);
        close(m_masterFd);

        m_loop->stop();

        setenv("TERM", "xterm-256color", 1);
        execlp(shellPath.c_str(), shellPath.c_str(), "--login", "-i", nullptr);
        _exit(1);
    }

    close(m_slaveFd);
    m_slaveFd = -1;
    m_childPid = pid;
    m_state.store(State::Running);

    m_loop->start();
    return true;
}

void PtyIO::setWindowSize(int rows, int cols)
{
    m_winsize.ws_row = rows;
    m_winsize.ws_col = cols;

    if (m_masterFd != -1) {
        if (ioctl(m_masterFd, TIOCSWINSZ, &m_winsize) == -1) {
            setError(Error::IoctlError);
        }
    }
}

void PtyIO::setLocalecho(bool enabled)
{
    m_localecho.store(enabled);
}

void PtyIO::setNonBlocking(bool enabled)
{
    m_nonBlocking.store(enabled); // Store our state
    if (m_masterFd != -1) {
        int flags = ::fcntl(m_masterFd, F_GETFL);
        if (enabled)
            ::fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);
        else
            ::fcntl(m_masterFd, F_SETFL, flags & ~O_NONBLOCK);
    }
}

void PtyIO::setReadCallback(ReadCallback cb)
{
    std::scoped_lock lock(m_cbMutex);
    m_readCallback = std::move(cb);
}

void PtyIO::setExitCallback(ExitCallback cb)
{
    std::scoped_lock lock(m_cbMutex);
    m_exitCallback = std::move(cb);
}

const std::string &PtyIO::slaveName() const
{
    return m_slaveName;
}

pid_t PtyIO::childPid() const
{
    return m_childPid;
}

PtyIO::State PtyIO::state() const
{
    return m_state.load();
}

PtyIO::Error PtyIO::error() const
{
    return m_error.load();
}


// Note: m_errorString is not thread-safe, but setError is.
const std::string &PtyIO::errorString() const
{
    return m_errorString;
        }

std::string_view PtyIO::errorToString(Error err)
{
    switch (err) {
        case Error::None:
            return "No error";
        case Error::OpenFailed:
            return "Failed to open PTY";
        case Error::GrantFailed:
            return "grantpt() failed";
        case Error::UnlockFailed:
            return "unlockpt() failed";
        case Error::SlaveOpenFailed:
            return "Failed to open slave PTY";
        case Error::ForkFailed:
            return "fork() failed";
        case Error::ExecFailed:
            return "exec() failed";
        case Error::WriteError:
            return "Write to PTY failed";
        case Error::ReadError:
            return "Read from PTY failed";
        case Error::IoctlError:
            return "ioctl() failed";
        case Error::LoopError:
            return "Event loop error";
        default:
            return "Unknown error";
    }
}

void PtyIO::setError(Error err)
{
    m_error.store(err);
    m_errorString = std::string(errorToString(err));
    if (err != Error::None)
        JOB_LOG_ERROR("[PtyIO] Error set: {}", m_errorString);
}


void PtyIO::onEvents(uint32_t events)
{
    if (events & (POLLERR | POLLNVAL)) {
        setError(Error::ReadError);
        closeDevice(); // Fatal error, shut down
        return;
    }
    if (events & POLLHUP) {
        closeDevice();
        return;
    }
    if (events & POLLIN) {
        char buffer[4096];
        while(true) {
            ssize_t n = ::read(m_masterFd, buffer, sizeof(buffer));
            if (n > 0) {
                std::scoped_lock lock(m_cbMutex);
                if (m_readCallback)
                    m_readCallback(buffer, static_cast<size_t>(n));
            } else if (n == 0) {
                closeDevice();
                break;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data to read
                break;
            } else if (errno != EINTR) {
                setError(Error::ReadError);
                closeDevice();
                break;
            }
        }
    }

    // kids are always a PITA
    if (m_childPid > 0) {
        int status = 0;

        pid_t result = ::waitpid(m_childPid, &status, WNOHANG);
        if (result == m_childPid) {
            m_childPid = -1;
            {
                std::scoped_lock lock(m_cbMutex);
                if (m_exitCallback)
                    m_exitCallback(status);
            }
            closeDevice();
        }

    }
}

} // job::io
// CHECKPOINT: v1.1
