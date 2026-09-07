#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <csignal>
#include <sys/wait.h>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_process.h>

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

// =============================================================================
// Block 1: Usage / examples
// =============================================================================

TEST_CASE("JobProcess starts with no configured program", "[job_io][process][usage][state]")
{
    JobProcess process;

    REQUIRE(process.program().empty());
    REQUIRE(process.arguments().empty());

    REQUIRE(process.state() == JobProcess::State::NotRunning);
    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);

    REQUIRE_FALSE(process.hasExitStatus());
    REQUIRE_FALSE(process.exitedNormally());
    REQUIRE(process.exitCode() == -1);
    REQUIRE_FALSE(process.wasSignaled());
    REQUIRE(process.terminationSignal() == -1);

    REQUIRE(process.terminationAttempts() == 0);
    REQUIRE(process.killAttempts() == 0);
}

TEST_CASE("JobProcess stores program and arguments", "[job_io][process][usage][config]")
{
    JobProcess process;

    process.setProgram("/bin/echo");
    process.setArguments({"one", "two", "three"});

    REQUIRE(process.program() == "/bin/echo");
    REQUIRE(process.arguments().size() == 3);
    REQUIRE(process.arguments()[0] == "one");
    REQUIRE(process.arguments()[1] == "two");
    REQUIRE(process.arguments()[2] == "three");
}

TEST_CASE("JobProcess exposes its process environment", "[job_io][process][usage][environment]")
{
    JobProcess process;

    REQUIRE_FALSE(process.environ().initial().empty());
    REQUIRE_FALSE(process.environ().resolved().empty());
    REQUIRE(process.environ().initial().size() == process.environ().resolved().size());
}

TEST_CASE("JobProcess runs a simple process", "[job_io][process][usage][start]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "sleep 0.05; exit 0"});

    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    std::atomic<int> startedPid{-1};

    const auto startedConnection = process.started.connect([&](int pid) {
        startedPid.store(pid, std::memory_order_release);
        started.store(true, std::memory_order_release);
    });

    const auto finishedConnection = process.finished.connect([&]() {
        finished.store(true, std::memory_order_release);
    });

    REQUIRE(startedConnection);
    REQUIRE(finishedConnection);

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return started.load(std::memory_order_acquire);
    }));

    REQUIRE(startedPid.load(std::memory_order_acquire) > 0);

    REQUIRE(waitUntil([&]() {
        return finished.load(std::memory_order_acquire);
    }));

    REQUIRE(process.state() == JobProcess::State::Finished);
    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 0);
    REQUIRE_FALSE(process.wasSignaled());
    REQUIRE(process.terminationSignal() == -1);
}

TEST_CASE("JobProcess captures process output", "[job_io][process][usage][output]")
{
    JobProcess process;

    process.setProgram("/bin/echo");
    process.setArguments({"JOB-PROCESS-OUTPUT"});

    std::atomic<bool> sawOutput{false};
    std::atomic<bool> finished{false};

    std::mutex outputMutex;
    std::string output;

    const auto readyReadConnection = process.readyRead.connect([&](std::string_view data) {
        std::scoped_lock lock(outputMutex);
        output.append(data);

        if (output.find("JOB-PROCESS-OUTPUT") != std::string::npos)
            sawOutput.store(true, std::memory_order_release);
    });

    const auto finishedConnection = process.finished.connect([&]() {
        finished.store(true, std::memory_order_release);
    });

    REQUIRE(readyReadConnection);
    REQUIRE(finishedConnection);
    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return sawOutput.load(std::memory_order_acquire);
    }));

    REQUIRE(waitUntil([&]() {
        return finished.load(std::memory_order_acquire);
    }));

    {
        std::scoped_lock lock(outputMutex);
        REQUIRE(output.find("JOB-PROCESS-OUTPUT") != std::string::npos);
    }

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 0);
}

TEST_CASE("JobProcess reports a nonzero exit code", "[job_io][process][usage][result]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "exit 42"});

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 42);
    REQUIRE_FALSE(process.wasSignaled());
    REQUIRE(process.terminationSignal() == -1);
}

TEST_CASE("JobProcess emits lifecycle state changes", "[job_io][process][usage][state][signal]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "sleep 0.05"});

    std::mutex stateMutex;
    std::vector<JobProcess::State> states;

    const auto stateChangedConnection = process.stateChanged.connect([&](JobProcess::State state) {
        std::scoped_lock lock(stateMutex);
        states.push_back(state);
    });

    REQUIRE(stateChangedConnection);
    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    std::scoped_lock lock(stateMutex);

    REQUIRE_FALSE(states.empty());
    REQUIRE(states.front() == JobProcess::State::Starting);
    REQUIRE(states.back() == JobProcess::State::Finished);
}

TEST_CASE("JobProcess delegates terminal controls", "[job_io][process][usage][terminal]")
{
    JobProcess process;

    REQUIRE_NOTHROW(process.setWindowSize(48, 132));
    REQUIRE_NOTHROW(process.setLocalEcho(false));
    REQUIRE_NOTHROW(process.setNonBlocking(false));
    REQUIRE_NOTHROW(process.setNonBlocking(true));
}

TEST_CASE("JobProcess read and write reject a closed process terminal", "[job_io][process][usage][io]")
{
    JobProcess process;

    char buffer[16]{};
    const std::string data = "hello";

    REQUIRE(process.read(buffer, sizeof(buffer)) == -1);
    REQUIRE(process.write(data.data(), data.size()) == -1);
}

// =============================================================================
// Block 2: Process lifetime / termination
// =============================================================================

TEST_CASE("JobProcess terminate sends SIGTERM", "[job_io][process][usage][terminate]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "while true; do sleep 1; done"});

    REQUIRE(process.start());
    REQUIRE(process.pid() > 0);
    REQUIRE(process.isRunning());

    REQUIRE(process.terminationAttempts() == 0);
    REQUIRE(process.terminate());
    REQUIRE(process.terminationAttempts() == 1);

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);

    REQUIRE(process.hasExitStatus());
    REQUIRE_FALSE(process.exitedNormally());
    REQUIRE(process.exitCode() == -1);
    REQUIRE(process.wasSignaled());
    REQUIRE(process.terminationSignal() == SIGTERM);
}

TEST_CASE("JobProcess kill sends SIGKILL", "[job_io][process][usage][kill]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "while true; do sleep 1; done"});

    REQUIRE(process.start());
    REQUIRE(process.pid() > 0);
    REQUIRE(process.isRunning());

    REQUIRE(process.killAttempts() == 0);
    REQUIRE(process.kill());
    REQUIRE(process.killAttempts() == 1);

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);

    REQUIRE(process.hasExitStatus());
    REQUIRE_FALSE(process.exitedNormally());
    REQUIRE(process.exitCode() == -1);
    REQUIRE(process.wasSignaled());
    REQUIRE(process.terminationSignal() == SIGKILL);
}

TEST_CASE("JobProcess repeated terminate attempts are counted", "[job_io][process][usage][terminate][attempts]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "trap '' TERM; while true; do sleep 1; done"});

    REQUIRE(process.start());
    REQUIRE(process.terminate());
    REQUIRE(process.terminationAttempts() == 1);

    if (process.isRunning()) {
        REQUIRE(process.terminate());
        REQUIRE(process.terminationAttempts() == 2);
    }

    if (process.isRunning())
        REQUIRE(process.kill());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));
}

TEST_CASE("JobProcess close does not fabricate an exit result", "[job_io][process][usage][close]")
{
    JobProcess process;

    REQUIRE_NOTHROW(process.close());
    REQUIRE(process.state() == JobProcess::State::NotRunning);
    REQUIRE_FALSE(process.isRunning());
    REQUIRE_FALSE(process.hasExitStatus());
}

// =============================================================================
// Block 3: Edge cases / failure behavior
// =============================================================================

TEST_CASE("JobProcess rejects start without a program", "[job_io][process][edge][start]")
{
    JobProcess process;

    REQUIRE_FALSE(process.start());

    REQUIRE(process.state() == JobProcess::State::NotRunning);
    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);
    REQUIRE_FALSE(process.hasExitStatus());
    REQUIRE_FALSE(process.lastErrorString.empty());
}

TEST_CASE("JobProcess rejects a second start while running", "[job_io][process][edge][start]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "while true; do sleep 1; done"});

    REQUIRE(process.start());
    REQUIRE(process.isRunning());
    REQUIRE(process.pid() > 0);

    REQUIRE_FALSE(process.start());
    REQUIRE_FALSE(process.lastErrorString.empty());

    REQUIRE(process.kill());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));
}

TEST_CASE("JobProcess terminate rejects a process that is not running", "[job_io][process][edge][terminate]")
{
    JobProcess process;

    REQUIRE_FALSE(process.terminate());
    REQUIRE_FALSE(process.lastErrorString.empty());
    REQUIRE(process.terminationAttempts() == 0);
}

TEST_CASE("JobProcess kill rejects a process that is not running", "[job_io][process][edge][kill]")
{
    JobProcess process;

    REQUIRE_FALSE(process.kill());
    REQUIRE_FALSE(process.lastErrorString.empty());
    REQUIRE(process.killAttempts() == 0);
}

TEST_CASE("JobProcess failed exec reports child exit status", "[job_io][process][edge][exec]")
{
    JobProcess process;

    process.setProgram("/this/path/does/not/exist");

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.pid() == -1);
    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 127);
}

TEST_CASE("JobProcess close is idempotent", "[job_io][process][edge][close]")
{
    JobProcess process;

    REQUIRE_NOTHROW(process.close());
    REQUIRE_NOTHROW(process.close());

    REQUIRE(process.state() == JobProcess::State::NotRunning);
    REQUIRE_FALSE(process.isRunning());
    REQUIRE(process.pid() == -1);
}

TEST_CASE("JobProcess can start again after finishing", "[job_io][process][edge][reincarnation]")
{
    JobProcess process;

    process.setProgram("/bin/true");

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 0);
    REQUIRE(process.pid() == -1);

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitedNormally());
    REQUIRE(process.exitCode() == 0);
    REQUIRE(process.pid() == -1);
}

TEST_CASE("JobProcess resets execution result on restart", "[job_io][process][edge][reincarnation][result]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "exit 37"});

    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitCode() == 37);

    process.setArguments({"-c", "sleep 0.1; exit 0"});

    REQUIRE(process.start());

    REQUIRE_FALSE(process.hasExitStatus());
    REQUIRE(process.terminationAttempts() == 0);
    REQUIRE(process.killAttempts() == 0);

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(process.hasExitStatus());
    REQUIRE(process.exitCode() == 0);
}

TEST_CASE("JobProcess emits started exactly once", "[job_io][process][edge][signals][started]")
{
    JobProcess process;

    process.setProgram("/bin/sh");
    process.setArguments({"-c", "sleep 0.05"});

    std::atomic<std::size_t> startedCount{0};

    const auto startedConnection = process.started.connect([&](int) {
        startedCount.fetch_add(1, std::memory_order_release);
    });

    REQUIRE(startedConnection);
    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    REQUIRE(startedCount.load(std::memory_order_acquire) == 1);
}

TEST_CASE("JobProcess emits finished exactly once", "[job_io][process][edge][signals][finished]")
{
    JobProcess process;

    process.setProgram("/bin/true");

    std::atomic<std::size_t> finishedCount{0};

    const auto finishedConnection = process.finished.connect([&]() {
        finishedCount.fetch_add(1, std::memory_order_release);
    });

    REQUIRE(finishedConnection);
    REQUIRE(process.start());

    REQUIRE(waitUntil([&]() {
        return process.state() == JobProcess::State::Finished;
    }));

    std::this_thread::sleep_for(50ms);

    REQUIRE(finishedCount.load(std::memory_order_acquire) == 1);
}

TEST_CASE("JobProcess short lived process publishes sane final state", "[job_io][process][edge][race]")
{
    constexpr std::size_t ITERATIONS = 100;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("Short-lived process iteration: " << iteration);

        JobProcess process;

        process.setProgram("/bin/true");

        std::atomic<std::size_t> startedCount{0};
        std::atomic<std::size_t> finishedCount{0};

        const auto startedConnection = process.started.connect([&](int) {
            startedCount.fetch_add(1, std::memory_order_release);
        });

        const auto finishedConnection = process.finished.connect([&]() {
            finishedCount.fetch_add(1, std::memory_order_release);
        });

        REQUIRE(startedConnection);
        REQUIRE(finishedConnection);
        REQUIRE(process.start());

        REQUIRE(waitUntil([&]() {
            return process.state() == JobProcess::State::Finished;
        }));

        REQUIRE(finishedCount.load(std::memory_order_acquire) == 1);
        REQUIRE(process.pid() == -1);
        REQUIRE(process.hasExitStatus());
        REQUIRE(process.exitedNormally());
        REQUIRE(process.exitCode() == 0);

        /*
         * A process can be reaped before start() can publish a live PID. The
         * finished signal is mandatory; started is only meaningful when a live
         * PID was observable during start().
         */
        REQUIRE(startedCount.load(std::memory_order_acquire) <= 1);
    }
}

TEST_CASE("JobProcess destruction owns cleanup of a running child", "[job_io][process][edge][lifetime][destructor]")
{
    pid_t childPid = -1;

    {
        JobProcess process;

        process.setProgram("/bin/sh");
        process.setArguments({"-c", "while true; do sleep 1; done"});

        REQUIRE(process.start());

        childPid = static_cast<pid_t>(process.pid());

        REQUIRE(childPid > 0);
    }

    REQUIRE(waitUntil([&]() {
        errno = 0;

        if (::kill(childPid, 0) == -1)
            return errno == ESRCH;

        return false;
    }));
}

TEST_CASE("JobProcess program and arguments serialize without runtime state",
          "[job_io][process][edge][serialization]")
{
    JobProcess process;

    process.setProgram("/usr/bin/g++");
    process.setArguments({"-std=c++26", "-c", "foo.cpp"});

    const auto json = process.toJson();

    REQUIRE(json.is_object());
    REQUIRE_FALSE(json.empty());

    REQUIRE(json.contains("m_program"));
    REQUIRE(json.contains("m_arguments"));

    REQUIRE(json["m_program"] == "/usr/bin/g++");
    REQUIRE(json["m_arguments"].is_array());
    REQUIRE(json["m_arguments"].size() == 3);

    const std::string serialized = json.dump();

    REQUIRE(serialized.find("m_pty") == std::string::npos);
    REQUIRE(serialized.find("m_state") == std::string::npos);
    REQUIRE(serialized.find("m_waitStatus") == std::string::npos);
    REQUIRE(serialized.find("m_hasWaitStatus") == std::string::npos);
    REQUIRE(serialized.find("m_startPublished") == std::string::npos);
    REQUIRE(serialized.find("m_terminationAttempts") == std::string::npos);
    REQUIRE(serialized.find("m_killAttempts") == std::string::npos);
}

TEST_CASE("JobProcess factories create process objects", "[job_io][process][edge][factory]")
{
    const auto shared = JobProcess::createShared();
    const auto unique = JobProcess::createUniq();

    REQUIRE(shared != nullptr);
    REQUIRE(unique != nullptr);

    REQUIRE(shared->state() == JobProcess::State::NotRunning);
    REQUIRE(unique->state() == JobProcess::State::NotRunning);

    REQUIRE(shared->pid() == -1);
    REQUIRE(unique->pid() == -1);
}

// =============================================================================
// Block 4: Benchmarks / stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobProcess survives repeated reincarnation", "[job_io][process][stress][reincarnation]")
{
    constexpr std::size_t ITERATIONS = 100;

    JobProcess process;

    process.setProgram("/bin/true");

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobProcess reincarnation iteration: " << iteration);

        REQUIRE(process.start());

        REQUIRE(waitUntil([&]() {
            return process.state() == JobProcess::State::Finished;
        }));

        REQUIRE(process.pid() == -1);
        REQUIRE(process.hasExitStatus());
        REQUIRE(process.exitedNormally());
        REQUIRE(process.exitCode() == 0);
        REQUIRE_FALSE(process.isRunning());
    }
}

TEST_CASE("JobProcess signal delivery remains single shot under stress",
          "[job_io][process][stress][signals]")
{
    constexpr std::size_t ITERATIONS = 100;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobProcess signal iteration: " << iteration);

        JobProcess process;

        process.setProgram("/bin/true");

        std::atomic<std::size_t> finishedCount{0};

        const auto finishedConnection = process.finished.connect([&]() {
            finishedCount.fetch_add(1, std::memory_order_release);
        });

        REQUIRE(finishedConnection);
        REQUIRE(process.start());

        REQUIRE(waitUntil([&]() {
            return process.state() == JobProcess::State::Finished;
        }));

        REQUIRE(finishedCount.load(std::memory_order_acquire) == 1);
    }
}

TEST_CASE("JobProcess terminate and reincarnate remains stable",
          "[job_io][process][stress][terminate][reincarnation]")
{
    constexpr std::size_t ITERATIONS = 50;

    JobProcess process;

    for (std::size_t iteration = 0; iteration < ITERATIONS; ++iteration) {
        INFO("JobProcess terminate iteration: " << iteration);

        process.setProgram("/bin/sh");
        process.setArguments({"-c", "while true; do sleep 1; done"});

        REQUIRE(process.start());
        REQUIRE(process.pid() > 0);
        REQUIRE(process.terminate());

        REQUIRE(waitUntil([&]() {
            return process.state() == JobProcess::State::Finished;
        }));

        REQUIRE(process.pid() == -1);
        REQUIRE(process.hasExitStatus());
        REQUIRE(process.wasSignaled());
        REQUIRE(process.terminationSignal() == SIGTERM);
    }
}

TEST_CASE("JobProcess benchmarks", "[job_io][process][benchmark]")
{
    BENCHMARK("JobProcess construction")
    {
        JobProcess process;
        return process.state() == JobProcess::State::NotRunning;
    };
}

#endif

} // namespace job::io::test
