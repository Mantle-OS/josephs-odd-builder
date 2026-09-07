#pragma once

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_object.h>
#include <job_signal.h>

#include "pty_io.h"

// #include "jobio_export.h"
// JOBIO_EXPORT

namespace job::io {

class JobProcess : public core::Object
{
public:
    using Ptr  = std::shared_ptr<JobProcess>;
    using WPtr = std::weak_ptr<JobProcess>;
    using UPtr = std::unique_ptr<JobProcess>;

    enum class State {
        NotRunning = 0,
        Starting,
        Running,
        Terminating,
        Finished
    };

    JobProcess();
    ~JobProcess() override;

    JobProcess(const JobProcess &) = delete;
    JobProcess &operator=(const JobProcess &) = delete;
    JobProcess(JobProcess &&) = delete;
    JobProcess &operator=(JobProcess &&) = delete;

    template <typename... Args>
        requires std::constructible_from<JobProcess, Args...>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<JobProcess>(std::forward<Args>(args)...);
    }

    template <typename... Args>
        requires std::constructible_from<JobProcess, Args...>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<JobProcess>(std::forward<Args>(args)...);
    }

    // Process ----------------------------------------------------------------

    [[nodiscard]] bool start();




    [[nodiscard]] bool terminate();
    [[nodiscard]] bool kill();

    void close();

    [[nodiscard]] bool isRunning() const noexcept;

    [[nodiscard]] const std::string &program() const noexcept;
    [[nodiscard]] const std::vector<std::string> &arguments() const noexcept;

    void setProgram(std::string program);
    void setArguments(std::vector<std::string> arguments);

    [[nodiscard]] int pid() const noexcept;
    [[nodiscard]] State state() const noexcept;

    // Process result ----------------------------------------------------------

    [[nodiscard]] bool hasExitStatus() const noexcept;

    [[nodiscard]] bool exitedNormally() const noexcept;
    [[nodiscard]] int exitCode() const noexcept;

    [[nodiscard]] bool wasSignaled() const noexcept;
    [[nodiscard]] int terminationSignal() const noexcept;

    [[nodiscard]] std::uint8_t terminationAttempts() const noexcept;
    [[nodiscard]] std::uint8_t killAttempts() const noexcept;

    // Environment -------------------------------------------------------------

    [[nodiscard]] PtyEnviron &environ() noexcept;
    [[nodiscard]] const PtyEnviron &environ() const noexcept;

    // Terminal / IO -----------------------------------------------------------

    [[nodiscard]] ssize_t read(char *buffer, std::size_t maxlen);
    [[nodiscard]] ssize_t write(const char *data, std::size_t len);

    void setWindowSize(int rows, int cols);
    void setLocalEcho(bool enabled);
    void setNonBlocking(bool enabled);

    // Signals -----------------------------------------------------------------

    core::Signal<State> stateChanged;
    core::Signal<int> started;
    core::Signal<std::string_view> readyRead;
    core::Signal<> finished;
    core::Signal<> notResponding;

private:
    void resetProcessState() noexcept;
    [[nodiscard]] bool setState(State state) noexcept;

    void onRead(const char *data, std::size_t len);
    void onExit(int status);

    std::string m_program;
    std::vector<std::string> m_arguments;

    [[=core::NoSerialize{}]]
        PtyIO::UPtr m_pty;

    [[=core::NoSerialize{}]]
        std::atomic<State> m_state{State::NotRunning};

    [[=core::NoSerialize{}]]
        std::atomic<int> m_waitStatus{0};

    [[=core::NoSerialize{}]]
        std::atomic<bool> m_hasWaitStatus{false};

    [[=core::NoSerialize{}]]
        std::atomic<bool> m_startPublished{false};

    [[=core::NoSerialize{}]]
        std::atomic<std::uint8_t> m_terminationAttempts{0};

    [[=core::NoSerialize{}]]
        std::atomic<std::uint8_t> m_killAttempts{0};
};

} // namespace job::io