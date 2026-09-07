#include "pty_io.h"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <utility>
#include <vector>
#include <poll.h>
#include <fcntl.h>

#include <sys/syscall.h>
#include <sys/wait.h>

#include <job_logger.h>

namespace job::io {

static std::string resolveProgram(const std::string &program,
                                  const std::vector<std::string> &environment)
{
    if (program.empty() || program.find('/') != std::string::npos)
        return program;

    std::string path;

    for (const auto &entry : environment) {
        if (entry.starts_with("PATH=")) {
            path = entry.substr(5);
            break;
        }
    }

    if (path.empty())
        path = "/bin:/usr/bin";

    std::size_t begin = 0;

    while (begin <= path.size()) {
        const std::size_t end = path.find(':', begin);
        const std::string_view directory =
            end == std::string::npos
                ? std::string_view(path).substr(begin)
                : std::string_view(path).substr(begin, end - begin);

        std::string candidate;

        if (directory.empty())
            candidate = "./" + program;
        else
            candidate = std::string(directory) + "/" + program;

        if (::access(candidate.c_str(), X_OK) == 0)
            return candidate;

        if (end == std::string::npos)
            break;

        begin = end + 1;
    }

    /*
     * Preserve ordinary exec failure semantics. execve() will report ENOENT
     * and the child exits with 127.
     */
    return program;
}

static bool setCloseOnExec(int fd)
{
    const int flags = ::fcntl(fd, F_GETFD, 0);

    if (flags == -1)
        return false;

    return ::fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != -1;
}

static bool resetChildSignals() noexcept
{
    sigset_t mask{};

    if (::sigemptyset(&mask) == -1)
        return false;

    if (::sigprocmask(SIG_SETMASK, &mask, nullptr) == -1)
        return false;

    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    action.sa_flags   = 0;

    if (::sigemptyset(&action.sa_mask) == -1)
        return false;

    constexpr int signals[] = {
        SIGHUP,
        SIGINT,
        SIGQUIT,
        SIGPIPE,
        SIGTERM,
        SIGCHLD,
    };

    for (const int signal : signals) {
        if (::sigaction(signal, &action, nullptr) == -1)
            return false;
    }

    return true;
}

PtyIO::PtyIO() :
    m_termios(makeDefaultTermios())
{
    if (!buildEnviron())
        JOB_LOG_WARN("[PtyIO] Failed to build initial environment snapshot");
}

PtyIO::~PtyIO()
{
    /*
     * Nobody may call back into an object whose lifetime is ending.
     */
    {
        std::scoped_lock lock(m_cbMutex);
        m_readCallback = {};
        m_exitCallback = {};
    }

    /*
     * Explicit closeDevice() may detach the I/O lifetime from a still-running
     * process. Destruction cannot do that: after this object disappears there
     * would be nobody left responsible for reaping the child.
     */
    if (childPid() > 0) {
        if (!killProcess())
            JOB_LOG_WARN("[PtyIO] Failed to kill child {} during destruction", childPid());

        if (!reapChild(true))
            JOB_LOG_WARN("[PtyIO] Failed to reap child during destruction");
    }

    closeDevice();
}

bool PtyIO::openDevice()
{
    if (state() != State::Closed)
        return false;

    /*
     * An earlier explicit close may have detached the PTY while preserving an
     * owned child. Do not replace that process lifetime until waitpid() proves
     * the old child is gone.
     */
    if (childPid() > 0 && !reapChild(false))
        return false;

    std::shared_ptr<threads::JobIoAsyncThread> oldLoop;
    termios terminalAttributes{};
    winsize windowSize{};

    {
        std::scoped_lock lock(m_lifecycleMutex);

        if (m_state.load(std::memory_order_acquire) != State::Closed)
            return false;

        oldLoop = std::move(m_loop);

        terminalAttributes = m_termios;
        windowSize          = m_winsize;

        m_state.store(State::Opening, std::memory_order_release);
    }

    /*
     * A previous callback-driven close may have requested worker shutdown from
     * that worker itself. Finish joining it here, outside the lifecycle lock.
     */
    if (oldLoop)
        oldLoop->stop();

    setError(Error::None);

    const int masterFd = ::posix_openpt(O_RDWR | O_NOCTTY);

    if (masterFd == -1) {
        setError(Error::OpenFailed);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    if (::grantpt(masterFd) == -1) {
        setError(Error::GrantFailed);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    if (::unlockpt(masterFd) == -1) {
        setError(Error::UnlockFailed);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    char *name = ::ptsname(masterFd);

    if (!name) {
        setError(Error::SlaveOpenFailed);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    const std::string slaveName{name};

    const int slaveFd = ::open(name, O_RDWR | O_NOCTTY);

    if (slaveFd == -1) {
        setError(Error::SlaveOpenFailed);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    if (::tcsetattr(slaveFd, TCSANOW, &terminalAttributes) == -1) {
        setError(Error::IoctlError);
        ::close(slaveFd);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    if (::ioctl(masterFd, TIOCSWINSZ, &windowSize) == -1) {
        setError(Error::IoctlError);
        ::close(slaveFd);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    const int flags = ::fcntl(masterFd, F_GETFL, 0);

    if (flags == -1 || ::fcntl(masterFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        setError(Error::IoctlError);
        ::close(slaveFd);
        ::close(masterFd);
        m_nonBlocking.store(false, std::memory_order_release);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    m_nonBlocking.store(true, std::memory_order_release);

    std::shared_ptr<threads::JobIoAsyncThread> loop;

    try {
        loop = std::make_shared<threads::JobIoAsyncThread>();
    } catch (const std::exception &e) {
        JOB_LOG_ERROR("[PtyIO] Failed to create JobIoAsyncThread: {}", e.what());
        setError(Error::LoopError);
        ::close(slaveFd);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    if (!loop->registerFD(
            masterFd,
            threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered,
            [this](threads::IOEvent events) {
                onEvents(events);
            })) {
        JOB_LOG_ERROR("[PtyIO] Failed to register PTY master with event loop");
        setError(Error::LoopError);
        ::close(slaveFd);
        ::close(masterFd);
        m_state.store(State::Closed, std::memory_order_release);
        return false;
    }

    /*
     * Publish the complete PTY lifetime atomically with respect to closeDevice.
     * The worker is started while this lock is held, so an immediately-ready
     * descriptor cannot race an incompletely-published PtyIO.
     */
    {
        std::scoped_lock lock(m_lifecycleMutex);

        m_masterFd.store(masterFd, std::memory_order_release);
        m_slaveFd.store(slaveFd, std::memory_order_release);
        m_slaveName = slaveName;
        m_loop      = loop;

        loop->start();

        m_state.store(State::Open, std::memory_order_release);
    }

    return true;
}

void PtyIO::closeDevice()
{
    std::shared_ptr<threads::JobIoAsyncThread> loop;

    int masterFd = -1;
    int slaveFd  = -1;
    int pidFd    = -1;

    {
        std::scoped_lock lock(m_lifecycleMutex);

        if (m_state.load(std::memory_order_acquire) == State::Closed &&
            !m_loop &&
            m_masterFd.load(std::memory_order_acquire) == -1 &&
            m_slaveFd.load(std::memory_order_acquire) == -1 &&
            m_pidFd.load(std::memory_order_acquire) == -1) {
            return;
        }

        m_state.store(State::Closing, std::memory_order_release);

        loop = m_loop;

        /*
         * exchange() transfers ownership of each descriptor to exactly one
         * closing path. Concurrent closeDevice()/onEvents()/onPidEvents() calls
         * cannot close the same descriptor twice.
         */
        masterFd = m_masterFd.exchange(-1, std::memory_order_acq_rel);
        slaveFd  = m_slaveFd.exchange(-1, std::memory_order_acq_rel);
        pidFd    = m_pidFd.exchange(-1, std::memory_order_acq_rel);

        m_slaveName.clear();
    }

    if (loop) {
        if (masterFd != -1) {
            if (!loop->unregisterFD(masterFd))
                JOB_LOG_WARN("[PtyIO] Failed to unregister PTY master fd {}", masterFd);
        }

        if (pidFd != -1) {
            if (!loop->unregisterFD(pidFd))
                JOB_LOG_WARN("[PtyIO] Failed to unregister pidfd {}", pidFd);
        }

        /*
         * stop() already understands the worker-self case.
         */
        loop->stop();
    }

    if (masterFd != -1)
        ::close(masterFd);

    if (slaveFd != -1)
        ::close(slaveFd);

    if (pidFd != -1)
        ::close(pidFd);

    /*
     * Deliberately preserve m_childPid. Explicit I/O closure and process
     * ownership are separate lifetimes. Only waitpid() clears the child PID.
     */
    m_state.store(State::Closed, std::memory_order_release);
}

ssize_t PtyIO::read(char *buffer, std::size_t maxlen)
{
    if (!buffer)
        return -1;

    if (maxlen == 0)
        return 0;

    constexpr int POLL_TIMEOUT_MS = 50;

    while (true) {
        int masterFd = -1;

        {
            std::scoped_lock lock(m_lifecycleMutex);
            masterFd = m_masterFd.load(std::memory_order_acquire);
        }

        if (masterFd == -1)
            return -1;

        const ssize_t bytesRead = ::read(masterFd, buffer, maxlen);

        if (bytesRead >= 0)
            return bytesRead;

        if (errno == EINTR)
            continue;

        if (errno == EIO)
            return 0;

        if (errno == EBADF)
            return -1;

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            setError(Error::ReadError);
            return -1;
        }

        if (m_nonBlocking.load(std::memory_order_acquire))
            return 0;

        pollfd pfd{
            .fd = masterFd,
            .events = POLLIN,
            .revents = 0,
        };

        while (true) {
            const int ready = ::poll(&pfd, 1, POLL_TIMEOUT_MS);

            if (ready > 0)
                break;

            if (ready == 0) {
                if (m_masterFd.load(std::memory_order_acquire) != masterFd)
                    return -1;

                continue;
            }

            if (errno == EINTR)
                continue;

            if (errno == EBADF)
                return -1;

            setError(Error::ReadError);
            return -1;
        }

        if (pfd.revents & POLLNVAL)
            return -1;

        if (pfd.revents & (POLLERR | POLLHUP))
            return 0;
    }
}

ssize_t PtyIO::write(const char *data, std::size_t len)
{
    if (!data)
        return -1;

    if (len == 0)
        return 0;

    constexpr int POLL_TIMEOUT_MS = 50;

    while (true) {
        int masterFd = -1;

        {
            std::scoped_lock lock(m_lifecycleMutex);
            masterFd = m_masterFd.load(std::memory_order_acquire);
        }

        if (masterFd == -1)
            return -1;

        const ssize_t written = ::write(masterFd, data, len);

        if (written >= 0)
            return written;

        if (errno == EINTR)
            continue;

        if (errno == EBADF)
            return -1;

        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            setError(Error::WriteError);
            return -1;
        }

        if (m_nonBlocking.load(std::memory_order_acquire))
            return -1;

        pollfd pfd{
            .fd = masterFd,
            .events = POLLOUT,
            .revents = 0,
        };

        while (true) {
            const int ready = ::poll(&pfd, 1, POLL_TIMEOUT_MS);

            if (ready > 0)
                break;

            if (ready == 0) {
                if (m_masterFd.load(std::memory_order_acquire) != masterFd)
                    return -1;

                continue;
            }

            if (errno == EINTR)
                continue;

            if (errno == EBADF)
                return -1;

            setError(Error::WriteError);
            return -1;
        }

        if (pfd.revents & POLLNVAL)
            return -1;

        if (pfd.revents & (POLLERR | POLLHUP))
            return -1;
    }
}

int PtyIO::fd() const
{
    return m_masterFd.load(std::memory_order_acquire);
}

bool PtyIO::isOpen() const
{
    const State current = state();
    return current == State::Open || current == State::Running;
}

PtyEnviron &PtyIO::environ() noexcept
{
    return m_environ;
}

const PtyEnviron &PtyIO::environ() const noexcept
{
    return m_environ;
}

bool PtyIO::buildEnviron()
{
    if (m_environ.buildInitList())
        return true;

    setError(Error::EnvironmentError);
    return false;
}

bool PtyIO::startProcess(const std::string &program,
                         const std::vector<std::string> &arguments)
{
    if (program.empty())
        return false;

    /*
     * Everything the child needs after fork() is prepared beforehand.
     */
    std::vector<char *> argv;
    argv.reserve(arguments.size() + 2);
    argv.push_back(const_cast<char *>(program.c_str()));

    for (const auto &argument : arguments)
        argv.push_back(const_cast<char *>(argument.c_str()));

    argv.push_back(nullptr);

    std::vector<std::string> environment = m_environ.toStrings();

    std::vector<char *> envp;
    envp.reserve(environment.size() + 1);

    for (auto &entry : environment)
        envp.push_back(entry.data());

    envp.push_back(nullptr);

    const std::string resolvedProgram = resolveProgram(program, environment);

    int setupPipe[2]{-1, -1};

    if (::pipe(setupPipe) == -1) {
        setError(Error::ForkFailed);
        return false;
    }

    if (!setCloseOnExec(setupPipe[0]) || !setCloseOnExec(setupPipe[1])) {
        ::close(setupPipe[0]);
        ::close(setupPipe[1]);
        setError(Error::ForkFailed);
        return false;
    }

    pid_t pid = -1;

    {
        /*
         * Prevent closeDevice() from invalidating the PTY descriptors between
         * validation and fork().
         */
        std::scoped_lock lock(m_lifecycleMutex);

        if (!isOpen() ||
            m_state.load(std::memory_order_acquire) == State::Running) {
            ::close(setupPipe[0]);
            ::close(setupPipe[1]);
            return false;
        }

        const int masterFd = m_masterFd.load(std::memory_order_acquire);
        const int slaveFd  = m_slaveFd.load(std::memory_order_acquire);

        if (masterFd == -1 || slaveFd == -1) {
            ::close(setupPipe[0]);
            ::close(setupPipe[1]);
            return false;
        }

        pid = ::fork();

        if (pid < 0) {
            ::close(setupPipe[0]);
            ::close(setupPipe[1]);
            setError(Error::ForkFailed);
            return false;
        }

        if (pid == 0) {
            ::close(setupPipe[0]);

            /*
             * The caller may be a test runner, GUI application, daemon, or
             * anything else with custom signal handlers and blocked signals.
             * None of that application policy belongs in the launched child.
             */
            if (!resetChildSignals())
                ::_exit(126);

            if (::setsid() == -1)
                ::_exit(126);

            if (::ioctl(slaveFd, TIOCSCTTY, 0) == -1)
                ::_exit(126);

            if (::dup2(slaveFd, STDIN_FILENO) == -1)
                ::_exit(126);

            if (::dup2(slaveFd, STDOUT_FILENO) == -1)
                ::_exit(126);

            if (::dup2(slaveFd, STDERR_FILENO) == -1)
                ::_exit(126);

            if (slaveFd > STDERR_FILENO)
                ::close(slaveFd);

            ::close(masterFd);

            /*
             * Tell the parent that signal reset, setsid(), TIOCSCTTY and stdio
             * setup are complete. After startProcess() returns, the child is
             * therefore a proven session/process-group leader and no longer in
             * the inherited Catch2/application signal-handler window.
             */
            constexpr char READY = 1;

            if (::write(setupPipe[1], &READY, sizeof(READY)) != sizeof(READY))
                ::_exit(126);

            ::close(setupPipe[1]);

            ::execve(resolvedProgram.c_str(), argv.data(), envp.data());
            ::_exit(127);
        }

        /*
         * Parent no longer owns the slave side once the child exists.
         */
        const int parentSlaveFd = m_slaveFd.exchange(-1, std::memory_order_acq_rel);

        if (parentSlaveFd != -1)
            ::close(parentSlaveFd);

        m_childPid.store(pid, std::memory_order_release);
    }

    ::close(setupPipe[1]);

    char ready = 0;
    ssize_t setupRead = -1;

    while (true) {
        setupRead = ::read(setupPipe[0], &ready, sizeof(ready));

        if (setupRead == -1 && errno == EINTR)
            continue;

        break;
    }

    ::close(setupPipe[0]);

    if (setupRead != sizeof(ready) || ready != 1) {
        /*
         * The child died during pre-exec process setup.
         */
        int status = 0;

        while (true) {
            const pid_t result = ::waitpid(pid, &status, 0);

            if (result == pid)
                break;

            if (result == -1 && errno == EINTR)
                continue;

            if (result == -1 && errno == ECHILD)
                break;

            break;
        }

        m_childPid.store(-1, std::memory_order_release);
        setError(Error::ExecFailed);
        return false;
    }

    /*
     * Publish Running before pidfd registration. A very short-lived child may
     * already have exited; registering its pidfd can therefore invoke the
     * completion path immediately.
     */
    m_state.store(State::Running, std::memory_order_release);

    if (!openPidFd()) {
        /*
         * The child belongs to us, but without pidfd observation we refuse to
         * leave an asynchronously-owned execution behind.
         */
        const bool signalled = signalChild(SIGKILL);

        if (!signalled)
            JOB_LOG_WARN("[PtyIO] Failed to signal child {} after pidfd setup failure", childPid());

        const bool reaped = reapChild(true);

        if (!reaped)
            JOB_LOG_WARN("[PtyIO] Failed to reap child after pidfd setup failure");

        setError(Error::PidFdError);

        if (fd() >= 0)
            m_state.store(State::Open, std::memory_order_release);
        else
            m_state.store(State::Closed, std::memory_order_release);

        return false;
    }

    return true;
}

bool PtyIO::startShell(ShellType shellType)
{
    const std::string_view path = shellPath(shellType);

    if (path.empty())
        return false;

    return startShell(std::string(path));
}

bool PtyIO::startShell(const std::string &shellPath)
{
    if (shellPath.empty())
        return false;

    return startProcess(shellPath, {"-i"});
}

bool PtyIO::terminateProcess()
{
    return signalChild(SIGTERM);
}

bool PtyIO::killProcess()
{
    return signalChild(SIGKILL);
}

bool PtyIO::terminateProcess(int pid)
{
    return signalProcess(static_cast<pid_t>(pid), SIGTERM);
}

bool PtyIO::killProcess(int pid)
{
    return signalProcess(static_cast<pid_t>(pid), SIGKILL);
}

std::string_view PtyIO::shellPath(ShellType shellType) noexcept
{
    switch (shellType) {
    case ShellType::Sh:
        return "/bin/sh";
    case ShellType::Dash:
        return "/bin/dash";
    case ShellType::Bash:
        return "/bin/bash";
    case ShellType::Zsh:
        return "/bin/zsh";
    }

    return {};
}

bool PtyIO::signalChild(int signal)
{
    /*
     * Keep waitpid()/reaping from releasing this PID while we are deciding how
     * to signal it. A zombie PID cannot be reused until reaped.
     */
    std::scoped_lock lock(m_lifecycleMutex);

    const pid_t pid = m_childPid.load(std::memory_order_acquire);

    if (pid <= 0) {
        setError(Error::SignalError);
        return false;
    }

    const pid_t pgid = ::getpgid(pid);

    if (pgid == pid) {
        if (::kill(-pgid, signal) == 0)
            return true;

        const int signalError = errno;

        setError(Error::SignalError);
        JOB_LOG_WARN("[PtyIO] Failed to signal child process group {}: {}",
                     pgid, std::strerror(signalError));
        return false;
    }

    /*
     * The setup handshake means this should ordinarily never be needed after a
     * successful startProcess(), but retaining the concrete PID fallback keeps
     * cleanup robust for partial-start failure paths.
     */
    if (pgid == -1 && errno != ESRCH) {
        const int pgidError = errno;

        setError(Error::SignalError);
        JOB_LOG_WARN("[PtyIO] getpgid({}) failed: {}", pid, std::strerror(pgidError));
        return false;
    }

    if (::kill(pid, signal) == 0)
        return true;

    const int signalError = errno;

    setError(Error::SignalError);
    JOB_LOG_WARN("[PtyIO] Failed to signal child {}: {}", pid, std::strerror(signalError));
    return false;
}

bool PtyIO::signalProcess(pid_t pid, int signal)
{
    if (pid <= 0) {
        setError(Error::SignalError);
        return false;
    }

    /*
     * Explicit numeric PID means exactly that PID. No process-group inference
     * is made for externally-supplied identifiers.
     */
    if (::kill(pid, signal) == 0)
        return true;

    const int signalError = errno;

    setError(Error::SignalError);
    JOB_LOG_WARN("[PtyIO] Failed to signal process {}: {}", pid, std::strerror(signalError));
    return false;
}

bool PtyIO::openPidFd()
{
    std::scoped_lock lock(m_lifecycleMutex);

    const pid_t pid = m_childPid.load(std::memory_order_acquire);

    if (pid <= 0 || !m_loop) {
        setError(Error::PidFdError);
        return false;
    }

    const int pidFd = static_cast<int>(::syscall(SYS_pidfd_open, pid, 0));

    if (pidFd == -1) {
        setError(Error::PidFdError);
        JOB_LOG_WARN("[PtyIO] pidfd_open({}) failed: {}", pid, std::strerror(errno));
        return false;
    }

    /*
     * Publish before registration while holding the lifecycle lock. If the
     * pidfd is already readable, its worker callback cannot run the reap path
     * until this publication is complete.
     */
    m_pidFd.store(pidFd, std::memory_order_release);

    if (!m_loop->registerFD(
            pidFd,
            threads::IOEvent::Read |
                threads::IOEvent::Error |
                threads::IOEvent::HangUp |
                threads::IOEvent::EdgeTriggered,
            [this](threads::IOEvent events) {
                onPidEvents(events);
            })) {
        m_pidFd.store(-1, std::memory_order_release);
        ::close(pidFd);

        JOB_LOG_WARN("[PtyIO] Failed to register pidfd {} for child {}", pidFd, pid);
        setError(Error::PidFdError);
        return false;
    }

    return true;
}

void PtyIO::closePidFd() noexcept
{
    std::shared_ptr<threads::JobIoAsyncThread> loop;
    int pidFd = -1;

    {
        std::scoped_lock lock(m_lifecycleMutex);

        pidFd = m_pidFd.exchange(-1, std::memory_order_acq_rel);
        loop  = m_loop;
    }

    if (pidFd == -1)
        return;

    if (loop) {
        if (!loop->unregisterFD(pidFd))
            JOB_LOG_WARN("[PtyIO] Failed to unregister pidfd {}", pidFd);
    }

    ::close(pidFd);
}

bool PtyIO::reapChild(bool wait)
{
    int status      = 0;
    bool hasStatus  = false;
    pid_t pid       = -1;

    {
        /*
         * Serialize waitpid against signalChild and other reap attempts.
         */
        std::unique_lock lock(m_lifecycleMutex);

        pid = m_childPid.load(std::memory_order_acquire);

        if (pid <= 0)
            return true;

        while (true) {
            const pid_t result = ::waitpid(pid, &status, wait ? 0 : WNOHANG);

            if (result == pid) {
                m_childPid.store(-1, std::memory_order_release);
                hasStatus = true;
                break;
            }

            if (result == 0)
                return false;

            if (result == -1 && errno == EINTR)
                continue;

            if (result == -1 && errno == ECHILD) {
                /*
                 * Somebody else already reaped this PID: a host SIGCHLD
                 * handler, or a previous reap on another thread. The child is
                 * gone, but its status is gone with it, so there is nothing
                 * honest to publish.
                 */
                m_childPid.store(-1, std::memory_order_release);
                JOB_LOG_WARN("[PtyIO] Child {} was reaped elsewhere; no exit status to publish", pid);
                break;
            }

            const int waitError = errno;

            lock.unlock();

            setError(Error::WaitError);
            JOB_LOG_WARN("[PtyIO] waitpid({}) failed: {}", pid, std::strerror(waitError));
            return false;
        }
    }

    closePidFd();

    if (!hasStatus)
        return true;

    ExitCallback callback;

    {
        std::scoped_lock lock(m_cbMutex);
        callback = m_exitCallback;
    }

    if (callback)
        callback(status);

    return true;
}

void PtyIO::setWindowSize(int rows, int cols)
{
    if (rows <= 0 || cols <= 0)
        return;

    std::scoped_lock lock(m_lifecycleMutex);

    m_winsize.ws_row = static_cast<unsigned short>(rows);
    m_winsize.ws_col = static_cast<unsigned short>(cols);

    const int masterFd = m_masterFd.load(std::memory_order_acquire);

    if (masterFd != -1 && ::ioctl(masterFd, TIOCSWINSZ, &m_winsize) == -1)
        setError(Error::IoctlError);
}

void PtyIO::setLocalecho(bool enabled)
{
    std::scoped_lock lock(m_lifecycleMutex);

    m_localecho.store(enabled, std::memory_order_release);

    if (enabled)
        m_termios.c_lflag |= ECHO;
    else
        m_termios.c_lflag &= static_cast<tcflag_t>(~ECHO);

    /*
     * The parent hands the slave to the child and closes it. Master and slave
     * share one line discipline, so the master is the surviving handle to the
     * same termios state.
     */
    int terminalFd = m_slaveFd.load(std::memory_order_acquire);

    if (terminalFd == -1)
        terminalFd = m_masterFd.load(std::memory_order_acquire);

    if (terminalFd != -1 && ::tcsetattr(terminalFd, TCSANOW, &m_termios) == -1)
        setError(Error::IoctlError);
}

void PtyIO::setNonBlocking(bool enabled)
{
    std::scoped_lock lock(m_lifecycleMutex);

    const int masterFd = m_masterFd.load(std::memory_order_acquire);

    /*
     * The master belongs to the edge-triggered async loop and must therefore
     * remain physically non-blocking for its entire lifetime. This setting
     * controls only the blocking semantics exposed by read()/write().
     */
    if (masterFd != -1) {
        const int flags = ::fcntl(masterFd, F_GETFL, 0);

        if (flags == -1 || ::fcntl(masterFd, F_SETFL, flags | O_NONBLOCK) == -1) {
            setError(Error::IoctlError);
            return;
        }
    }

    m_nonBlocking.store(enabled, std::memory_order_release);
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

std::string PtyIO::slaveName() const
{
    std::scoped_lock lock(m_lifecycleMutex);
    return m_slaveName;
}

pid_t PtyIO::childPid() const noexcept
{
    return m_childPid.load(std::memory_order_acquire);
}

PtyIO::State PtyIO::state() const noexcept
{
    return m_state.load(std::memory_order_acquire);
}

PtyIO::Error PtyIO::error() const noexcept
{
    return m_error.load(std::memory_order_acquire);
}

std::string_view PtyIO::errorString() const noexcept
{
    return errorToString(error());
}

termios PtyIO::makeDefaultTermios()
{
    termios tio{};

    tio.c_iflag = ICRNL | IXON;
    tio.c_oflag = OPOST;
    tio.c_cflag = CREAD | CS8;
    tio.c_lflag = ICANON | ECHO | ISIG;

    tio.c_cc[VINTR]    = 3;
    tio.c_cc[VQUIT]    = 28;
    tio.c_cc[VERASE]   = 127;
    tio.c_cc[VKILL]    = 21;
    tio.c_cc[VEOF]     = 4;
    tio.c_cc[VEOL]     = 0;
    tio.c_cc[VEOL2]    = 0;
    tio.c_cc[VSTART]   = 17;
    tio.c_cc[VSTOP]    = 19;
    tio.c_cc[VSUSP]    = 26;
    tio.c_cc[VREPRINT] = 18;
    tio.c_cc[VDISCARD] = 15;
    tio.c_cc[VWERASE]  = 23;
    tio.c_cc[VLNEXT]   = 22;

    return tio;
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
        return "PTY ioctl/fcntl operation failed";
    case Error::LoopError:
        return "Event loop error";
    case Error::EnvironmentError:
        return "Environment operation failed";
    case Error::SignalError:
        return "Process signal operation failed";
    case Error::PidFdError:
        return "pidfd operation failed";
    case Error::WaitError:
        return "waitpid() failed";
    }

    return "Unknown error";
}

void PtyIO::setError(Error err)
{
    m_error.store(err, std::memory_order_release);

    if (err != Error::None)
        JOB_LOG_ERROR("[PtyIO] Error set: {}", errorToString(err));
}
void PtyIO::onEvents(threads::IOEvent events)
{
    bool streamClosed = false;

    if (threads::hasEvent(events, threads::IOEvent::Read) ||
        threads::hasEvent(events, threads::IOEvent::HangUp)) {
        char buffer[4096];

        while (true) {
            int masterFd = -1;

            {
                std::scoped_lock lock(m_lifecycleMutex);
                masterFd = m_masterFd.load(std::memory_order_acquire);
            }

            if (masterFd == -1) {
                streamClosed = true;
                break;
            }

            const ssize_t bytesRead = ::read(masterFd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                ReadCallback callback;

                {
                    std::scoped_lock lock(m_cbMutex);
                    callback = m_readCallback;
                }

                if (callback)
                    callback(buffer, static_cast<std::size_t>(bytesRead));

                continue;
            }

            if (bytesRead == 0) {
                streamClosed = true;
                break;
            }

            if (errno == EINTR)
                continue;

            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;

            if (errno == EIO || errno == EBADF) {
                streamClosed = true;
                break;
            }

            setError(Error::ReadError);
            streamClosed = true;
            break;
        }
    }

    if (threads::hasEvent(events, threads::IOEvent::Error) &&
        !threads::hasEvent(events, threads::IOEvent::HangUp)) {
        setError(Error::ReadError);
        streamClosed = true;
    }

    if (threads::hasEvent(events, threads::IOEvent::HangUp))
        streamClosed = true;

    if (!streamClosed)
        return;

    std::shared_ptr<threads::JobIoAsyncThread> loop;
    int masterFd = -1;

    {
        std::scoped_lock lock(m_lifecycleMutex);

        /*
         * Transfer ownership of this close to the callback. A concurrent
         * closeDevice() gets -1 and cannot double-close it.
         */
        masterFd = m_masterFd.exchange(-1, std::memory_order_acq_rel);
        loop     = m_loop;

        m_slaveName.clear();
    }

    if (masterFd != -1) {
        if (loop && !loop->unregisterFD(masterFd))
            JOB_LOG_WARN("[PtyIO] Failed to unregister PTY master fd {}", masterFd);

        ::close(masterFd);
    }

    /*
     * PTY stream lifetime says nothing about whether the child is gone.
     * pidfd remains the process-lifetime source.
     */
    if (childPid() <= 0)
        closeDevice();
}

void PtyIO::onPidEvents(threads::IOEvent events)
{
    if (!threads::hasEvent(events, threads::IOEvent::Read) &&
        !threads::hasEvent(events, threads::IOEvent::HangUp) &&
        !threads::hasEvent(events, threads::IOEvent::Error)) {
        return;
    }

    /*
     * pidfd readability identifies the tracked child independently of PTY
     * HUP/EIO. reapChild() owns waitpid and callback publication.
     */
    if (!reapChild(true)) {
        setError(Error::WaitError);
        return;
    }

    closeDevice();
}

} // namespace job::io