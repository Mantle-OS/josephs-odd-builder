#include "pty_io.h"

#include <fcntl.h>
#include <errno.h>
#include <cstring>

#include <sys/wait.h>
#include <poll.h>

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

    m_state = State::Opening;

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

    // Set default terminal settings
    ::tcsetattr(m_slaveFd, TCSANOW, &m_termios);
    ::ioctl(m_masterFd, TIOCSWINSZ, &m_winsize);

    if (m_nonBlocking)
        ::fcntl(m_masterFd, F_SETFL, O_NONBLOCK);

    m_state = State::Open;
    return true;
}

void PtyIO::closeDevice()
{
    if (m_state == State::Closed)
        return;

    m_state = State::Closing;

    stopReaderThread();

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
    m_state = State::Closed;
}


ssize_t PtyIO::read(char *buffer, size_t maxlen)
{
    if (m_masterFd == -1)
        return -1;
    return ::read(m_masterFd, buffer, maxlen);
}

ssize_t PtyIO::write(const char *data, size_t len)
{
    if (m_masterFd == -1)
        return -1;

    ssize_t written = ::write(m_masterFd, data, len);
    if (written == -1)
        setError(Error::WriteError);

    if (m_localecho && m_readCallback)
        m_readCallback(data, len);

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

        setenv("TERM", "xterm-256color", 1);
        execlp(shellPath.c_str(), shellPath.c_str(), "--login", "-i", nullptr);
        _exit(1);
    }

    close(m_slaveFd);
    m_slaveFd = -1;
    m_childPid = pid;
    m_state = State::Running;

    startReaderThread();
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
    m_localecho = enabled;
}

void PtyIO::setNonBlocking(bool enabled)
{
    m_nonBlocking = enabled;
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
    m_readCallback = std::move(cb);
}

void PtyIO::setExitCallback(ExitCallback cb)
{
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
    return m_state;
}

PtyIO::Error PtyIO::error() const
{
    return m_error;
}

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
        default:
            return "Unknown error";
    }
}

void PtyIO::setError(Error err)
{
    m_error = err;
    m_errorString = std::string(errorToString(err));
}

void PtyIO::startReaderThread()
{
    job::threads::JobThreadOptions opts = job::threads::JobThreadOptions::normal();
    opts.name = "PTY Reader";

    m_readerThread = std::make_shared<job::threads::JobThread>(opts);

    m_readerThread->setRunFunction([this](std::stop_token token) {
        this->readerLoop(token);
    });

    auto ret = m_readerThread->start();
    if (ret != job::threads::JobThread::StartResult::Started) {
        setError(Error::OpenFailed);
    }
}

void PtyIO::stopReaderThread()
{
    if (!m_readerThread)
        return;

    m_readerThread->requestStop();
    m_readerThread->join();
    m_readerThread.reset();
}

void PtyIO::readerLoop(std::stop_token token)
{
    constexpr int kPollTimeoutMs = 100;
    char buffer[1024];
    struct pollfd pfd;
    pfd.fd = m_masterFd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    while (!token.stop_requested()) {
        if (m_masterFd < 0)
            break;

        pfd.revents = 0;
        int pret = ::poll(&pfd, 1, kPollTimeoutMs);

        if (pret < 0) {
            if (errno == EINTR)
                continue;
            setError(Error::ReadError);
            break;
        }

        if (pret == 0)
            continue; // timeout → check m_running again

        if (pfd.revents & (POLLERR | POLLNVAL)) {
            setError(Error::ReadError);
            break;
        }
        if (pfd.revents & POLLHUP)
            break; // graceful EOF

        if (pfd.revents & POLLIN) {
            ssize_t n = ::read(m_masterFd, buffer, sizeof(buffer));

            if (n > 0) {
                if (m_readCallback)
                    m_readCallback(buffer, static_cast<size_t>(n));
            } else if (n == 0) {
                break; // EOF — slave closed
            } else if (errno != EAGAIN && errno != EINTR) {
                setError(Error::ReadError);
                break;
            }
        }

        // Read our own child only
        if (m_childPid > 0) {
            int status = 0;
            pid_t result = ::waitpid(m_childPid, &status, WNOHANG);
            if (result == m_childPid) {
                m_childPid = -1;
                if (m_exitCallback)
                    m_exitCallback(status);
                break;
            }
        }
    }
}

} // job::io
