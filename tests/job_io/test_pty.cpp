#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <csignal>
#include <cerrno>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <pty_io.h>

namespace {
[[nodiscard]] pid_t forkSleepingChild()
{
    int execPipe[2]{-1, -1};

    if (::pipe2(execPipe, O_CLOEXEC) == -1)
        return -1;

    const pid_t pid = ::fork();

    if (pid == -1) {
        ::close(execPipe[0]);
        ::close(execPipe[1]);
        return -1;
    }

    if (pid == 0) {
        ::close(execPipe[0]);

        ::execl("/bin/sleep", "sleep", "600", static_cast<char *>(nullptr));

        const int execError = errno;
        static_cast<void>(::write(execPipe[1], &execError, sizeof(execError)));
        ::_exit(127);
    }

    ::close(execPipe[1]);

    int execError = 0;
    ssize_t bytesRead = -1;

    while (true) {
        bytesRead = ::read(execPipe[0], &execError, sizeof(execError));

        if (bytesRead == -1 && errno == EINTR)
            continue;

        break;
    }

    ::close(execPipe[0]);

    /*
     * EOF means exec succeeded because O_CLOEXEC closed the child's write end.
     * Data means execl failed and the child reported errno.
     */
    if (bytesRead == 0)
        return pid;

    int status = 0;

    while (::waitpid(pid, &status, 0) == -1 && errno == EINTR) {
    }

    return -1;
}
} // namespace

namespace job::io::test {

using namespace std::chrono_literals;


template <typename Predicate>
bool waitUntil(Predicate &&predicate, std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate())
            return true;

        std::this_thread::sleep_for(10ms);
    }

    return predicate();
}

class TestEnvironmentGuard
{
public:
    explicit TestEnvironmentGuard(std::string name) :
        m_name(std::move(name))
    {
        if (const char *value = ::getenv(m_name.c_str()))
            m_original = value;
    }

    ~TestEnvironmentGuard()
    {
        if (m_original)
            ::setenv(m_name.c_str(), m_original->c_str(), 1);
        else
            ::unsetenv(m_name.c_str());
    }

    TestEnvironmentGuard(const TestEnvironmentGuard &) = delete;
    TestEnvironmentGuard &operator=(const TestEnvironmentGuard &) = delete;
    TestEnvironmentGuard(TestEnvironmentGuard &&) = delete;
    TestEnvironmentGuard &operator=(TestEnvironmentGuard &&) = delete;

private:
    std::string m_name;
    std::optional<std::string> m_original;
};

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("PtyIO owns an initial process environment snapshot", "[job_io][pty][usage][environment]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.environ().initial().empty());
    REQUIRE_FALSE(pty.environ().resolved().empty());
    REQUIRE(pty.environ().initial().size() == pty.environ().resolved().size());
}

TEST_CASE("PtyIO can rebuild its process environment snapshot", "[job_io][pty][usage][environment]")
{
    constexpr const char *name = "JOB_PTY_IO_ENVIRONMENT_TEST";

    TestEnvironmentGuard guard(name);

    REQUIRE(::setenv(name, "before", 1) == 0);

    PtyIO pty;

    REQUIRE(pty.environ().containsInitial(name));
    REQUIRE(pty.environ().value(name) == "before");

    REQUIRE(::setenv(name, "after", 1) == 0);

    REQUIRE(pty.buildEnviron());
    REQUIRE(pty.environ().containsInitial(name));
    REQUIRE(pty.environ().value(name) == "after");
}

TEST_CASE("PtyIO environment model mutation does not mutate process environ",
          "[job_io][pty][usage][environment]")
{
    constexpr const char *name = "JOB_PTY_IO_MODEL_ONLY_TEST";

    TestEnvironmentGuard guard(name);

    REQUIRE(::setenv(name, "process-value", 1) == 0);

    PtyIO pty;

    REQUIRE(pty.environ().value(name) == "process-value");
    REQUIRE(pty.environ().set(name, "model-value"));

    REQUIRE(pty.environ().value(name) == "model-value");

    const char *processValue = ::getenv(name);

    REQUIRE(processValue != nullptr);
    REQUIRE(std::string_view(processValue) == "process-value");
}

#ifdef JOB_TEST_IO_PTY_DEBUG_SER

TEST_CASE("PtyIO environment serializes to JSON", "[job_io][pty][usage][environment][json]")
{
    PtyIO pty;

    const auto json = pty.environ().toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    const std::string serialized = json.dump(4);

    REQUIRE_FALSE(serialized.empty());
    REQUIRE(serialized.find("\"m_values\"") == std::string::npos);

    WARN("[PTY ENV JSON]\n" << serialized);
}

TEST_CASE("PtyIO environment serializes to YAML", "[job_io][pty][usage][environment][yaml]")
{
    PtyIO pty;

    const YAML::Node yaml = pty.environ().toYaml();

    REQUIRE(yaml.IsMap());
    REQUIRE(yaml.size() > 0);

    YAML::Emitter emitter;
    emitter << yaml;

    REQUIRE(emitter.good());

    const std::string serialized = emitter.c_str();

    REQUIRE_FALSE(serialized.empty());
    REQUIRE(serialized.find("m_values") == std::string::npos);

    WARN("[PTY ENV YAML]\n" << serialized);
}

TEST_CASE("PtyIO environment serializes to binary", "[job_io][pty][usage][environment][binary]")
{
    PtyIO pty;

    std::vector<uint8_t> binary;
    pty.environ().toBinary(binary);

    REQUIRE_FALSE(binary.empty());

    WARN("[PTY ENV BINARY] " << binary.size() << " bytes");
}

#endif

TEST_CASE("PtyIO opens and closes a pseudo terminal", "[job_io][pty][usage][lifecycle]")
{
    PtyIO pty;

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.fd() == -1);

    REQUIRE(pty.openDevice());

    REQUIRE(pty.state() == PtyIO::State::Open);
    REQUIRE(pty.isOpen());
    REQUIRE(pty.fd() >= 0);
    REQUIRE_FALSE(pty.slaveName().empty());
    REQUIRE(pty.error() == PtyIO::Error::None);

    pty.closeDevice();

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.fd() == -1);
    REQUIRE(pty.slaveName().empty());
}

TEST_CASE("PtyIO can be reincarnated after an ordinary close", "[job_io][pty][usage][lifecycle][reopen]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    const int firstFd = pty.fd();

    REQUIRE(firstFd >= 0);

    pty.closeDevice();

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.fd() == -1);

    REQUIRE(pty.openDevice());

    REQUIRE(pty.state() == PtyIO::State::Open);
    REQUIRE(pty.isOpen());
    REQUIRE(pty.fd() >= 0);
    REQUIRE_FALSE(pty.slaveName().empty());

    pty.closeDevice();

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE_FALSE(pty.isOpen());
}

TEST_CASE("PtyIO keeps its master descriptor physically non-blocking",
          "[job_io][pty][usage][nonblocking]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    const int initialFlags = ::fcntl(pty.fd(), F_GETFL, 0);

    REQUIRE(initialFlags >= 0);
    REQUIRE((initialFlags & O_NONBLOCK) != 0);

    /*
     * setNonBlocking() now controls the public read()/write() semantics only.
     * The master itself remains O_NONBLOCK because it is owned by the
     * edge-triggered async loop.
     */
    pty.setNonBlocking(false);

    const int logicalBlockingFlags = ::fcntl(pty.fd(), F_GETFL, 0);

    REQUIRE(logicalBlockingFlags >= 0);
    REQUIRE((logicalBlockingFlags & O_NONBLOCK) != 0);

    pty.setNonBlocking(true);

    const int logicalNonBlockingFlags = ::fcntl(pty.fd(), F_GETFL, 0);

    REQUIRE(logicalNonBlockingFlags >= 0);
    REQUIRE((logicalNonBlockingFlags & O_NONBLOCK) != 0);
}

TEST_CASE("PtyIO blocking read does not hold the lifecycle mutex",
          "[job_io][pty][usage][blocking][read][lifecycle]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    const std::string slaveName = pty.slaveName();
    const int slaveFd = ::open(slaveName.c_str(), O_RDWR | O_NOCTTY);

    REQUIRE(slaveFd >= 0);

    pty.setNonBlocking(false);

    std::atomic<bool> readerStarted{false};
    std::atomic<bool> readerDone{false};
    std::atomic<bool> closeDone{false};

    char buffer[16]{};
    ssize_t readResult = 0;

    std::thread reader([&]() {
        readerStarted.store(true, std::memory_order_release);
        readResult = pty.read(buffer, sizeof(buffer));
        readerDone.store(true, std::memory_order_release);
    });

    REQUIRE(waitUntil([&]() {
        return readerStarted.load(std::memory_order_acquire);
    }));

    std::this_thread::sleep_for(50ms);

    REQUIRE_FALSE(readerDone.load(std::memory_order_acquire));

    std::thread closer([&]() {
        pty.closeDevice();
        closeDone.store(true, std::memory_order_release);
    });

    const bool closeReturnedWhileReadBlocked = waitUntil([&]() {
        return closeDone.load(std::memory_order_acquire);
    }, 250ms);

    reader.join();
    closer.join();
    ::close(slaveFd);

    REQUIRE(closeReturnedWhileReadBlocked);
    REQUIRE(readerDone.load(std::memory_order_acquire));
    REQUIRE(readResult == -1);
    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO blocking write does not hold the lifecycle mutex",
          "[job_io][pty][usage][blocking][write][lifecycle]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    const std::string slaveName = pty.slaveName();
    const int slaveFd = ::open(slaveName.c_str(), O_RDWR | O_NOCTTY);

    REQUIRE(slaveFd >= 0);

    termios attributes{};

    REQUIRE(::tcgetattr(slaveFd, &attributes) == 0);

    ::cfmakeraw(&attributes);

    REQUIRE(::tcsetattr(slaveFd, TCSANOW, &attributes) == 0);

    /*
     * Fill the slave input queue while the master is still non-blocking.
     */
    std::vector<char> fill(4096, 'x');
    bool queueFull = false;

    for (std::size_t attempt = 0; attempt < 1024; ++attempt) {
        const ssize_t written = ::write(pty.fd(), fill.data(), fill.size());

        if (written >= 0)
            continue;

        if (errno == EINTR) {
            --attempt;
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            queueFull = true;
            break;
        }

        FAIL("Unexpected PTY fill write failure: " << std::strerror(errno));
    }

    if (!queueFull) {
        ::close(slaveFd);
        SKIP("Could not saturate the PTY input queue on this kernel");
    }

    /*
     * Confirm the queue is still refusing bytes immediately before switching
     * modes. This narrows, but cannot eliminate, the window in which the
     * kernel might make room before the writer thread reaches its syscall.
     */
    const ssize_t probe = ::write(pty.fd(), fill.data(), 1);
    const int probeError = errno;

    if (probe != -1 || (probeError != EAGAIN && probeError != EWOULDBLOCK)) {
        ::close(slaveFd);
        SKIP("PTY input queue drained before the blocking write could be staged");
    }

    pty.setNonBlocking(false);

    std::atomic<bool> writerStarted{false};
    std::atomic<bool> writerDone{false};
    std::atomic<bool> resizeDone{false};

    const char byte = 'y';
    ssize_t writeResult = -1;

    std::thread writer([&]() {
        writerStarted.store(true, std::memory_order_release);
        writeResult = pty.write(&byte, 1);
        writerDone.store(true, std::memory_order_release);
    });

    REQUIRE(waitUntil([&]() {
        return writerStarted.load(std::memory_order_acquire);
    }));

    std::this_thread::sleep_for(50ms);

    const bool writeWasBlocked = !writerDone.load(std::memory_order_acquire);

    bool resizeReturnedWhileWriteBlocked = false;

    if (writeWasBlocked) {
        /*
         * setWindowSize() takes m_lifecycleMutex. If write() were still
         * holding it inside the parked syscall, this thread could never
         * finish.
         */
        std::thread resizer([&]() {
            pty.setWindowSize(49, 133);
            resizeDone.store(true, std::memory_order_release);
        });

        resizeReturnedWhileWriteBlocked = waitUntil([&]() {
            return resizeDone.load(std::memory_order_acquire);
        }, 250ms);

        resizer.join();
    }

    /*
     * Release the writer before joining, whatever the outcome above.
     */
    std::vector<char> drain(8192);

    while (!writerDone.load(std::memory_order_acquire)) {
        const ssize_t drained = ::read(slaveFd, drain.data(), drain.size());

        if (drained > 0)
            continue;

        if (drained == -1 && errno == EINTR)
            continue;

        break;
    }

    writer.join();
    ::close(slaveFd);

    if (!writeWasBlocked)
        SKIP("Write completed immediately; the blocking path was not exercised");

    REQUIRE(resizeReturnedWhileWriteBlocked);
    REQUIRE(writeResult == 1);

    winsize size{};

    REQUIRE(::ioctl(pty.fd(), TIOCGWINSZ, &size) == 0);
    REQUIRE(size.ws_row == 49);
    REQUIRE(size.ws_col == 133);
}


TEST_CASE("PtyIO applies terminal window size", "[job_io][pty][usage][window]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    pty.setWindowSize(48, 132);

    winsize size{};

    REQUIRE(::ioctl(pty.fd(), TIOCGWINSZ, &size) == 0);
    REQUIRE(size.ws_row == 48);
    REQUIRE(size.ws_col == 132);
}

TEST_CASE("PtyIO ignores invalid terminal window sizes", "[job_io][pty][usage][window][edge]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    pty.setWindowSize(48, 132);

    winsize before{};

    REQUIRE(::ioctl(pty.fd(), TIOCGWINSZ, &before) == 0);

    pty.setWindowSize(0, 132);
    pty.setWindowSize(48, 0);
    pty.setWindowSize(-1, 132);
    pty.setWindowSize(48, -1);

    winsize after{};

    REQUIRE(::ioctl(pty.fd(), TIOCGWINSZ, &after) == 0);
    REQUIRE(after.ws_row == before.ws_row);
    REQUIRE(after.ws_col == before.ws_col);
}

TEST_CASE("PtyIO local echo returns terminal input", "[job_io][pty][usage][echo]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<std::size_t> received{0};
    std::mutex outputMutex;
    std::string output;

    pty.setLocalecho(true);

    pty.setReadCallback([&](const char *data, std::size_t len) {
        {
            std::scoped_lock lock(outputMutex);
            output.append(data, len);
        }

        received.fetch_add(len, std::memory_order_release);
    });

    const std::string input = "ping\n";

    REQUIRE(pty.write(input.data(), input.size()) == static_cast<ssize_t>(input.size()));

    REQUIRE(waitUntil([&]() {
        return received.load(std::memory_order_acquire) > 0;
    }));

    {
        std::scoped_lock lock(outputMutex);
        REQUIRE(output.find("ping") != std::string::npos);
    }
}

TEST_CASE("PtyIO local echo can be changed after a child starts",
          "[job_io][pty][usage][echo][process]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

    pty.setLocalecho(false);

    termios attributes{};

    REQUIRE(::tcgetattr(pty.fd(), &attributes) == 0);
    REQUIRE((attributes.c_lflag & ECHO) == 0);

    pty.setLocalecho(true);

    REQUIRE(::tcgetattr(pty.fd(), &attributes) == 0);
    REQUIRE((attributes.c_lflag & ECHO) != 0);

    REQUIRE(pty.killProcess());

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));
}

TEST_CASE("PtyIO starts an arbitrary process on the pseudo terminal", "[job_io][pty][usage][process]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> sawOutput{false};
    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    std::mutex outputMutex;
    std::string output;

    pty.setReadCallback([&](const char *data, std::size_t len) {
        std::scoped_lock lock(outputMutex);
        output.append(data, len);

        if (output.find("JOB-PTY-PROCESS") != std::string::npos)
            sawOutput.store(true, std::memory_order_release);
    });

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/echo", {"JOB-PTY-PROCESS"}));
    REQUIRE(pty.state() == PtyIO::State::Running);
    REQUIRE(pty.childPid() > 0);

    REQUIRE(waitUntil([&]() {
        return sawOutput.load(std::memory_order_acquire);
    }));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));

    {
        std::scoped_lock lock(outputMutex);
        REQUIRE(output.find("JOB-PTY-PROCESS") != std::string::npos);
    }
}

TEST_CASE("PtyIO observes child exit independently of PTY lifetime",
          "[job_io][pty][usage][process][pidfd]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    /*
     * The shell exits immediately while a descendant briefly remains alive
     * with the inherited PTY descriptors. Child exit notification must come
     * from the process lifetime source rather than waiting for PTY HUP.
     */
    REQUIRE(pty.startProcess("/bin/sh", {"-c", "sleep 1 & exit 23"}));

    REQUIRE(pty.childPid() > 0);

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 23);
    REQUIRE(pty.childPid() == -1);

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));
}

TEST_CASE("PtyIO terminates its current process group with SIGTERM",
          "[job_io][pty][usage][process][terminate]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

    const pid_t childPid = pty.childPid();

    REQUIRE(childPid > 0);
    REQUIRE(pty.state() == PtyIO::State::Running);

    REQUIRE(pty.terminateProcess());

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGTERM);

    REQUIRE(pty.childPid() == -1);

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));
}

TEST_CASE("PtyIO kills its current process group with SIGKILL",
          "[job_io][pty][usage][process][kill]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

    const pid_t childPid = pty.childPid();

    REQUIRE(childPid > 0);
    REQUIRE(pty.state() == PtyIO::State::Running);

    REQUIRE(pty.killProcess());

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGKILL);

    REQUIRE(pty.childPid() == -1);

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));
}

TEST_CASE("PtyIO can explicitly terminate a numeric process ID",
          "[job_io][pty][usage][process][terminate][pid]")
{
    PtyIO pty;

    const pid_t childPid = forkSleepingChild();

    REQUIRE(childPid > 0);

    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.terminateProcess(static_cast<int>(childPid)));

    int status = 0;

    REQUIRE(::waitpid(childPid, &status, 0) == childPid);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGTERM);

    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO can explicitly kill a numeric process ID",
          "[job_io][pty][usage][process][kill][pid]")
{
    PtyIO pty;

    const pid_t childPid = forkSleepingChild();

    REQUIRE(childPid > 0);

    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.killProcess(static_cast<int>(childPid)));

    int status = 0;

    REQUIRE(::waitpid(childPid, &status, 0) == childPid);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGKILL);

    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO starts an interactive shell by path", "[job_io][pty][usage][shell]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> sawOutput{false};
    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    std::mutex outputMutex;
    std::string output;

    pty.setReadCallback([&](const char *data, std::size_t len) {
        std::scoped_lock lock(outputMutex);
        output.append(data, len);

        if (output.find("JOB-PTY-SHELL") != std::string::npos)
            sawOutput.store(true, std::memory_order_release);
    });

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startShell("/bin/sh"));

    const std::string command = "printf 'JOB-PTY-SHELL\\n'\nexit\n";

    REQUIRE(pty.write(command.data(), command.size()) == static_cast<ssize_t>(command.size()));

    REQUIRE(waitUntil([&]() {
        return sawOutput.load(std::memory_order_acquire);
    }));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
}

TEST_CASE("PtyIO starts POSIX sh using ShellType", "[job_io][pty][usage][shell][type]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> sawOutput{false};
    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    std::mutex outputMutex;
    std::string output;

    pty.setReadCallback([&](const char *data, std::size_t len) {
        std::scoped_lock lock(outputMutex);
        output.append(data, len);

        if (output.find("JOB-SHELL-TYPE-SH") != std::string::npos)
            sawOutput.store(true, std::memory_order_release);
    });

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startShell(PtyIO::ShellType::Sh));

    const std::string command = "printf 'JOB-SHELL-TYPE-SH\\n'\nexit\n";

    REQUIRE(pty.write(command.data(), command.size()) == static_cast<ssize_t>(command.size()));

    REQUIRE(waitUntil([&]() {
        return sawOutput.load(std::memory_order_acquire);
    }));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
}

TEST_CASE("PtyIO starts dash using ShellType", "[job_io][pty][usage][shell][type]")
{
    if (!std::filesystem::exists("/bin/dash"))
        SKIP("/bin/dash is not installed");

    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startShell(PtyIO::ShellType::Dash));

    const std::string command = "exit\n";

    REQUIRE(pty.write(command.data(), command.size()) == static_cast<ssize_t>(command.size()));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
}

TEST_CASE("PtyIO starts bash using ShellType", "[job_io][pty][usage][shell][type]")
{
    if (!std::filesystem::exists("/bin/bash"))
        SKIP("/bin/bash is not installed");

    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startShell(PtyIO::ShellType::Bash));

    const std::string command = "exit\n";

    REQUIRE(pty.write(command.data(), command.size()) == static_cast<ssize_t>(command.size()));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
}

TEST_CASE("PtyIO starts zsh using ShellType", "[job_io][pty][usage][shell][type]")
{
    if (!std::filesystem::exists("/bin/zsh"))
        SKIP("/bin/zsh is not installed");

    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startShell(PtyIO::ShellType::Zsh));

    const std::string command = "exit\n";

    REQUIRE(pty.write(command.data(), command.size()) == static_cast<ssize_t>(command.size()));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);
}

// =============================================================================
// Block 2: Edge cases / failure behavior
// =============================================================================

TEST_CASE("PtyIO rejects a second open", "[job_io][pty][edge][lifecycle]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE_FALSE(pty.openDevice());

    REQUIRE(pty.isOpen());
    REQUIRE(pty.fd() >= 0);
}

TEST_CASE("PtyIO close is idempotent", "[job_io][pty][edge][lifecycle]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    pty.closeDevice();

    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.state() == PtyIO::State::Closed);

    REQUIRE_NOTHROW(pty.closeDevice());

    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO callback driven close does not self join", "[job_io][pty][edge][lifecycle][callback]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/true"));

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));

    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.fd() == -1);
    REQUIRE(pty.childPid() == -1);

    REQUIRE_NOTHROW(pty.closeDevice());

    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO can reincarnate after callback driven close",
          "[job_io][pty][edge][lifecycle][callback][reopen]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/true"));

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));

    REQUIRE_FALSE(pty.isOpen());
    REQUIRE(pty.fd() == -1);
    REQUIRE(pty.childPid() == -1);

    REQUIRE(pty.openDevice());

    REQUIRE(pty.state() == PtyIO::State::Open);
    REQUIRE(pty.isOpen());
    REQUIRE(pty.fd() >= 0);
    REQUIRE_FALSE(pty.slaveName().empty());

    pty.closeDevice();

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE_FALSE(pty.isOpen());
}

TEST_CASE("PtyIO rejects process launch before opening", "[job_io][pty][edge][process]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.startProcess("/bin/echo", {"hello"}));
    REQUIRE_FALSE(pty.startShell("/bin/sh"));

    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Closed);
}

TEST_CASE("PtyIO rejects an empty process path", "[job_io][pty][edge][process]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    REQUIRE_FALSE(pty.startProcess(""));
    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Open);
}

TEST_CASE("PtyIO rejects an empty shell path", "[job_io][pty][edge][shell]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    REQUIRE_FALSE(pty.startShell(""));
    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Open);
}

TEST_CASE("PtyIO rejects an unknown shell type", "[job_io][pty][edge][shell][type]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    const auto invalid = static_cast<PtyIO::ShellType>(255);

    REQUIRE_FALSE(pty.startShell(invalid));
    REQUIRE(pty.childPid() == -1);
    REQUIRE(pty.state() == PtyIO::State::Open);
}

TEST_CASE("PtyIO rejects a second child while one is running", "[job_io][pty][edge][process]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/sh", {"-c", "sleep 10"}));

    REQUIRE(pty.state() == PtyIO::State::Running);
    REQUIRE(pty.childPid() > 0);

    REQUIRE_FALSE(pty.startProcess("/bin/echo", {"second"}));

    REQUIRE(pty.killProcess());

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));
}

TEST_CASE("PtyIO terminate rejects missing current child", "[job_io][pty][edge][process][terminate]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.terminateProcess());
    REQUIRE(pty.error() == PtyIO::Error::SignalError);
}

TEST_CASE("PtyIO kill rejects missing current child", "[job_io][pty][edge][process][kill]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.killProcess());
    REQUIRE(pty.error() == PtyIO::Error::SignalError);
}

TEST_CASE("PtyIO explicit terminate rejects invalid numeric process IDs",
          "[job_io][pty][edge][process][terminate][pid]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.terminateProcess(-1));
    REQUIRE(pty.error() == PtyIO::Error::SignalError);

    REQUIRE_FALSE(pty.terminateProcess(0));
    REQUIRE(pty.error() == PtyIO::Error::SignalError);
}

TEST_CASE("PtyIO explicit kill rejects invalid numeric process IDs",
          "[job_io][pty][edge][process][kill][pid]")
{
    PtyIO pty;

    REQUIRE_FALSE(pty.killProcess(-1));
    REQUIRE(pty.error() == PtyIO::Error::SignalError);

    REQUIRE_FALSE(pty.killProcess(0));
    REQUIRE(pty.error() == PtyIO::Error::SignalError);
}

TEST_CASE("PtyIO read and write reject closed devices", "[job_io][pty][edge][io]")
{
    PtyIO pty;

    char buffer[16]{};
    const std::string data = "hello";

    REQUIRE(pty.read(buffer, sizeof(buffer)) == -1);
    REQUIRE(pty.write(data.data(), data.size()) == -1);
}

TEST_CASE("PtyIO rejects null buffers", "[job_io][pty][edge][io]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    REQUIRE(pty.read(nullptr, 16) == -1);
    REQUIRE(pty.write(nullptr, 16) == -1);
}

TEST_CASE("PtyIO accepts zero length IO on an open device", "[job_io][pty][edge][io]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    char buffer{};

    REQUIRE(pty.read(&buffer, 0) == 0);
    REQUIRE(pty.write(&buffer, 0) == 0);
}

TEST_CASE("PtyIO does not publish a fabricated status when its child was reaped elsewhere",
          "[job_io][pty][edge][process][wait][echild]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<std::size_t> exitCount{0};
    std::atomic<bool> childReady{false};

    std::mutex outputMutex;
    std::string output;

    pty.setReadCallback([&](const char *data, std::size_t len) {
        std::scoped_lock lock(outputMutex);
        output.append(data, len);

        if (output.find("JOB-ECHILD-READY") != std::string::npos)
            childReady.store(true, std::memory_order_release);
    });

    pty.setExitCallback([&](int) {
        exitCount.fetch_add(1, std::memory_order_release);
    });

    /*
     * Ignore SIGHUP so closing the PTY does not decide the child lifetime for
     * us. closeDevice() deliberately preserves m_childPid while dropping pidfd
     * observation, which lets this test simulate a host application reaping
     * the child itself.
     */
    REQUIRE(pty.startProcess(
        "/bin/sh",
        {"-c", "trap '' HUP; printf 'JOB-ECHILD-READY\\n'; while true; do sleep 1; done"}));

    REQUIRE(waitUntil([&]() {
        return childReady.load(std::memory_order_acquire);
    }));

    const pid_t pid = pty.childPid();

    REQUIRE(pid > 0);

    pty.closeDevice();

    REQUIRE(pty.state() == PtyIO::State::Closed);
    REQUIRE(pty.childPid() == pid);
    REQUIRE(::kill(pid, SIGKILL) == 0);

    int externalStatus = 0;

    REQUIRE(::waitpid(pid, &externalStatus, 0) == pid);
    REQUIRE(WIFSIGNALED(externalStatus));
    REQUIRE(WTERMSIG(externalStatus) == SIGKILL);
    REQUIRE(exitCount.load(std::memory_order_acquire) == 0);

    /*
     * openDevice() attempts reapChild(false) before creating a new PTY. Since
     * the host already consumed the wait status, waitpid() must report ECHILD.
     * That clears ownership without inventing a status or invoking the callback.
     */
    REQUIRE(pty.openDevice());

    REQUIRE(pty.childPid() == -1);
    REQUIRE(exitCount.load(std::memory_order_acquire) == 0);
    REQUIRE(pty.state() == PtyIO::State::Open);

    pty.closeDevice();
}

TEST_CASE("PtyIO child does not inherit async loop epoll or eventfd descriptors",
          "[job_io][pty][edge][process][exec][cloexec]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> scanDone{false};
    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    std::mutex outputMutex;
    std::string output;

    pty.setReadCallback([&](const char *data, std::size_t len) {
        std::scoped_lock lock(outputMutex);
        output.append(data, len);

        if (output.find("JOB-CLOEXEC-SCAN-DONE") != std::string::npos)
            scanDone.store(true, std::memory_order_release);
    });

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    /*
     * The JobIoAsyncThread exists before fork(). Without EPOLL_CLOEXEC and
     * EFD_CLOEXEC these two anon_inode descriptors survive execve() and are
     * visible in the launched process.
     */
    const std::string command =
        "for fd in /proc/self/fd/[0-9]*; do "
        "target=$(readlink \"$fd\" 2>/dev/null || true); "
        "case \"$target\" in "
        "'anon_inode:[eventpoll]'|'anon_inode:[eventfd]') "
        "printf '%s\\n' \"$target\";; "
        "esac; "
        "done; "
        "printf 'JOB-CLOEXEC-SCAN-DONE\\n'";

    REQUIRE(pty.startProcess("/bin/sh", {"-c", command}));

    REQUIRE(waitUntil([&]() {
        return scanDone.load(std::memory_order_acquire);
    }));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 0);

    std::scoped_lock lock(outputMutex);

    REQUIRE(output.find("anon_inode:[eventpoll]") == std::string::npos);
    REQUIRE(output.find("anon_inode:[eventfd]") == std::string::npos);
}

TEST_CASE("PtyIO reports process launch failure through child exit", "[job_io][pty][edge][exec]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<bool> exited{false};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exited.store(true, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/this/path/does/not/exist"));

    REQUIRE(waitUntil([&]() {
        return exited.load(std::memory_order_acquire);
    }));

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 127);
    REQUIRE(pty.childPid() == -1);
}

TEST_CASE("PtyIO exit callback fires exactly once", "[job_io][pty][edge][process][pidfd][callback]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<std::size_t> exitCount{0};

    pty.setExitCallback([&](int) {
        exitCount.fetch_add(1, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/true"));

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));

    std::this_thread::sleep_for(50ms);

    REQUIRE(exitCount.load(std::memory_order_acquire) == 1);
}

TEST_CASE("PtyIO terminate callback fires exactly once",
          "[job_io][pty][edge][process][terminate][callback]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<std::size_t> exitCount{0};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exitCount.fetch_add(1, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));
    REQUIRE(pty.terminateProcess());

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));

    std::this_thread::sleep_for(50ms);

    REQUIRE(exitCount.load(std::memory_order_acquire) == 1);

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGTERM);
}

TEST_CASE("PtyIO kill callback fires exactly once",
          "[job_io][pty][edge][process][kill][callback]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());

    std::atomic<std::size_t> exitCount{0};
    std::atomic<int> exitStatus{-1};

    pty.setExitCallback([&](int status) {
        exitStatus.store(status, std::memory_order_release);
        exitCount.fetch_add(1, std::memory_order_release);
    });

    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));
    REQUIRE(pty.killProcess());

    REQUIRE(waitUntil([&]() {
        return pty.childPid() == -1;
    }));

    std::this_thread::sleep_for(50ms);

    REQUIRE(exitCount.load(std::memory_order_acquire) == 1);

    const int status = exitStatus.load(std::memory_order_acquire);

    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGKILL);
}

TEST_CASE("PtyIO can start again after terminated child is reaped",
          "[job_io][pty][edge][process][terminate][reopen]")
{
    PtyIO pty;

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

    REQUIRE(pty.terminateProcess());

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));

    REQUIRE(pty.childPid() == -1);

    REQUIRE(pty.openDevice());
    REQUIRE(pty.startProcess("/bin/true"));

    REQUIRE(waitUntil([&]() {
        return pty.state() == PtyIO::State::Closed;
    }));

    REQUIRE(pty.childPid() == -1);
}

// =============================================================================
// Block 3: Benchmarks / stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("PtyIO survives repeated ordinary reincarnation", "[job_io][pty][stress][reopen]")
{
    constexpr std::size_t ITERATIONS = 50;

    PtyIO pty;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY ordinary reincarnation iteration: " << iteration);

        REQUIRE(pty.openDevice());
        REQUIRE(pty.state() == PtyIO::State::Open);
        REQUIRE(pty.isOpen());
        REQUIRE(pty.fd() >= 0);

        pty.closeDevice();

        REQUIRE(pty.state() == PtyIO::State::Closed);
        REQUIRE_FALSE(pty.isOpen());
        REQUIRE(pty.fd() == -1);
    }
}

TEST_CASE("PtyIO survives repeated callback reincarnation", "[job_io][pty][stress][callback][reopen]")
{
    constexpr std::size_t ITERATIONS = 50;

    PtyIO pty;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY callback reincarnation iteration: " << iteration);

        REQUIRE(pty.openDevice());
        REQUIRE(pty.startProcess("/bin/true"));

        REQUIRE(waitUntil([&]() {
            return pty.state() == PtyIO::State::Closed;
        }));

        REQUIRE_FALSE(pty.isOpen());
        REQUIRE(pty.fd() == -1);
        REQUIRE(pty.childPid() == -1);
    }
}

TEST_CASE("PtyIO survives repeated terminate and reincarnation",
          "[job_io][pty][stress][terminate][reopen]")
{
    constexpr std::size_t ITERATIONS = 50;

    PtyIO pty;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY terminate reincarnation iteration: " << iteration);

        REQUIRE(pty.openDevice());
        REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

        REQUIRE(pty.childPid() > 0);
        REQUIRE(pty.terminateProcess());

        REQUIRE(waitUntil([&]() {
            return pty.state() == PtyIO::State::Closed;
        }));

        REQUIRE(pty.childPid() == -1);
        REQUIRE_FALSE(pty.isOpen());
    }
}

TEST_CASE("PtyIO survives repeated kill and reincarnation",
          "[job_io][pty][stress][kill][reopen]")
{
    constexpr std::size_t ITERATIONS = 50;

    PtyIO pty;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY kill reincarnation iteration: " << iteration);

        REQUIRE(pty.openDevice());
        REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));

        REQUIRE(pty.childPid() > 0);
        REQUIRE(pty.killProcess());

        REQUIRE(waitUntil([&]() {
            return pty.state() == PtyIO::State::Closed;
        }));

        REQUIRE(pty.childPid() == -1);
        REQUIRE_FALSE(pty.isOpen());
    }
}

TEST_CASE("PtyIO pidfd exit delivery remains single shot under stress",
          "[job_io][pty][stress][pidfd][callback]")
{
    constexpr std::size_t ITERATIONS = 100;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY pidfd callback iteration: " << iteration);

        PtyIO pty;

        REQUIRE(pty.openDevice());

        std::atomic<std::size_t> exitCount{0};

        pty.setExitCallback([&](int) {
            exitCount.fetch_add(1, std::memory_order_release);
        });

        REQUIRE(pty.startProcess("/bin/true"));

        REQUIRE(waitUntil([&]() {
            return pty.state() == PtyIO::State::Closed;
        }));

        REQUIRE(pty.childPid() == -1);
        REQUIRE(exitCount.load(std::memory_order_acquire) == 1);
    }
}

TEST_CASE("PtyIO process-group termination survives descendants",
          "[job_io][pty][stress][terminate][process-group]")
{
    constexpr std::size_t ITERATIONS = 50;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("PTY process-group termination iteration: " << iteration);

        PtyIO pty;

        REQUIRE(pty.openDevice());

        std::atomic<bool> exited{false};
        std::atomic<int> exitStatus{-1};

        pty.setExitCallback([&](int status) {
            exitStatus.store(status, std::memory_order_release);
            exited.store(true, std::memory_order_release);
        });

        REQUIRE(pty.startProcess("/bin/sh", {"-c", "while true; do sleep 1; done"}));
        REQUIRE(pty.terminateProcess());

        REQUIRE(waitUntil([&]() {
            return exited.load(std::memory_order_acquire);
        }));

        const int status = exitStatus.load(std::memory_order_acquire);

        REQUIRE(WIFSIGNALED(status));
        REQUIRE(WTERMSIG(status) == SIGTERM);
        REQUIRE(pty.childPid() == -1);
    }
}

TEST_CASE("PtyIO benchmarks", "[job_io][pty][benchmark]")
{
    BENCHMARK("PtyIO open and close")
    {
        PtyIO pty;

        if (!pty.openDevice())
            return false;

        pty.closeDevice();
        return true;
    };
}

TEST_CASE("PtyIO environment binary serialization benchmarks",
          "[job_io][pty][benchmark][environment][binary]")
{
    PtyIO pty;

    std::vector<uint8_t> sizeProbe;
    pty.environ().toBinary(sizeProbe);

    REQUIRE_FALSE(sizeProbe.empty());

    WARN("[PTY ENV BINARY BENCHMARK] serialized size: " << sizeProbe.size() << " bytes");

    BENCHMARK("PtyEnviron toBinary with allocation")
    {
        std::vector<uint8_t> binary;
        pty.environ().toBinary(binary);

        return binary.size();
    };

    std::vector<uint8_t> binary;
    binary.reserve(sizeProbe.size());

    BENCHMARK("PtyEnviron toBinary reused buffer")
    {
        binary.clear();
        pty.environ().toBinary(binary);

        return binary.size();
    };
}

#endif

} // namespace job::io::test