#include "job_process.h"

#include <limits>
#include <string>

#include <sys/wait.h>

namespace job::io {

JobProcess::JobProcess() :
    m_pty(PtyIO::createUniq())
{
    m_pty->setReadCallback([this](const char *data, std::size_t len) {
        onRead(data, len);
    });

    m_pty->setExitCallback([this](int status) {
        onExit(status);
    });
}

JobProcess::~JobProcess()
{
    /*
     * PtyIO owns the actual child lifetime. Its destructor may therefore kill
     * and reap a still-running child. Disconnect our callbacks first so that
     * destruction of PtyIO cannot call back into a partially-destroyed
     * JobProcess.
     */
    m_pty->setReadCallback({});
    m_pty->setExitCallback({});
    m_pty.reset();
}

bool JobProcess::start()
{
    const State current = state();

    if (current == State::Starting ||
        current == State::Running ||
        current == State::Terminating) {
        lastErrorString = "Process is already running";
        return false;
    }

    if (m_program.empty()) {
        lastErrorString = "Process program is empty";
        return false;
    }

    lastErrorString.clear();
    resetProcessState();

    (void)setState(State::Starting);

    if (!m_pty->isOpen() && !m_pty->openDevice()) {
        lastErrorString = m_pty->errorString();
        (void)setState(State::NotRunning);
        return false;
    }

    if (!m_pty->startProcess(m_program, m_arguments)) {
        lastErrorString = m_pty->errorString();
        m_pty->closeDevice();
        (void)setState(State::NotRunning);
        return false;
    }

    /*
     * PtyIO is the sole owner/source of the live child PID. A very short-lived
     * process may already have been reaped by the time we get here.
     */
    const int processId = pid();

    if (processId <= 0) {
        if (hasExitStatus()) {
            m_startPublished.store(true, std::memory_order_release);

            if (setState(State::Finished))
                finished.emit();

            return true;
        }

        lastErrorString = "Process started without a valid PID or exit status";
        m_pty->closeDevice();
        (void)setState(State::NotRunning);
        return false;
    }

    (void)setState(State::Running);
    started.emit(processId);

    /*
     * onExit() deliberately defers Finished publication until the successful
     * start has been published. Store with release so an exit callback racing
     * this point can safely observe it.
     */
    m_startPublished.store(true, std::memory_order_release);

    if (hasExitStatus() && setState(State::Finished))
        finished.emit();

    return true;
}

bool JobProcess::terminate()
{
    const State previous = state();

    if (previous != State::Starting &&
        previous != State::Running &&
        previous != State::Terminating) {
        lastErrorString = "Process is not running";
        return false;
    }

    /*
     * Publish Terminating before delivering the signal. The process may exit
     * immediately and onExit() must be allowed to advance us to Finished
     * without terminate() subsequently overwriting that state.
     */
    (void)setState(State::Terminating);

    if (!m_pty->terminateProcess()) {
        lastErrorString = m_pty->errorString();

        State expected = State::Terminating;

        if (m_state.compare_exchange_strong(expected,
                                            previous,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
            stateChanged.emit(previous);
        }

        return false;
    }

    auto attempts = m_terminationAttempts.load(std::memory_order_acquire);

    while (attempts != std::numeric_limits<std::uint8_t>::max() &&
           !m_terminationAttempts.compare_exchange_weak(
               attempts,
               static_cast<std::uint8_t>(attempts + 1),
               std::memory_order_acq_rel,
               std::memory_order_acquire)) {
    }

    lastErrorString.clear();
    return true;
}

bool JobProcess::kill()
{
    const State previous = state();

    if (previous != State::Starting &&
        previous != State::Running &&
        previous != State::Terminating) {
        lastErrorString = "Process is not running";
        return false;
    }

    (void)setState(State::Terminating);

    if (!m_pty->killProcess()) {
        lastErrorString = m_pty->errorString();

        State expected = State::Terminating;

        if (m_state.compare_exchange_strong(expected,
                                            previous,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
            stateChanged.emit(previous);
        }

        return false;
    }

    auto attempts = m_killAttempts.load(std::memory_order_acquire);

    while (attempts != std::numeric_limits<std::uint8_t>::max() &&
           !m_killAttempts.compare_exchange_weak(
               attempts,
               static_cast<std::uint8_t>(attempts + 1),
               std::memory_order_acq_rel,
               std::memory_order_acquire)) {
    }

    lastErrorString.clear();
    return true;
}

void JobProcess::close()
{
    m_pty->closeDevice();

    /*
     * close() closes the terminal/IO lifetime. It does not kill an owned child;
     * PtyIO deliberately treats those as separate lifetimes.
     */
    const State current = state();

    if (current != State::Starting &&
        current != State::Running &&
        current != State::Terminating) {
        (void)setState(State::NotRunning);
    }
}

bool JobProcess::isRunning() const noexcept
{
    const State current = state();

    return current == State::Starting ||
           current == State::Running ||
           current == State::Terminating;
}

const std::string &JobProcess::program() const noexcept
{
    return m_program;
}

const std::vector<std::string> &JobProcess::arguments() const noexcept
{
    return m_arguments;
}

void JobProcess::setProgram(std::string program)
{
    m_program = std::move(program);
}

void JobProcess::setArguments(std::vector<std::string> arguments)
{
    m_arguments = std::move(arguments);
}

int JobProcess::pid() const noexcept
{
    return static_cast<int>(m_pty->childPid());
}

JobProcess::State JobProcess::state() const noexcept
{
    return m_state.load(std::memory_order_acquire);
}

bool JobProcess::hasExitStatus() const noexcept
{
    return m_hasWaitStatus.load(std::memory_order_acquire);
}

bool JobProcess::exitedNormally() const noexcept
{
    if (!hasExitStatus())
        return false;

    return WIFEXITED(m_waitStatus.load(std::memory_order_acquire));
}

int JobProcess::exitCode() const noexcept
{
    if (!hasExitStatus())
        return -1;

    const int status = m_waitStatus.load(std::memory_order_acquire);

    if (!WIFEXITED(status))
        return -1;

    return WEXITSTATUS(status);
}

bool JobProcess::wasSignaled() const noexcept
{
    if (!hasExitStatus())
        return false;

    return WIFSIGNALED(m_waitStatus.load(std::memory_order_acquire));
}

int JobProcess::terminationSignal() const noexcept
{
    if (!hasExitStatus())
        return -1;

    const int status = m_waitStatus.load(std::memory_order_acquire);

    if (!WIFSIGNALED(status))
        return -1;

    return WTERMSIG(status);
}

std::uint8_t JobProcess::terminationAttempts() const noexcept
{
    return m_terminationAttempts.load(std::memory_order_acquire);
}

std::uint8_t JobProcess::killAttempts() const noexcept
{
    return m_killAttempts.load(std::memory_order_acquire);
}

PtyEnviron &JobProcess::environ() noexcept
{
    return m_pty->environ();
}

const PtyEnviron &JobProcess::environ() const noexcept
{
    return m_pty->environ();
}

ssize_t JobProcess::read(char *buffer, std::size_t maxlen)
{
    return m_pty->read(buffer, maxlen);
}

ssize_t JobProcess::write(const char *data, std::size_t len)
{
    return m_pty->write(data, len);
}

void JobProcess::setWindowSize(int rows, int cols)
{
    m_pty->setWindowSize(rows, cols);
}

void JobProcess::setLocalEcho(bool enabled)
{
    m_pty->setLocalecho(enabled);
}

void JobProcess::setNonBlocking(bool enabled)
{
    m_pty->setNonBlocking(enabled);
}

void JobProcess::resetProcessState() noexcept
{
    m_waitStatus.store(0, std::memory_order_release);
    m_hasWaitStatus.store(false, std::memory_order_release);
    m_startPublished.store(false, std::memory_order_release);
    m_terminationAttempts.store(0, std::memory_order_release);
    m_killAttempts.store(0, std::memory_order_release);
}

bool JobProcess::setState(State state) noexcept
{
    const State previous = m_state.exchange(state, std::memory_order_acq_rel);

    if (previous == state)
        return false;

    stateChanged.emit(state);
    return true;
}

void JobProcess::onRead(const char *data, std::size_t len)
{
    if (!data || len == 0)
        return;

    readyRead.emit(std::string_view(data, len));
}

void JobProcess::onExit(int status)
{
    m_waitStatus.store(status, std::memory_order_release);
    m_hasWaitStatus.store(true, std::memory_order_release);

    /*
     * A child may exit while start() is still publishing Running and started.
     * In that case start() owns the eventual Finished transition.
     */
    if (!m_startPublished.load(std::memory_order_acquire))
        return;

    if (setState(State::Finished))
        finished.emit();
}

} // namespace job::io