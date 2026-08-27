#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <job_signal.h>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

// =============================================================================
// Block 1: Usage / Examples
// =============================================================================

TEST_CASE("Signal: Basic connect, emit, and slot execution", "[core][signal][example]")
{
    Signal<int, const std::string&> eventSignal;
    TestReceiver receiver;

    int lambdaValue = 0;

    auto conn1 = eventSignal.connect([&](int val, const std::string&) {
        lambdaValue = val;
    });

    auto conn2 = eventSignal.connect([&receiver](int val, const std::string& str) {
        receiver.handleEvent(val, str);
    });

    REQUIRE(conn1);
    REQUIRE(conn2);

    REQUIRE(conn1.connected());
    REQUIRE(conn2.connected());

    REQUIRE(conn1.id() != 0);
    REQUIRE(conn2.id() != 0);

    REQUIRE(eventSignal.connectionCount() == 2);
    REQUIRE_FALSE(eventSignal.empty());

    eventSignal.emit(42, "JobEventAlpha");

    CHECK(lambdaValue == 42);
    CHECK(receiver.receivedInt == 42);
    CHECK(receiver.receivedStr == "JobEventAlpha");

    conn1.disconnect();

    CHECK_FALSE(conn1.connected());
    CHECK_FALSE(conn1);
    CHECK(eventSignal.connectionCount() == 1);

    eventSignal.emit(100, "JobEventBeta");

    CHECK(lambdaValue == 42);
    CHECK(receiver.receivedInt == 100);
    CHECK(receiver.receivedStr == "JobEventBeta");

    eventSignal.disconnect(conn2);

    CHECK_FALSE(conn2.connected());
    CHECK(eventSignal.connectionCount() == 0);
    CHECK(eventSignal.empty());
}

TEST_CASE("Signal: Multiple listeners receive fan-out invocation", "[core][signal][example][fanout]")
{
    Signal<int> counterSignal;
    int sum = 0;

    const auto c1 = counterSignal.connect([&](int v) { sum += v; });
    const auto c2 = counterSignal.connect([&](int v) { sum += v * 2; });
    const auto c3 = counterSignal.connect([&](int v) { sum += v * 3; });

    REQUIRE(c1);
    REQUIRE(c2);
    REQUIRE(c3);

    REQUIRE(c1.id() != 0);
    REQUIRE(c2.id() != 0);
    REQUIRE(c3.id() != 0);

    REQUIRE(counterSignal.connectionCount() == 3);

    counterSignal.emit(10);

    CHECK(sum == 60);

    counterSignal.disconnectAll();

    CHECK_FALSE(c1.connected());
    CHECK_FALSE(c2.connected());
    CHECK_FALSE(c3.connected());

    CHECK(counterSignal.empty());
    CHECK(counterSignal.connectionCount() == 0);

    counterSignal.emit(10);

    CHECK(sum == 60);
}

TEST_CASE("Signal: Function call operator emits the signal", "[core][signal][example][operator]")
{
    Signal<int, const std::string&> signal;

    int receivedValue = 0;
    std::string receivedString;

    const auto connection = signal.connect([&](int value, const std::string& str) {
        receivedValue = value;
        receivedString = str;
    });

    REQUIRE(connection);

    signal(42, "operator-call");

    CHECK(receivedValue == 42);
    CHECK(receivedString == "operator-call");
}

TEST_CASE("Signal: Reference arguments preserve reference semantics", "[core][signal][example][reference]")
{
    Signal<int&> signal;

    const auto connection = signal.connect([](int& value) {
        value += 10;
    });

    REQUIRE(connection);

    int value = 5;
    signal.emit(value);

    CHECK(value == 15);
}

TEST_CASE("Signal: Const reference arguments fan out without changing the source value", "[core][signal][example][reference]")
{
    Signal<const std::string&> signal;

    std::string first;
    std::string second;

    const auto firstConnection = signal.connect([&](const std::string& value) {
        first = value;
    });

    const auto secondConnection = signal.connect([&](const std::string& value) {
        second = value;
    });

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);

    const std::string source = "shared-reference";
    signal.emit(source);

    CHECK(source == "shared-reference");
    CHECK(first == "shared-reference");
    CHECK(second == "shared-reference");
}

// =============================================================================
// Block 2: Connection Handle Semantics
// =============================================================================

TEST_CASE("Signal: Default Connection is disconnected and harmless", "[core][signal][connection][edge_cases]")
{
    Connection connection;

    CHECK(connection.id() == 0);
    CHECK(connection.flags() == ConnectionFlag::None);
    CHECK_FALSE(connection.isUnique());
    CHECK_FALSE(connection.isSingleShot());
    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);

    REQUIRE_NOTHROW(connection.disconnect());

    CHECK(connection.id() == 0);
    CHECK_FALSE(connection.connected());
}

TEST_CASE("Signal: Connection handle exposes ID, flags, and connected state", "[core][signal][connection]")
{
    Signal<int> signal;

    const auto connection = signal.connect([](int) {}, ConnectionFlag::SingleShot);

    CHECK(connection.id() != 0);
    CHECK(connection.flags() == ConnectionFlag::SingleShot);
    CHECK_FALSE(connection.isUnique());
    CHECK(connection.isSingleShot());
    CHECK(connection.connected());
    CHECK(connection);
    CHECK(signal.connectionCount() == 1);
}

TEST_CASE("Signal: Connection flags can be combined", "[core][signal][connection][flags]")
{
    constexpr ConnectionFlag flags = ConnectionFlag::Unique | ConnectionFlag::SingleShot;

    STATIC_CHECK(hasConnectionFlag(flags, ConnectionFlag::Unique));
    STATIC_CHECK(hasConnectionFlag(flags, ConnectionFlag::SingleShot));
    STATIC_CHECK_FALSE(hasConnectionFlag(flags, ConnectionFlag::None));

    ConnectionFlag mutableFlags = ConnectionFlag::Unique;
    mutableFlags |= ConnectionFlag::SingleShot;

    CHECK(hasConnectionFlag(mutableFlags, ConnectionFlag::Unique));
    CHECK(hasConnectionFlag(mutableFlags, ConnectionFlag::SingleShot));
}

TEST_CASE("Signal: Connection disconnect is explicit and idempotent", "[core][signal][connection][disconnect]")
{
    Signal<int> signal;
    int calls = 0;

    auto connection = signal.connect([&](int) {
        ++calls;
    });

    REQUIRE(connection);
    REQUIRE(signal.connectionCount() == 1);

    signal.emit(1);

    REQUIRE(calls == 1);

    connection.disconnect();

    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit(1);

    CHECK(calls == 1);

    REQUIRE_NOTHROW(connection.disconnect());
    REQUIRE_NOTHROW(connection.disconnect());

    CHECK_FALSE(connection.connected());
    CHECK(signal.connectionCount() == 0);
}

TEST_CASE("Signal: Copied Connection handles reference the same connection", "[core][signal][connection][copy]")
{
    Signal<int> signal;

    auto first = signal.connect([](int) {});
    auto second = first;

    REQUIRE(first);
    REQUIRE(second);

    CHECK(first.id() == second.id());
    CHECK(first.connected());
    CHECK(second.connected());

    second.disconnect();

    CHECK_FALSE(first.connected());
    CHECK_FALSE(second.connected());

    CHECK_FALSE(first);
    CHECK_FALSE(second);

    CHECK(signal.empty());
}

TEST_CASE("Signal: Destroying a Connection handle does not disconnect the signal", "[core][signal][connection][lifetime]")
{
    Signal<int> signal;
    int calls = 0;

    {
        auto connection = signal.connect([&](int) {
            ++calls;
        });

        REQUIRE(connection);
        REQUIRE(signal.connectionCount() == 1);
    }

    CHECK(signal.connectionCount() == 1);

    signal.emit(1);

    CHECK(calls == 1);
}

TEST_CASE("Signal: Signal disconnect invalidates retained Connection handle", "[core][signal][connection][disconnect]")
{
    Signal<int> signal;

    auto connection = signal.connect([](int) {});

    REQUIRE(connection);

    signal.disconnect(connection);

    CHECK_FALSE(connection);
    CHECK_FALSE(connection.connected());
    CHECK(signal.empty());
}

TEST_CASE("Signal: Disconnect by Connection ID invalidates retained handle", "[core][signal][connection][connection_id]")
{
    Signal<int> signal;

    auto connection = signal.connect([](int) {});

    REQUIRE(connection);

    const auto id = connection.id();

    REQUIRE(id != 0);

    signal.disconnect(id);

    CHECK_FALSE(connection);
    CHECK_FALSE(connection.connected());
    CHECK(signal.empty());
}

TEST_CASE("Signal: disconnectAll invalidates every retained handle", "[core][signal][connection][disconnect_all]")
{
    Signal<int> signal;

    auto first = signal.connect([](int) {});
    auto second = signal.connect([](int) {});
    auto third = signal.connect([](int) {});

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);

    signal.disconnectAll();

    CHECK_FALSE(first);
    CHECK_FALSE(second);
    CHECK_FALSE(third);

    CHECK_FALSE(first.connected());
    CHECK_FALSE(second.connected());
    CHECK_FALSE(third.connected());

    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);
}

TEST_CASE("Signal: Retained Connection becomes disconnected when Signal is destroyed", "[core][signal][connection][lifetime]")
{
    Connection connection;

    {
        Signal<int> signal;

        connection = signal.connect([](int) {});

        REQUIRE(connection);
        REQUIRE(connection.connected());
    }

    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);

    REQUIRE_NOTHROW(connection.disconnect());
}

TEST_CASE("Signal: Connection remains safe after sender destruction", "[core][signal][connection][lifetime][sender]")
{
    Connection connection;

    auto signal = std::make_unique<Signal<int>>();

    connection = signal->connect([](int) {});

    REQUIRE(connection);
    REQUIRE(connection.connected());

    signal.reset();

    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);

    REQUIRE_NOTHROW(connection.disconnect());
    REQUIRE_NOTHROW(connection.disconnect());
}

TEST_CASE("Signal: Concurrent disconnect and sender destruction are safe", "[core][signal][connection][lifetime][concurrency]")
{
    constexpr int Iterations = 1'000;

    for (int i = 0; i < Iterations; ++i) {
        auto signal = std::make_unique<Signal<int>>();
        Connection connection = signal->connect([](int) {});

        REQUIRE(connection);

        std::atomic<bool> start{false};

        std::thread worker([&]() {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            connection.disconnect();
        });

        start.store(true, std::memory_order_release);

        signal.reset();

        worker.join();

        CHECK_FALSE(connection.connected());
    }
}

// =============================================================================
// Block 2: Connection Flags - Unique
// =============================================================================

TEST_CASE("Signal: Bare callback cannot honestly use Unique semantics", "[core][signal][unique][edge_cases]")
{
    Signal<int> signal;
    int calls = 0;

    const auto connection = signal.connect([&](int) {
        ++calls;
    }, ConnectionFlag::Unique);

    CHECK_FALSE(connection);
    CHECK_FALSE(connection.connected());
    CHECK(connection.id() == 0);
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit(1);

    CHECK(calls == 0);
}

TEST_CASE("Signal: Unique reflected connection rejects duplicate receiver and slot", "[core][signal][unique][object]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(first);
    REQUIRE(first.isUnique());
    REQUIRE_FALSE(first.isSingleShot());

    auto duplicate =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    CHECK_FALSE(duplicate);
    CHECK_FALSE(duplicate.connected());
    CHECK(duplicate.id() == 0);

    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(7, 12.5);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 7);
    CHECK(controller.lastValue == 12.5);
}

TEST_CASE("Signal: Non-Unique reflected connection permits duplicate receiver and slot", "[core][signal][unique][object]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    auto second =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    REQUIRE(first);
    REQUIRE(second);

    CHECK(first.id() != second.id());
    CHECK(sensor.readingEmitted.connectionCount() == 2);
    CHECK(controller.connectionCount() == 2);

    sensor.emitReading(3, 20.0);

    CHECK(controller.invocationCount == 2);
}

TEST_CASE("Signal: Unique connection can be recreated after disconnect", "[core][signal][unique][disconnect]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(first);

    first.disconnect();

    REQUIRE_FALSE(first);
    REQUIRE(sensor.readingEmitted.empty());

    auto second =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(second);

    CHECK(second.id() != first.id());
    CHECK(second.isUnique());

    sensor.emitReading(8, 44.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 8);
    CHECK(controller.lastValue == 44.0);
}

TEST_CASE("Signal: Unique identity includes receiver identity", "[core][signal][unique][receiver]")
{
    SensorNode sensor;
    ControllerNode firstController;
    ControllerNode secondController;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            firstController,
            ConnectionFlag::Unique);

    auto second =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            secondController,
            ConnectionFlag::Unique);

    REQUIRE(first);
    REQUIRE(second);

    CHECK(first.id() != second.id());
    CHECK(sensor.readingEmitted.connectionCount() == 2);

    sensor.emitReading(10, 1.5);

    CHECK(firstController.invocationCount == 1);
    CHECK(secondController.invocationCount == 1);
}

// =============================================================================
// Block 2: Connection Flags - SingleShot
// =============================================================================

TEST_CASE("Signal: SingleShot callback executes once and disconnects itself", "[core][signal][single_shot]")
{
    Signal<int> signal;
    int calls = 0;

    auto connection = signal.connect([&](int value) {
        calls += value;
    }, ConnectionFlag::SingleShot);

    REQUIRE(connection);
    REQUIRE(connection.isSingleShot());
    REQUIRE_FALSE(connection.isUnique());
    REQUIRE(signal.connectionCount() == 1);

    signal.emit(2);

    CHECK(calls == 2);
    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit(2);

    CHECK(calls == 2);
}

TEST_CASE("Signal: SingleShot disconnects before callback invocation", "[core][signal][single_shot][reentrant]")
{
    Signal<int> signal;

    int calls = 0;
    bool disconnectedInsideCallback = false;

    Connection connection;

    connection = signal.connect([&](int value) {
        ++calls;
        disconnectedInsideCallback = !connection.connected();

        if (value == 1)
            signal.emit(2);
    }, ConnectionFlag::SingleShot);

    REQUIRE(connection);

    signal.emit(1);

    CHECK(calls == 1);
    CHECK(disconnectedInsideCallback);
    CHECK_FALSE(connection);
    CHECK(signal.empty());
}

TEST_CASE("Signal: SingleShot does not prevent ordinary connections from continuing", "[core][signal][single_shot][mixed]")
{
    Signal<int> signal;

    int permanentCalls = 0;
    int singleShotCalls = 0;

    const auto permanent = signal.connect([&](int) {
        ++permanentCalls;
    });

    auto singleShot = signal.connect([&](int) {
        ++singleShotCalls;
    }, ConnectionFlag::SingleShot);

    REQUIRE(permanent);
    REQUIRE(singleShot);

    signal.emit(1);

    CHECK(permanentCalls == 1);
    CHECK(singleShotCalls == 1);

    CHECK(permanent.connected());
    CHECK_FALSE(singleShot.connected());
    CHECK(signal.connectionCount() == 1);

    signal.emit(1);

    CHECK(permanentCalls == 2);
    CHECK(singleShotCalls == 1);
}

TEST_CASE("Signal: Multiple SingleShot callbacks each execute once", "[core][signal][single_shot]")
{
    Signal<> signal;

    int firstCalls = 0;
    int secondCalls = 0;
    int thirdCalls = 0;

    auto first = signal.connect([&]() {
        ++firstCalls;
    }, ConnectionFlag::SingleShot);

    auto second = signal.connect([&]() {
        ++secondCalls;
    }, ConnectionFlag::SingleShot);

    auto third = signal.connect([&]() {
        ++thirdCalls;
    }, ConnectionFlag::SingleShot);

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);

    REQUIRE(signal.connectionCount() == 3);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);
    CHECK(thirdCalls == 1);

    CHECK_FALSE(first);
    CHECK_FALSE(second);
    CHECK_FALSE(third);

    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);
    CHECK(thirdCalls == 1);
}

TEST_CASE("Signal: Explicitly disconnected SingleShot never executes", "[core][signal][single_shot][disconnect]")
{
    Signal<> signal;

    int calls = 0;

    auto connection = signal.connect([&]() {
        ++calls;
    }, ConnectionFlag::SingleShot);

    REQUIRE(connection);

    connection.disconnect();

    CHECK_FALSE(connection);

    signal.emit();

    CHECK(calls == 0);
    CHECK(signal.empty());
}

TEST_CASE("Signal: Unique and SingleShot flags work together", "[core][signal][unique][single_shot][object]")
{
    SensorNode sensor;
    ControllerNode controller;

    constexpr ConnectionFlag Flags = ConnectionFlag::Unique | ConnectionFlag::SingleShot;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    REQUIRE(first);
    REQUIRE(first.isUnique());
    REQUIRE(first.isSingleShot());

    auto duplicate =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    CHECK_FALSE(duplicate);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
    CHECK_FALSE(first);
    CHECK(sensor.readingEmitted.empty());

    auto replacement =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    REQUIRE(replacement);

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 2);
    CHECK(controller.lastValue == 20.0);
    CHECK_FALSE(replacement);
}

TEST_CASE("Signal: Concurrent emit executes SingleShot callback exactly once", "[core][signal][single_shot][concurrency]")
{
    Signal<int> signal;

    std::atomic<int> calls{0};

    auto connection = signal.connect([&](int) {
        calls.fetch_add(1, std::memory_order_relaxed);
    }, ConnectionFlag::SingleShot);

    REQUIRE(connection);

    constexpr int ThreadCount = 8;
    constexpr int EmitsPerThread = 1'000;

    std::atomic<bool> start{false};
    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (int t = 0; t < ThreadCount; ++t) {
        workers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int i = 0; i < EmitsPerThread; ++i)
                signal.emit(1);
        });
    }

    start.store(true, std::memory_order_release);

    for (auto& worker : workers)
        worker.join();

    CHECK(calls.load(std::memory_order_acquire) == 1);
    CHECK_FALSE(connection.connected());
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);
}

// =============================================================================
// Block 2: Edge Cases & Thread Safety
// =============================================================================

TEST_CASE("Signal: Empty and disconnected signals are safe", "[core][signal][edge_cases]")
{
    SECTION("Emitting an empty signal is safe and a no-op")
    {
        Signal<int, double, std::string> emptySignal;

        REQUIRE(emptySignal.empty());
        REQUIRE(emptySignal.connectionCount() == 0);

        REQUIRE_NOTHROW(emptySignal.emit(1, 2.0, "test"));
        REQUIRE_NOTHROW(emptySignal(1, 2.0, "test"));

        CHECK(emptySignal.empty());
    }

    SECTION("Disconnecting an already disconnected or invalid ID is safe")
    {
        Signal<int> signal;
        auto connection = signal.connect([](int) {});

        REQUIRE(connection);
        REQUIRE(signal.connectionCount() == 1);

        signal.disconnect(connection);

        CHECK_FALSE(connection);
        CHECK(signal.connectionCount() == 0);
        CHECK(signal.empty());

        REQUIRE_NOTHROW(signal.disconnect(connection));
        REQUIRE_NOTHROW(signal.disconnect(Connection::ConnectionId{0}));
        REQUIRE_NOTHROW(signal.disconnect(Connection::ConnectionId{999999}));

        CHECK(signal.connectionCount() == 0);
    }

    SECTION("Repeated disconnectAll calls are safe")
    {
        Signal<int> signal;

        const auto first = signal.connect([](int) {});
        const auto second = signal.connect([](int) {});
        const auto third = signal.connect([](int) {});

        REQUIRE(first);
        REQUIRE(second);
        REQUIRE(third);

        REQUIRE(signal.connectionCount() == 3);

        signal.disconnectAll();

        CHECK(signal.empty());
        CHECK(signal.connectionCount() == 0);

        REQUIRE_NOTHROW(signal.disconnectAll());
        REQUIRE_NOTHROW(signal.disconnectAll());

        CHECK(signal.empty());
        CHECK(signal.connectionCount() == 0);
    }

    SECTION("An empty callback can exist without crashing emission")
    {
        Signal<int> signal;

        Signal<int>::Callback callback;
        auto connection = signal.connect(std::move(callback));

        REQUIRE(connection);
        REQUIRE(connection.id() != 0);
        REQUIRE(signal.connectionCount() == 1);

        REQUIRE_NOTHROW(signal.emit(42));

        connection.disconnect();

        CHECK_FALSE(connection);
        CHECK(signal.empty());
    }
}

TEST_CASE("Signal: Connection IDs are nonzero and unique", "[core][signal][edge_cases][connection_id]")
{
    Signal<> signal;

    const auto first = signal.connect([]() {});
    const auto second = signal.connect([]() {});
    const auto third = signal.connect([]() {});

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);

    CHECK(first.id() != 0);
    CHECK(second.id() != 0);
    CHECK(third.id() != 0);

    CHECK(first.id() != second.id());
    CHECK(first.id() != third.id());
    CHECK(second.id() != third.id());
}

TEST_CASE("Signal: Callback invocation follows connection order", "[core][signal][edge_cases][ordering]")
{
    Signal<> signal;
    std::vector<int> invocationOrder;

    const auto first = signal.connect([&]() {
        invocationOrder.push_back(1);
    });

    const auto second = signal.connect([&]() {
        invocationOrder.push_back(2);
    });

    const auto third = signal.connect([&]() {
        invocationOrder.push_back(3);
    });

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(third);

    signal.emit();

    REQUIRE(invocationOrder.size() == 3);
    CHECK(invocationOrder[0] == 1);
    CHECK(invocationOrder[1] == 2);
    CHECK(invocationOrder[2] == 3);
}

TEST_CASE("Signal: Stateful callbacks preserve state across emissions", "[core][signal][edge_cases][stateful]")
{
    Signal<> signal;

    int observedCount = 0;

    const auto connection = signal.connect([count = 0, &observedCount]() mutable {
        observedCount = ++count;
    });

    REQUIRE(connection);

    signal.emit();
    CHECK(observedCount == 1);

    signal.emit();
    CHECK(observedCount == 2);

    signal.emit();
    CHECK(observedCount == 3);
}

TEST_CASE("Signal: Stateful callback state survives COW connection changes", "[core][signal][edge_cases][stateful][cow]")
{
    Signal<> signal;

    int firstObservedCount = 0;
    int secondCalls = 0;

    const auto first = signal.connect([count = 0, &firstObservedCount]() mutable {
        firstObservedCount = ++count;
    });

    REQUIRE(first);

    signal.emit();
    signal.emit();

    REQUIRE(firstObservedCount == 2);

    const auto second = signal.connect([&]() {
        ++secondCalls;
    });

    REQUIRE(second);

    signal.emit();

    CHECK(firstObservedCount == 3);
    CHECK(secondCalls == 1);
}

TEST_CASE("Signal: Callback can disconnect itself during emission", "[core][signal][edge_cases][reentrant]")
{
    Signal<int> signal;

    Connection connection;
    int calls = 0;

    connection = signal.connect([&](int) {
        ++calls;
        connection.disconnect();
    });

    REQUIRE(connection);
    REQUIRE(signal.connectionCount() == 1);

    signal.emit(1);

    CHECK(calls == 1);
    CHECK_FALSE(connection);
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit(1);

    CHECK(calls == 1);
}

TEST_CASE("Signal: Disconnect during emission affects subsequent emissions", "[core][signal][edge_cases][reentrant][snapshot]")
{
    Signal<> signal;

    int firstCalls = 0;
    int secondCalls = 0;

    Connection secondConnection;

    const auto firstConnection = signal.connect([&]() {
        ++firstCalls;
        secondConnection.disconnect();
    });

    secondConnection = signal.connect([&]() {
        ++secondCalls;
    });

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);
    REQUIRE(signal.connectionCount() == 2);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);
    CHECK(signal.connectionCount() == 1);

    CHECK(firstConnection.connected());
    CHECK_FALSE(secondConnection.connected());

    signal.emit();

    CHECK(firstCalls == 2);
    CHECK(secondCalls == 1);
}

TEST_CASE("Signal: Connections added during emission begin with the next emission", "[core][signal][edge_cases][reentrant][snapshot]")
{
    Signal<> signal;

    int firstCalls = 0;
    int lateCalls = 0;

    Connection lateConnection;

    const auto firstConnection = signal.connect([&]() {
        ++firstCalls;

        if (!lateConnection) {
            lateConnection = signal.connect([&]() {
                ++lateCalls;
            });
        }
    });

    REQUIRE(firstConnection);
    REQUIRE(signal.connectionCount() == 1);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(lateCalls == 0);
    CHECK(lateConnection.connected());
    CHECK(signal.connectionCount() == 2);

    signal.emit();

    CHECK(firstCalls == 2);
    CHECK(lateCalls == 1);

    signal.emit();

    CHECK(firstCalls == 3);
    CHECK(lateCalls == 2);
}

TEST_CASE("Signal: disconnectAll can be called during emission", "[core][signal][edge_cases][reentrant][snapshot]")
{
    Signal<> signal;

    int firstCalls = 0;
    int secondCalls = 0;

    const auto firstConnection = signal.connect([&]() {
        ++firstCalls;
        signal.disconnectAll();
    });

    const auto secondConnection = signal.connect([&]() {
        ++secondCalls;
    });

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);
    REQUIRE(signal.connectionCount() == 2);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);

    CHECK_FALSE(firstConnection.connected());
    CHECK_FALSE(secondConnection.connected());

    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);
}

TEST_CASE("Signal: Disconnecting an earlier callback from a later callback is safe", "[core][signal][edge_cases][reentrant]")
{
    Signal<> signal;

    int firstCalls = 0;
    int secondCalls = 0;

    Connection firstConnection;

    firstConnection = signal.connect([&]() {
        ++firstCalls;
    });

    const auto secondConnection = signal.connect([&]() {
        ++secondCalls;
        firstConnection.disconnect();
    });

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 1);

    CHECK_FALSE(firstConnection.connected());
    CHECK(secondConnection.connected());

    CHECK(signal.connectionCount() == 1);

    signal.emit();

    CHECK(firstCalls == 1);
    CHECK(secondCalls == 2);
}

TEST_CASE("Signal: Nested emission is safe", "[core][signal][edge_cases][reentrant][nested_emit]")
{
    Signal<int> signal;

    std::vector<int> received;

    const auto connection = signal.connect([&](int value) {
        received.push_back(value);

        if (value == 1)
            signal.emit(2);
    });

    REQUIRE(connection);

    signal.emit(1);

    REQUIRE(received.size() == 2);
    CHECK(received[0] == 1);
    CHECK(received[1] == 2);
}

TEST_CASE("Signal: Multiple listeners receive independent value arguments", "[core][signal][edge_cases][arguments]")
{
    Signal<std::string> signal;

    std::string first;
    std::string second;

    const auto firstConnection = signal.connect([&](std::string value) {
        first = std::move(value);
    });

    const auto secondConnection = signal.connect([&](std::string value) {
        second = std::move(value);
    });

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);

    signal.emit("fanout-value");

    CHECK(first == "fanout-value");
    CHECK(second == "fanout-value");
}

TEST_CASE("Signal: Concurrent emission across multiple threads", "[core][signal][edge_cases][concurrency]")
{
    Signal<int> concurrentSignal;
    std::atomic<std::int64_t> totalSum{0};

    const auto connection = concurrentSignal.connect([&](int value) {
        totalSum.fetch_add(value, std::memory_order_relaxed);
    });

    REQUIRE(connection);

    constexpr int ThreadCount = 8;
    constexpr int EmitsPerThread = 5'000;

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (int t = 0; t < ThreadCount; ++t) {
        workers.emplace_back([&concurrentSignal]() {
            for (int i = 0; i < EmitsPerThread; ++i)
                concurrentSignal.emit(1);
        });
    }

    for (auto& worker : workers)
        worker.join();

    CHECK(totalSum.load(std::memory_order_acquire) ==
          static_cast<std::int64_t>(ThreadCount) * EmitsPerThread);
}

TEST_CASE("Signal: Concurrent connect and disconnect operations are safe", "[core][signal][edge_cases][concurrency]")
{
    Signal<int> signal;

    constexpr int ThreadCount = 8;
    constexpr int OperationsPerThread = 2'000;

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (int t = 0; t < ThreadCount; ++t) {
        workers.emplace_back([&signal]() {
            for (int i = 0; i < OperationsPerThread; ++i) {
                auto connection = signal.connect([](int) {});
                connection.disconnect();
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);
}

TEST_CASE("Signal: Concurrent emit and connection churn remain stable", "[core][signal][edge_cases][concurrency][stress]")
{
    Signal<int> signal;

    std::atomic<bool> start{false};
    std::atomic<std::int64_t> callbackExecutions{0};

    const auto permanentConnection = signal.connect([&](int) {
        callbackExecutions.fetch_add(1, std::memory_order_relaxed);
    });

    REQUIRE(permanentConnection);

    constexpr int EmitterThreadCount = 4;
    constexpr int ChurnThreadCount = 4;
    constexpr int OperationsPerThread = 5'000;

    std::vector<std::thread> workers;
    workers.reserve(EmitterThreadCount + ChurnThreadCount);

    for (int t = 0; t < EmitterThreadCount; ++t) {
        workers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int i = 0; i < OperationsPerThread; ++i)
                signal.emit(1);
        });
    }

    for (int t = 0; t < ChurnThreadCount; ++t) {
        workers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire))
                std::this_thread::yield();

            for (int i = 0; i < OperationsPerThread; ++i) {
                auto connection = signal.connect([](int) {});
                connection.disconnect();
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (auto& worker : workers)
        worker.join();

    CHECK(callbackExecutions.load(std::memory_order_acquire) ==
          static_cast<std::int64_t>(EmitterThreadCount) * OperationsPerThread);

    signal.disconnectAll();

    CHECK_FALSE(permanentConnection.connected());
    CHECK(signal.empty());
    CHECK(signal.connectionCount() == 0);
}

// =============================================================================
// Block 3: Benchmarks / Stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Signal benchmarks", "[core][signal][benchmark]")
{
    Signal<int> emptySignal;

    BENCHMARK("Signal emit (0 connected slots)")
    {
        emptySignal.emit(1);
        return emptySignal.empty();
    };

    Signal<int> singleSlotSignal;
    int accumulator = 0;

    const auto singleConnection = singleSlotSignal.connect([&](int value) {
        accumulator += value;
    });

    REQUIRE(singleConnection);

    BENCHMARK("Signal emit (1 connected slot)")
    {
        singleSlotSignal.emit(1);
        return accumulator;
    };

    Signal<int> fanoutSignal;
    int fanoutAccumulator = 0;

    std::vector<Connection> fanoutConnections;
    fanoutConnections.reserve(8);

    for (int i = 0; i < 8; ++i) {
        fanoutConnections.push_back(fanoutSignal.connect([&](int value) {
            fanoutAccumulator += value;
        }));
    }

    REQUIRE(fanoutSignal.connectionCount() == 8);

    BENCHMARK("Signal emit (8 fan-out slots)")
    {
        fanoutSignal.emit(1);
        return fanoutAccumulator;
    };

    Signal<int> largeFanoutSignal;
    int largeFanoutAccumulator = 0;

    std::vector<Connection> largeFanoutConnections;
    largeFanoutConnections.reserve(32);

    for (int i = 0; i < 32; ++i) {
        largeFanoutConnections.push_back(largeFanoutSignal.connect([&](int value) {
            largeFanoutAccumulator += value;
        }));
    }

    REQUIRE(largeFanoutSignal.connectionCount() == 32);

    BENCHMARK("Signal emit (32 fan-out slots)")
    {
        largeFanoutSignal.emit(1);
        return largeFanoutAccumulator;
    };

    BENCHMARK("Connect & disconnect churn")
    {
        Signal<int> signal;
        auto connection = signal.connect([](int) {});
        connection.disconnect();
        return signal.empty();
    };
}

TEST_CASE("Signal flag benchmarks", "[core][signal][benchmark][flags]")
{
    Signal<int> normalSignal;
    int normalAccumulator = 0;

    const auto normalConnection = normalSignal.connect([&](int value) {
        normalAccumulator += value;
    });

    REQUIRE(normalConnection);

    BENCHMARK("Signal ordinary emit with SingleShot fast-path check")
    {
        normalSignal.emit(1);
        return normalAccumulator;
    };

    BENCHMARK("Signal SingleShot connect and first emit")
    {
        Signal<int> signal;
        int calls = 0;

        auto connection = signal.connect([&](int) {
            ++calls;
        }, ConnectionFlag::SingleShot);

        signal.emit(1);

        return calls + static_cast<int>(connection.connected());
    };
}

TEST_CASE("Signal concurrent stress benchmark", "[core][signal][benchmark][stress]")
{
    constexpr int ThreadCount = 8;
    constexpr int EmitsPerThread = 10'000;

    Signal<int> signal;
    std::atomic<std::int64_t> total{0};

    const auto connection = signal.connect([&](int value) {
        total.fetch_add(value, std::memory_order_relaxed);
    });

    REQUIRE(connection);

    std::atomic_bool stop{false};
    std::atomic<std::uint64_t> generation{0};
    std::atomic<int> ready{0};
    std::atomic<int> finished{0};

    std::vector<std::thread> workers;
    workers.reserve(ThreadCount);

    for (int t = 0; t < ThreadCount; ++t) {
        workers.emplace_back([&]() {
            std::uint64_t localGeneration = generation.load(std::memory_order_acquire);

            ready.fetch_add(1, std::memory_order_release);
            ready.notify_one();

            while (true) {
                generation.wait(localGeneration, std::memory_order_acquire);

                if (stop.load(std::memory_order_acquire))
                    break;

                localGeneration = generation.load(std::memory_order_acquire);

                for (int i = 0; i < EmitsPerThread; ++i)
                    signal.emit(1);

                finished.fetch_add(1, std::memory_order_release);
                finished.notify_one();
            }
        });
    }

    while (ready.load(std::memory_order_acquire) != ThreadCount) {
        const int current = ready.load(std::memory_order_relaxed);
        ready.wait(current, std::memory_order_acquire);
    }

    BENCHMARK("Concurrent emission (8 threads x 10k emits)")
    {
        total.store(0, std::memory_order_relaxed);
        finished.store(0, std::memory_order_relaxed);

        generation.fetch_add(1, std::memory_order_release);
        generation.notify_all();

        while (finished.load(std::memory_order_acquire) != ThreadCount) {
            const int current = finished.load(std::memory_order_relaxed);
            finished.wait(current, std::memory_order_acquire);
        }

        return total.load(std::memory_order_relaxed);
    };

    stop.store(true, std::memory_order_release);
    generation.fetch_add(1, std::memory_order_release);
    generation.notify_all();

    for (auto& worker : workers)
        worker.join();

    CHECK(total.load(std::memory_order_relaxed) ==
          static_cast<std::int64_t>(ThreadCount) * EmitsPerThread);
}

#endif