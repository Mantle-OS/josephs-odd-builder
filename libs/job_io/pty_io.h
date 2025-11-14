#pragma once

#include <string>
#include <string_view>
#include <atomic>
#include <functional>

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <io_base.h>

#include <job_io_async_thread.h>

namespace job::io {

class PtyIO : public core::IODevice {
public:
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
        LoopError
    };

    using ReadCallback = IODevice::ReadCallback;
    using ExitCallback = std::function<void(int status)>;

    PtyIO();
    ~PtyIO() override;

    PtyIO(const PtyIO &) = delete;
    PtyIO &operator=(const PtyIO &) = delete;

    [[nodiscard]] bool openDevice() override;
    void closeDevice() override;

    [[nodiscard]] ssize_t read(char *buffer, size_t maxlen) override;
    [[nodiscard]] ssize_t write(const char *data, size_t len) override;

    [[nodiscard]] int fd() const override;
    [[nodiscard]] bool isOpen() const override;

    void setReadCallback(ReadCallback cb) override;
    void setNonBlocking(bool enabled) override;
    void setExitCallback(ExitCallback cb);

    [[nodiscard]] bool startShell(const std::string &shellPath);
    void setWindowSize(int rows, int cols);
    void setLocalecho(bool enabled);

    [[nodiscard]] const std::string &slaveName() const;
    [[nodiscard]] pid_t childPid() const;
    [[nodiscard]] State state() const;
    [[nodiscard]] Error error() const;
    [[nodiscard]] const std::string &errorString() const;

private:
    int m_masterFd = -1;
    int m_slaveFd = -1;
    std::string m_slaveName = {};
    pid_t m_childPid = -1;

    std::shared_ptr<threads::JobIoAsyncThread> m_loop;
    void onEvents(uint32_t events);

    std::atomic<bool> m_localecho = false;
    std::atomic<bool> m_nonBlocking = false;

    ReadCallback m_readCallback = nullptr;
    ExitCallback m_exitCallback = nullptr;

    std::atomic<State> m_state{State::Closed};
    std::atomic<Error> m_error{Error::None};

    std::string m_errorString = "No error";

    mutable std::mutex m_cbMutex;

    struct termios m_termios;
    inline struct termios makeDefaultTermios()
    {
        struct termios tio{};
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

    struct winsize m_winsize = {
        .ws_row = 24,
        .ws_col = 80,
        .ws_xpixel = 0,
        .ws_ypixel = 0,
    };

    void setError(Error err);
    static std::string_view errorToString(Error err);
};

} // namespace job::io
// CHECKPOINT: v1
