#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>

#include <job_async_event_loop.h>

using namespace job::threads;
using namespace std::chrono_literals;


#ifdef JOB_LINUX
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cerrno>

#include <sys/socket.h>
#include <unistd.h>

#include <job_io_async_thread.h>

#include "../test_spin_till.h"

using namespace job::threads;
using namespace std::chrono_literals;

namespace {
class SocketPair final {
public:
    SocketPair()
    {
        REQUIRE(
            ::socketpair(
                AF_UNIX,
                SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                0,
                m_fds
                ) == 0
            );
    }

    ~SocketPair()
    {
        if (m_fds[0] >= 0)
            ::close(m_fds[0]);

        if (m_fds[1] >= 0)
            ::close(m_fds[1]);
    }

    SocketPair(const SocketPair &) = delete;
    SocketPair &operator=(const SocketPair &) = delete;

    [[nodiscard]] int first() const noexcept
    {
        return m_fds[0];
    }

    [[nodiscard]] int second() const noexcept
    {
        return m_fds[1];
    }

private:
    int m_fds[2]{-1, -1};
};
} // namespace

TEST_CASE("JobIoAsyncThread modifyFD changes event interest", "[threading][io_async][modify_fd]")
{
    JobIoAsyncThread loop;
    SocketPair sockets;

    std::atomic<int> writeEvents{0};
    std::atomic<int> readEvents{0};
    std::atomic<bool> modified{false};

    loop.start();
    REQUIRE(loop.isRunning());

    REQUIRE(
        loop.registerFD( sockets.first(), IOEvent::Write,
                        [&](IOEvent events) {
                            if (hasEvent(events, IOEvent::Write)) {
                                writeEvents.fetch_add(1, std::memory_order_relaxed);
                                modified.store(loop.modifyFD(sockets.first(), IOEvent::Read), std::memory_order_release);
                            }

                            if (hasEvent(events, IOEvent::Read)) {
                                char buffer[32];
                                for (;;) {
                                    const ssize_t result = ::recv( sockets.first(), buffer, sizeof(buffer), 0);

                                    if (result > 0) {
                                        readEvents.fetch_add(1, std::memory_order_relaxed);
                                        continue;
                                    }

                                    if (result < 0 && errno == EINTR)
                                        continue;

                                    if (result < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                                        break;

                                    break;
                                }
                            }
                        }));

    REQUIRE(
        spin_until([&] { return writeEvents.load(std::memory_order_relaxed) > 0 && modified.load(std::memory_order_acquire); }, 500ms)
        );

    const char value = 'x';
    REQUIRE(::send( sockets.second(), &value, sizeof(value), 0 ) == sizeof(value));

    REQUIRE(
        spin_until([&] { return readEvents.load( std::memory_order_relaxed ) > 0; }, 500ms ));

    REQUIRE(writeEvents.load() >= 1);
    REQUIRE(readEvents.load() >= 1);

    REQUIRE(loop.unregisterFD(sockets.first()));

    loop.stop();
    REQUIRE_FALSE(loop.isRunning());
}

TEST_CASE("JobIoAsyncThread modifyFD rejects invalid fd", "[threading][io_async][modify_fd]")
{
    JobIoAsyncThread loop;

    loop.start();

    REQUIRE_FALSE(loop.modifyFD(-1, IOEvent::Read));

    loop.stop();
}

TEST_CASE("JobIoAsyncThread modifyFD rejects unregistered fd", "[threading][io_async][modify_fd]")
{
    JobIoAsyncThread loop;
    SocketPair sockets;
    loop.start();
    REQUIRE_FALSE(loop.modifyFD(sockets.first(), IOEvent::Read));
    loop.stop();
}

#endif


TEST_CASE("AsyncEventLoop post and stop", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_ran{false};

    loop.start();
    REQUIRE(loop.isRunning());

    loop.post([&] {
        task_ran.store(true);
    });

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(task_ran.load() == true);
    loop.stop();
    REQUIRE_FALSE(loop.isRunning());
}

TEST_CASE("AsyncEventLoop postDelayed", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_ran{false};
    auto start_time = std::chrono::steady_clock::now();

    loop.start();
    loop.postDelayed([&] {
        task_ran.store(true);
    }, 50ms);

    REQUIRE_FALSE(task_ran.load());
    std::this_thread::sleep_for(25ms);
    REQUIRE_FALSE(task_ran.load());

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    REQUIRE(task_ran.load() == true);
    REQUIRE(duration.count() >= 50);

    loop.stop();
}

TEST_CASE("AsyncEventLoop repeating timer", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<int> counter{0};

    loop.start();

    uint64_t timer_id = loop.addTimer([&] {
        counter++;
    }, 20ms, true);

    REQUIRE(timer_id > 0);

    std::this_thread::sleep_for(50ms);

    REQUIRE(counter.load() >= 2);
    REQUIRE(counter.load() <= 3);

    loop.stop();
}

TEST_CASE("AsyncEventLoop cancelTimer", "[threading][async_loop]")
{
    AsyncEventLoop loop;
    std::atomic<int> counter{0};

    loop.start();

    uint64_t timer_id = loop.addTimer([&] {
        counter++;
    }, 20ms, true);

    std::this_thread::sleep_for(50ms);

    bool canceled = loop.cancelTimer(timer_id);
    REQUIRE(canceled == true);

    int count_after_cancel = counter.load();
    REQUIRE(count_after_cancel >= 2);

    std::this_thread::sleep_for(50ms);

    REQUIRE(counter.load() == count_after_cancel);

    bool canceled_again = loop.cancelTimer(timer_id);
    REQUIRE(canceled_again == false);

    loop.stop();
}

TEST_CASE("AsyncEventLoop globalLoop", "[threading][async_loop]")
{
    auto &loop = AsyncEventLoop::globalLoop();
    REQUIRE(loop.isRunning());

    std::atomic<bool> task_ran{false};
    loop.post([&] { task_ran.store(true); });

    int retries = 0;
    while (!task_ran.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(task_ran.load() == true);
    // We don't stop the global loop Because well ....... it's global
}

TEST_CASE("AsyncEventLoop handles re-entrancy (post from a timer)", "[threading][async_loop][reentrancy]")
{
    AsyncEventLoop loop;
    std::atomic<bool> task_from_timer_ran{false};
    loop.start();
    loop.postDelayed([&] {
        loop.post([&] {
            task_from_timer_ran.store(true);
        });
    }, 10ms);

    int retries = 0;
    // this needs to go somewhere else . . .
    while (!task_from_timer_ran.load() && retries < 100) {
        std::this_thread::sleep_for(2ms);
        retries++;
    }

    REQUIRE(task_from_timer_ran.load() == true);
    loop.stop();
}
