#include <catch2/catch_test_macros.hpp>

// OLD DEPERCIATED
#include <atomic>
#include <thread>
#include <vector>
#include <functional>

#include <job_thread.h>
#include <job_task_queue.h>

using namespace job::threads;
using namespace std::chrono_literals;

TEST_CASE("TaskQueue basic post, take, and priority", "[threading][task_queue]")
{
    TaskQueue queue;
    std::atomic<int> result = 0;

    // low prio
    queue.post([&]{ result = 1; }, 10);

    // high prio
    queue.post([&]{ result = 2; }, 1);

    REQUIRE(queue.size() == 2);
    REQUIRE_FALSE(queue.isEmpty());

    auto task1 = queue.take();
    task1();
    REQUIRE(result == 2);

    auto task2 = queue.take();
    task2();
    REQUIRE(result == 1);

    REQUIRE(queue.size() == 0);
    REQUIRE(queue.isEmpty());
}

TEST_CASE("TaskQueue take blocks and stop unblocks", "[threading][task_queue]")
{
    TaskQueue queue;
    std::atomic<bool> consumer_finished{false};

    JobThread consumer;
    consumer.setRunFunction([&]([[maybe_unused]]std::stop_token token){
        auto task = queue.take();
        consumer_finished.store(true);
    });
    REQUIRE(consumer.start() == JobThread::StartResult::Started);

    // Tick tock mother F$&^$@
    std::this_thread::sleep_for(10ms);
    REQUIRE_FALSE(consumer_finished.load());

    queue.stop();

    int retries = 0;
    while (!consumer_finished.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(consumer_finished.load() == true);
    REQUIRE(consumer.join());
}

TEST_CASE("TaskQueue take with timeout", "[threading][task_queue]")
{
    TaskQueue queue;

    auto task = queue.take(10ms);
    REQUIRE_FALSE(task.has_value());

    queue.post([]{}, 1);
    auto task2 = queue.take(10ms);
    REQUIRE(task2.has_value());
    REQUIRE(queue.isEmpty());
}

TEST_CASE("TaskQueue maxSize (backpressure) works", "[threading][task_queue]")
{
    TaskQueue queue;
    queue.setMaxSize(1);
    std::atomic<int> state = 0;

    queue.post([]{}, 1);
    REQUIRE(queue.size() == 1);

    JobThread producer;
    producer.setRunFunction([&]([[maybe_unused]]std::stop_token token){
        state.store(1);
        queue.post([]{}, 2);
        state.store(2);
    });
    REQUIRE(producer.start() == JobThread::StartResult::Started);

    while (state.load() != 1)
        std::this_thread::sleep_for(1ms);

    std::this_thread::sleep_for(10ms);
    REQUIRE(state.load() == 1);

    auto task = queue.take();
    REQUIRE(task.operator bool() == true);

    int retries = 0;
    while (state.load() != 2 && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(state.load() == 2);
    REQUIRE(queue.size() == 1);
    REQUIRE(producer.join());
}

TEST_CASE("TaskQueue push_range", "[threading][task_queue]")
{
    using TaskItem = std::pair<int, std::function<void()>>;
    std::vector<TaskItem> tasks;
    std::atomic<int> sum = 0;

    tasks.push_back({2, [&]{ sum += 1; }});
    tasks.push_back({1, [&]{ sum += 10; }});
    tasks.push_back({3, [&]{ sum += 100; }});

    TaskQueue queue;
    queue.push_range(tasks.begin(), tasks.end());

    REQUIRE(queue.size() == 3);

    queue.take()();
    REQUIRE(sum.load() == 10);

    queue.take()();
    REQUIRE(sum.load() == 11);

    queue.take()();
    REQUIRE(sum.load() == 111);

    REQUIRE(queue.isEmpty());
}

TEST_CASE("TaskQueue pop and front", "[threading][task_queue]")
{
    TaskQueue queue;
    std::atomic<int> result = 0;

    queue.post([&]{ result = 1; }, 10);
    queue.post([&]{ result = 2; }, 1);

    REQUIRE(queue.size() == 2);
    auto front_task = queue.front();
    REQUIRE(front_task.has_value());

    front_task.value()();
    REQUIRE(result.load() == 2);

    REQUIRE(queue.pop() == true);
    REQUIRE(queue.size() == 1);

    queue.take()();
    REQUIRE(result.load() == 1);
}

TEST_CASE("TaskQueue metrics tracking", "[threading][task_queue]")
{
    TaskQueue queue;

    auto metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 0);
    REQUIRE(metrics.byPriority.size() == JobThreadMetrics::kDefaultPriorityLevels);

    queue.post([]{}, 1);
    queue.post([]{}, 1);
    queue.post([]{}, 3);
    queue.post([]{}, 9);

    queue.post([]{}, -1);
    queue.post([]{}, 15);

    metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 6);

    // Resized to 15 + 1
    REQUIRE(metrics.byPriority.size() == 16);
    REQUIRE(metrics.byPriority[1] == 2);
    REQUIRE(metrics.byPriority[3] == 1);
    REQUIRE(metrics.byPriority[9] == 1);
    REQUIRE(metrics.byPriority[15] == 1);
    REQUIRE(metrics.byPriority[0] == 0);

    auto task = queue.take();
    metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 5);

    REQUIRE(metrics.byPriority[1] == 2);
    REQUIRE(metrics.byPriority[3] == 1);

    REQUIRE(queue.pop() == true);
    metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 4);
    REQUIRE(metrics.byPriority[1] == 1);

    auto task2 = queue.take();
    metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 3);
    REQUIRE(metrics.byPriority[1] == 0);

    queue.clear();
    metrics = queue.snapshotMetrics();
    REQUIRE(metrics.total == 0);
    REQUIRE(metrics.byPriority[3] == 0);
    REQUIRE(metrics.byPriority[15] == 0);
}

TEST_CASE("TaskQueue swap() exchanges all state", "[threading][task_queue][swap]")
{
    TaskQueue queueA;
    queueA.setMaxSize(10);
    queueA.post([&]{}, 1);
    queueA.post([&]{}, 1);
    queueA.post([&]{}, 3);

    TaskQueue queueB;
    queueB.setMaxSize(20);
    queueB.post([&]{}, 5);

    queueB.stop();

    auto metricsA_before = queueA.snapshotMetrics();
    REQUIRE(metricsA_before.total == 3);
    REQUIRE(metricsA_before.byPriority[1] == 2);
    REQUIRE(queueA.stopped() == false);

    auto metricsB_before = queueB.snapshotMetrics();
    REQUIRE(metricsB_before.total == 1);
    REQUIRE(metricsB_before.byPriority[5] == 1);
    REQUIRE(queueB.stopped() == true);

    // Slight of hand :P .... magic
    queueA.swap(queueB);

    // A is now B's state
    auto metricsA_after = queueA.snapshotMetrics();
    REQUIRE(metricsA_after.total == 1);
    REQUIRE(metricsA_after.byPriority[5] == 1);
    REQUIRE(queueA.stopped() == true);

    auto taskA = queueA.take(1ms);
    REQUIRE(taskA.has_value() == true);
    REQUIRE(queueA.isEmpty()); // Now it's empty

    // B is now A's state
    auto metricsB_after = queueB.snapshotMetrics();
    REQUIRE(metricsB_after.total == 3);
    REQUIRE(metricsB_after.byPriority[1] == 2);
    REQUIRE(queueB.stopped() == false);

    auto taskB = queueB.take(1ms);
    REQUIRE(taskB.has_value() == true);
    REQUIRE(queueB.size() == 2);
}
