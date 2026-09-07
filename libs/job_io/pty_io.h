#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <io_base.h>
#include <job_io_async_thread.h>

#include "pty_environ.h"

// #include "jobio_export.h"
// JOBIO_EXPORT

namespace job::io {

class PtyIO : public IODevice
{
public:
    using Ptr  = std::shared_ptr<PtyIO>;
    using WPtr = std::weak_ptr<PtyIO>;
    using UPtr = std::unique_ptr<PtyIO>;

    enum class State {
        Closed = 0,
        Opening,
        Open,
        Running,
        Closing
    };

    enum class Error {
        None = 0,
        OpenFailed,
        GrantFailed,
        UnlockFailed,
        SlaveOpenFailed,
        ForkFailed,
        ExecFailed,
        WriteError,
        ReadError,
        IoctlError,
        LoopError,
        EnvironmentError,
        SignalError,
        PidFdError,
        WaitError
    };

    enum class ShellType {
        Sh = 0,
        Dash,
        Bash,
        Zsh
    };

    using ReadCallback = IODevice::ReadCallback;
    using ExitCallback = std::function<void(int status)>;

    PtyIO();
    ~PtyIO() override;

    PtyIO(const PtyIO &) = delete;
    PtyIO &operator=(const PtyIO &) = delete;
    PtyIO(PtyIO &&) = delete;
    PtyIO &operator=(PtyIO &&) = delete;

    template <typename... Args>
        requires std::constructible_from<PtyIO, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<PtyIO>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<PtyIO, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<PtyIO>(std::forward<Args>(args)...);
    }

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, std::size_t len) override;

    [[nodiscard]] int fd() const override;
    [[nodiscard]] bool isOpen() const override;

    void setReadCallback(ReadCallback cb) override;
    void setNonBlocking(bool enabled) override;
    void setExitCallback(ExitCallback cb);

    // Environment ------------------------------------------------------------

    [[nodiscard]] PtyEnviron &environ() noexcept;
    [[nodiscard]] const PtyEnviron &environ() const noexcept;

    [[nodiscard]] bool buildEnviron();

    // Process / shell --------------------------------------------------------

    [[nodiscard]] bool startProcess(const std::string &program,
                                    const std::vector<std::string> &arguments = {});

    [[nodiscard]] bool startShell(ShellType shellType);
    [[nodiscard]] bool startShell(const std::string &shellPath);

    // Process control --------------------------------------------------------

    [[nodiscard]] bool terminateProcess();
    [[nodiscard]] bool killProcess();

    [[nodiscard]] bool terminateProcess(int pid);
    [[nodiscard]] bool killProcess(int pid);

    // Terminal ---------------------------------------------------------------

    void setWindowSize(int rows, int cols);
    void setLocalecho(bool enabled);

    [[nodiscard]] std::string slaveName() const;
    [[nodiscard]] pid_t childPid() const noexcept;
    [[nodiscard]] State state() const noexcept;
    [[nodiscard]] Error error() const noexcept;
    [[nodiscard]] std::string_view errorString() const noexcept;

private:
    static termios makeDefaultTermios();

    [[nodiscard]] static std::string_view shellPath(ShellType shellType) noexcept;

    [[nodiscard]] bool signalChild(int signal);
    [[nodiscard]] bool signalProcess(pid_t pid, int signal);

    [[nodiscard]] bool reapChild(bool wait);

    [[nodiscard]] bool openPidFd();
    void closePidFd() noexcept;

    void onEvents(threads::IOEvent events);
    void onPidEvents(threads::IOEvent events);

    void setError(Error err);
    static std::string_view errorToString(Error err);

    std::atomic<int> m_masterFd{-1};
    std::atomic<int> m_slaveFd{-1};
    std::atomic<int> m_pidFd{-1};

    std::string m_slaveName;

    std::atomic<pid_t> m_childPid{-1};

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;

    PtyEnviron m_environ;

    std::atomic<bool> m_localecho{false};
    std::atomic<bool> m_nonBlocking{false};

    ReadCallback m_readCallback;
    ExitCallback m_exitCallback;

    std::atomic<State> m_state{State::Closed};
    std::atomic<Error> m_error{Error::None};

    mutable std::mutex m_lifecycleMutex;
    mutable std::mutex m_cbMutex;

    termios m_termios{};

    winsize m_winsize{
        .ws_row = 24,
        .ws_col = 80,
        .ws_xpixel = 0,
        .ws_ypixel = 0,
    };
};

} // namespace job::io