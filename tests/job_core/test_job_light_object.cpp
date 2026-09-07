#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <concepts>
#include <memory>
#include <string>
#include <type_traits>

#include <job_light_object.h>
#include <job_signal.h>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

namespace job::core::tests {
struct PureLightNode final : public LightObject
{
    PureLightNode() = default;
    ~PureLightNode() override = default;

    PureLightNode(const PureLightNode &) = delete;
    PureLightNode &operator=(const PureLightNode &) = delete;
    PureLightNode(PureLightNode &&) = delete;
    PureLightNode &operator=(PureLightNode &&) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return true;
    }
};

struct StringLightNode final : public LightObject
{
    std::string value{"temperature_zone_1"};

    [[nodiscard]] bool isValid() const noexcept
    {
        return !value.empty();
    }
};

class RuntimeLightSensorNode final : public LightObject
{
public:
    Signal<int, double> readingEmitted;
    Signal<int> statusEmitted;

    std::string sensorName{"temperature_zone_1"};

    RuntimeLightSensorNode() = default;
    ~RuntimeLightSensorNode() override = default;

    RuntimeLightSensorNode(const RuntimeLightSensorNode &) = delete;
    RuntimeLightSensorNode &operator=(const RuntimeLightSensorNode &) = delete;
    RuntimeLightSensorNode(RuntimeLightSensorNode &&) = delete;
    RuntimeLightSensorNode &operator=(RuntimeLightSensorNode &&) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return !sensorName.empty();
    }

    void emitReading(int channel, double value)
    {
        readingEmitted.emit(channel, value);
    }

    void emitStatus(int status)
    {
        statusEmitted.emit(status);
    }
};

class RuntimeLightControllerNode final : public LightObject
{
public:
    int lastChannel{-1};
    double lastValue{0.0};
    int lastStatus{-1};
    int invocationCount{0};
    int statusInvocationCount{0};
    int alternateInvocationCount{0};
    int alternateLastChannel{-1};
    double alternateLastValue{0.0};

    RuntimeLightControllerNode() = default;
    ~RuntimeLightControllerNode() override = default;

    RuntimeLightControllerNode(const RuntimeLightControllerNode &) = delete;
    RuntimeLightControllerNode &operator=(const RuntimeLightControllerNode &) = delete;
    RuntimeLightControllerNode(RuntimeLightControllerNode &&) = delete;
    RuntimeLightControllerNode &operator=(RuntimeLightControllerNode &&) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return invocationCount > 0 ||
               statusInvocationCount > 0 ||
               alternateInvocationCount > 0;
    }

    void handleReading(int channel, double value)
    {
        lastChannel = channel;
        lastValue = value;
        ++invocationCount;
    }

    void handleStatus(int status)
    {
        lastStatus = status;
        ++statusInvocationCount;
    }

    void handleAlternateReading(int channel, double value)
    {
        alternateLastChannel = channel;
        alternateLastValue = value;
        ++alternateInvocationCount;
    }
};

} // namespace job::core::tests

// =============================================================================
// Block 1: Usage / Examples
// =============================================================================

TEST_CASE("LightObject: UID, validity, and signal connection", "[core][light_object][example]")
{
    auto sensor = std::make_unique<RuntimeLightSensorNode>();
    auto controller = std::make_shared<RuntimeLightControllerNode>();

    REQUIRE(sensor);
    REQUIRE(controller);

    CHECK(sensor->uid() != 0);
    CHECK(controller->uid() != 0);
    CHECK(controller->uid() > sensor->uid());

    CHECK(sensor->isValid());
    CHECK_FALSE(controller->isValid());

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);
    REQUIRE(connection.connected());
    REQUIRE(connection.id() != 0);

    CHECK(sensor->readingEmitted.connectionCount() == 1);
    CHECK(controller->connectionCount() == 1);

    sensor->emitReading(4, 98.6);

    CHECK(controller->lastChannel == 4);
    CHECK(controller->lastValue == 98.6);
    CHECK(controller->invocationCount == 1);
    CHECK(controller->isValid());
}

TEST_CASE("LightObject: Multiple senders can connect to one receiver", "[core][light_object][example][fanin]")
{
    RuntimeLightSensorNode firstSensor;
    RuntimeLightSensorNode secondSensor;
    RuntimeLightControllerNode controller;

    auto firstConnection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(firstSensor, controller);

    auto secondConnection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(secondSensor, controller);

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);
    REQUIRE(controller.connectionCount() == 2);

    firstSensor.emitReading(1, 10.0);
    secondSensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 2);
    CHECK(controller.lastValue == 20.0);
}

// =============================================================================
// Block 1: Object / LightObject Interoperability
// =============================================================================

TEST_CASE("LightObject: Light sender can connect to Object receiver", "[core][light_object][object][interop]")
{
    RuntimeLightSensorNode sensor;
    ControllerNode controller;

    STATIC_CHECK(LightObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK(ObjectType<ControllerNode>);
    STATIC_CHECK(SignalObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK(SignalObjectType<ControllerNode>);

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);

    sensor.emitReading(7, 77.7);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 7);
    CHECK(controller.lastValue == 77.7);
}

TEST_CASE("LightObject: Object sender can connect to Light receiver", "[core][light_object][object][interop]")
{
    SensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&SensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);

    sensor.emitReading(8, 88.8);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 8);
    CHECK(controller.lastValue == 88.8);
}

TEST_CASE("LightObject: Object and LightObject connections share receiver lifetime semantics", "[core][light_object][object][interop][lifetime]")
{
    SensorNode objectSensor;
    RuntimeLightSensorNode lightSensor;

    Connection objectConnection;
    Connection lightConnection;

    {
        RuntimeLightControllerNode controller;

        objectConnection =
            connect<&SensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(objectSensor, controller);

        lightConnection =
            connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleAlternateReading>(lightSensor, controller);

        REQUIRE(objectConnection);
        REQUIRE(lightConnection);
        REQUIRE(controller.connectionCount() == 2);

        objectSensor.emitReading(1, 10.0);
        lightSensor.emitReading(2, 20.0);

        CHECK(controller.invocationCount == 1);
        CHECK(controller.alternateInvocationCount == 1);
    }

    CHECK_FALSE(objectConnection.connected());
    CHECK_FALSE(lightConnection.connected());

    CHECK(objectSensor.readingEmitted.empty());
    CHECK(lightSensor.readingEmitted.empty());
}

// =============================================================================
// Block 2: Validity / Type Invariants
// =============================================================================

TEST_CASE("LightObject: Validity is independent from construction", "[core][light_object][validity][edge_cases]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    REQUIRE(sensor.isValid());
    REQUIRE_FALSE(controller.isValid());

    sensor.sensorName.clear();

    CHECK_FALSE(sensor.isValid());

    sensor.sensorName = "temperature_zone_2";

    CHECK(sensor.isValid());

    controller.handleReading(0, 42.0);

    CHECK(controller.isValid());
}

TEST_CASE("LightObject: Concept and address-pinned invariants", "[core][light_object][concept][edge_cases]")
{
    STATIC_CHECK(LightObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK(LightObjectType<RuntimeLightControllerNode>);

    STATIC_CHECK(std::same_as<LightObject::Ptr, std::shared_ptr<LightObject>>);
    STATIC_CHECK(std::same_as<LightObject::WPtr, std::weak_ptr<LightObject>>);
    STATIC_CHECK(std::same_as<LightObject::UPtr, std::unique_ptr<LightObject>>);

    STATIC_CHECK(SignalObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK(SignalObjectType<RuntimeLightControllerNode>);

    STATIC_CHECK_FALSE(ObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(ObjectType<RuntimeLightControllerNode>);

    STATIC_CHECK_FALSE(BaseObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(BaseObjectType<RuntimeLightControllerNode>);

    STATIC_CHECK(std::default_initializable<RuntimeLightSensorNode>);
    STATIC_CHECK(std::destructible<RuntimeLightSensorNode>);

    STATIC_CHECK_FALSE(std::copy_constructible<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(std::is_copy_assignable_v<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(std::move_constructible<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(std::is_move_assignable_v<RuntimeLightSensorNode>);
}


TEST_CASE("LightObject: Static JOB factories create requested concrete type", "[core][light_object][factory]")
{
    auto uniqueSensor = LightObject::createUniq<RuntimeLightSensorNode>();
    auto sharedSensor = LightObject::createShared<RuntimeLightSensorNode>();

    STATIC_CHECK(std::same_as<decltype(uniqueSensor), std::unique_ptr<RuntimeLightSensorNode>>);
    STATIC_CHECK(std::same_as<decltype(sharedSensor), std::shared_ptr<RuntimeLightSensorNode>>);

    REQUIRE(uniqueSensor);
    REQUIRE(sharedSensor);

    CHECK(uniqueSensor->isValid());
    CHECK(sharedSensor->isValid());
    CHECK(uniqueSensor->uid() != 0);
    CHECK(sharedSensor->uid() != 0);
    CHECK(uniqueSensor->uid() != sharedSensor->uid());
    CHECK_FALSE(uniqueSensor->signalsBlocked());
    CHECK_FALSE(sharedSensor->signalsBlocked());
}

TEST_CASE("LightObject: Runtime-only hierarchy is intentionally outside BaseObject persistence", "[core][light_object][concept][persistence]")
{
    STATIC_CHECK(std::derived_from<RuntimeLightSensorNode, LightObject>);
    STATIC_CHECK(std::derived_from<RuntimeLightControllerNode, LightObject>);

    STATIC_CHECK_FALSE(std::derived_from<RuntimeLightSensorNode, BaseObject>);
    STATIC_CHECK_FALSE(std::derived_from<RuntimeLightControllerNode, BaseObject>);

    STATIC_CHECK_FALSE(BaseObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(BaseObjectType<RuntimeLightControllerNode>);
    STATIC_CHECK_FALSE(ObjectType<RuntimeLightSensorNode>);
    STATIC_CHECK_FALSE(ObjectType<RuntimeLightControllerNode>);
}

// =============================================================================
// Block 2: Signal Blocking
// =============================================================================

TEST_CASE("LightObject: Signals are unblocked by default", "[core][light_object][blocking]")
{
    RuntimeLightSensorNode sensor;

    CHECK_FALSE(sensor.signalsBlocked());
}

TEST_CASE("LightObject: blockSignals returns the previous blocked state", "[core][light_object][blocking]")
{
    RuntimeLightSensorNode sensor;

    REQUIRE_FALSE(sensor.signalsBlocked());

    CHECK_FALSE(sensor.blockSignals(true));
    CHECK(sensor.signalsBlocked());

    CHECK(sensor.blockSignals(true));
    CHECK(sensor.signalsBlocked());

    CHECK(sensor.blockSignals(false));
    CHECK_FALSE(sensor.signalsBlocked());

    CHECK_FALSE(sensor.blockSignals(false));
}

TEST_CASE("LightObject: Blocking signals suppresses signal delivery", "[core][light_object][blocking][signal]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);

    sensor.emitReading(1, 10.0);

    REQUIRE(controller.invocationCount == 1);

    CHECK_FALSE(sensor.blockSignals(true));

    sensor.emitReading(2, 20.0);
    sensor.emitReading(3, 30.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 1);

    CHECK(sensor.blockSignals(false));

    sensor.emitReading(4, 40.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 4);
}

TEST_CASE("LightObject: Blocking does not disconnect connections", "[core][light_object][blocking][connection]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);

    CHECK_FALSE(sensor.blockSignals(true));

    CHECK(connection.connected());
    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 0);

    CHECK(sensor.blockSignals(false));

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 1);
}


TEST_CASE("LightObject: Blocked SingleShot remains armed until actual delivery", "[core][light_object][blocking][single_shot]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::SingleShot);

    REQUIRE(connection);
    REQUIRE(connection.isSingleShot());

    CHECK_FALSE(sensor.blockSignals(true));

    sensor.emitReading(1, 10.0);
    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 0);
    CHECK(connection.connected());
    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    CHECK(sensor.blockSignals(false));

    sensor.emitReading(3, 30.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 3);
    CHECK(controller.lastValue == 30.0);
    CHECK_FALSE(connection.connected());
    CHECK(sensor.readingEmitted.empty());
    CHECK(controller.connectionCount() == 0);

    sensor.emitReading(4, 40.0);
    CHECK(controller.invocationCount == 1);
}

// =============================================================================
// Block 2: Connection Lifetime
// =============================================================================

TEST_CASE("LightObject: Explicit disconnect removes receiver registration", "[core][light_object][connection][disconnect]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);
    REQUIRE(controller.connectionCount() == 1);

    connection.disconnect();

    CHECK_FALSE(connection);
    CHECK(sensor.readingEmitted.empty());
    CHECK(controller.connectionCount() == 0);
}

TEST_CASE("LightObject: Receiver destruction automatically disconnects signal", "[core][light_object][lifetime][receiver]")
{
    auto sensor = std::make_unique<RuntimeLightSensorNode>();

    Connection connection;

    {
        auto controller = std::make_unique<RuntimeLightControllerNode>();

        connection =
            connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(*sensor, *controller);

        REQUIRE(connection);

        sensor->emitReading(1, 42.0);

        CHECK(controller->invocationCount == 1);
    }

    CHECK_FALSE(connection.connected());
    CHECK(sensor->readingEmitted.empty());

    REQUIRE_NOTHROW(sensor->emitReading(2, 84.0));
}

TEST_CASE("LightObject: Sender destruction before receiver is safe", "[core][light_object][lifetime][sender]")
{
    auto controller = std::make_unique<RuntimeLightControllerNode>();

    Connection connection;

    {
        auto sensor = std::make_unique<RuntimeLightSensorNode>();

        connection =
            connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(*sensor, *controller);

        REQUIRE(connection);

        sensor->emitReading(1, 42.0);

        CHECK(controller->invocationCount == 1);
    }

    CHECK_FALSE(connection.connected());
    CHECK(controller->connectionCount() == 0);

    REQUIRE_NOTHROW(connection.disconnect());
    REQUIRE_NOTHROW(controller->disconnectAll());
}

TEST_CASE("LightObject: disconnectAll removes every incoming connection", "[core][light_object][connection][disconnect_all]")
{
    RuntimeLightSensorNode firstSensor;
    RuntimeLightSensorNode secondSensor;
    RuntimeLightControllerNode controller;

    auto firstConnection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(firstSensor, controller);

    auto secondConnection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(secondSensor, controller);

    auto statusConnection =
        connect<&RuntimeLightSensorNode::statusEmitted, &RuntimeLightControllerNode::handleStatus>(firstSensor, controller);

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);
    REQUIRE(statusConnection);

    REQUIRE(controller.connectionCount() == 3);

    controller.disconnectAll();

    CHECK_FALSE(firstConnection.connected());
    CHECK_FALSE(secondConnection.connected());
    CHECK_FALSE(statusConnection.connected());

    CHECK(firstSensor.readingEmitted.empty());
    CHECK(secondSensor.readingEmitted.empty());
    CHECK(firstSensor.statusEmitted.empty());
    CHECK(controller.connectionCount() == 0);
}


TEST_CASE("LightObject: Receiver can disconnect and reconnect", "[core][light_object][connection][reconnect]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto first =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller);

    REQUIRE(first);
    REQUIRE(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);
    REQUIRE(controller.invocationCount == 1);

    controller.disconnectAll();

    CHECK_FALSE(first.connected());
    CHECK(controller.connectionCount() == 0);
    CHECK(sensor.readingEmitted.empty());

    auto second =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller);

    REQUIRE(second);
    CHECK(second.id() != first.id());
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 2);
    CHECK(controller.lastValue == 20.0);
}

TEST_CASE("LightObject: connectionCount reports only live connections", "[core][light_object][connection][count]")
{
    RuntimeLightSensorNode firstSensor;
    auto secondSensor = std::make_unique<RuntimeLightSensorNode>();
    RuntimeLightControllerNode controller;

    auto first =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            firstSensor,
            controller);

    auto second =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            *secondSensor,
            controller);

    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(controller.connectionCount() == 2);

    first.disconnect();

    CHECK_FALSE(first.connected());
    CHECK(second.connected());
    CHECK(controller.connectionCount() == 1);

    secondSensor.reset();

    CHECK_FALSE(second.connected());
    CHECK(controller.connectionCount() == 0);
}

TEST_CASE("LightObject: Registering connection prunes stale receiver handles", "[core][light_object][connection][prune]")
{
    RuntimeLightSensorNode firstSensor;
    RuntimeLightSensorNode secondSensor;
    RuntimeLightControllerNode controller;

    auto first =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            firstSensor,
            controller);

    REQUIRE(first);
    REQUIRE(controller.connectionCount() == 1);

    first.disconnect();

    REQUIRE_FALSE(first);
    REQUIRE(controller.connectionCount() == 0);

    auto second =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            secondSensor,
            controller);

    REQUIRE(second);
    CHECK(controller.connectionCount() == 1);

    secondSensor.emitReading(9, 90.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 9);
}

// =============================================================================
// Block 2: Unique / SingleShot
// =============================================================================

TEST_CASE("LightObject: Unique connection rejects duplicate receiver and slot", "[core][light_object][connection][unique]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto first =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(first);
    REQUIRE(first.isUnique());

    auto duplicate =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    CHECK_FALSE(duplicate);
    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
}

TEST_CASE("LightObject: SingleShot connection executes once", "[core][light_object][connection][single_shot]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    auto connection =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::SingleShot);

    REQUIRE(connection);
    REQUIRE(connection.isSingleShot());

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
    CHECK_FALSE(connection.connected());
    CHECK(sensor.readingEmitted.empty());
    CHECK(controller.connectionCount() == 0);

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 1);
}


TEST_CASE("LightObject: Unique and SingleShot flags compose", "[core][light_object][connection][unique][single_shot]")
{
    RuntimeLightSensorNode sensor;
    RuntimeLightControllerNode controller;

    constexpr ConnectionFlag Flags = ConnectionFlag::Unique | ConnectionFlag::SingleShot;

    auto first =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    REQUIRE(first);
    REQUIRE(first.isUnique());
    REQUIRE(first.isSingleShot());

    auto duplicate =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    CHECK_FALSE(duplicate);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
    CHECK_FALSE(first);
    CHECK(sensor.readingEmitted.empty());
    CHECK(controller.connectionCount() == 0);

    auto replacement =
        connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
            sensor,
            controller,
            Flags);

    REQUIRE(replacement);

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 2);
    CHECK(controller.lastValue == 20.0);
    CHECK_FALSE(replacement);
    CHECK(controller.connectionCount() == 0);
}

// =============================================================================
// Block 2: UID
// =============================================================================

TEST_CASE("LightObject: UID values are nonzero and unique", "[core][light_object][uid][edge_cases]")
{
    RuntimeLightSensorNode first;
    RuntimeLightSensorNode second;
    RuntimeLightControllerNode third;

    CHECK(first.uid() != 0);
    CHECK(second.uid() != 0);
    CHECK(third.uid() != 0);

    CHECK(first.uid() != second.uid());
    CHECK(first.uid() != third.uid());
    CHECK(second.uid() != third.uid());

    CHECK(second.uid() > first.uid());
    CHECK(third.uid() > second.uid());
}

TEST_CASE("LightObject: UID remains stable for object lifetime", "[core][light_object][uid][edge_cases]")
{
    RuntimeLightSensorNode sensor;

    const auto uid = sensor.uid();

    REQUIRE(uid != 0);

    sensor.emitReading(1, 10.0);
    CHECK(sensor.uid() == uid);

    sensor.disconnectAll();
    CHECK(sensor.uid() == uid);

    CHECK_FALSE(sensor.blockSignals(true));
    CHECK(sensor.uid() == uid);

    CHECK(sensor.blockSignals(false));
    CHECK(sensor.uid() == uid);
}

// =============================================================================
// Block 3: Benchmarks
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("LightObject benchmarks", "[core][light_object][benchmark]")
{
    auto sensor = std::make_unique<RuntimeLightSensorNode>();
    auto controller = std::make_unique<RuntimeLightControllerNode>();

    const auto connection = connect<&RuntimeLightSensorNode::readingEmitted,
                                    &RuntimeLightControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);

    BENCHMARK("Connected LightObject slot invocation")
    {
        sensor->emitReading(1, 42.0);
        return controller->invocationCount;
    };

    BENCHMARK("LightObject isValid - valid RuntimeLightSensorNode")
    {
        benchmarkEscape(*sensor);
        return sensor->isValid();
    };

    BENCHMARK("LightObject connectionCount - one live connection")
    {
        return controller->connectionCount();
    };

    BENCHMARK("LightObject signalsBlocked - unblocked")
    {
        return sensor->signalsBlocked();
    };

    BENCHMARK("LightObject blockSignals toggle")
    {
        const bool previous = sensor->blockSignals(!sensor->signalsBlocked());
        return previous;
    };


    BENCHMARK("LightObject::createUniq allocation and destruction")
    {
        auto object = LightObject::createUniq<RuntimeLightSensorNode>();
        benchmarkEscape(*object);
        return object->uid();
    };

    BENCHMARK("LightObject::createShared allocation and destruction")
    {
        auto object = LightObject::createShared<RuntimeLightSensorNode>();
        benchmarkEscape(*object);
        return object->uid();
    };

    BENCHMARK("LightObject unique allocation and destruction")
    {
        auto object = std::make_unique<RuntimeLightSensorNode>();
        benchmarkEscape(*object);
        return object->uid();
    };

    BENCHMARK("LightObject shared allocation and destruction")
    {
        auto object = std::make_shared<RuntimeLightSensorNode>();
        benchmarkEscape(*object);
        return object->uid();
    };

    BENCHMARK("Pure LightObject make_unique")
    {
        auto object = std::make_unique<PureLightNode>();
        benchmarkEscape(*object);
        return object->uid();
    };

    BENCHMARK("Pure LightObject make_shared")
    {
        auto object = std::make_shared<PureLightNode>();
        benchmarkEscape(*object);
        return object->uid();
    };
    BENCHMARK("String LightObject make_shared")
    {
        auto object = std::make_shared<StringLightNode>();
        benchmarkEscape(*object);
        return object->uid();
    };
}

TEST_CASE("LightObject connection flag benchmarks", "[core][light_object][benchmark][connection][flags]")
{
    BENCHMARK("LightObject Unique connect and disconnect")
    {
        RuntimeLightSensorNode sensor;
        RuntimeLightControllerNode controller;

        auto connection =
            connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
                sensor,
                controller,
                ConnectionFlag::Unique);

        connection.disconnect();

        return controller.connectionCount();
    };

    BENCHMARK("LightObject SingleShot connect and emit")
    {
        RuntimeLightSensorNode sensor;
        RuntimeLightControllerNode controller;

        auto connection =
            connect<&RuntimeLightSensorNode::readingEmitted, &RuntimeLightControllerNode::handleReading>(
                sensor,
                controller,
                ConnectionFlag::SingleShot);

        sensor.emitReading(1, 42.0);

        return controller.invocationCount + static_cast<int>(connection.connected());
    };



}

TEST_CASE("Object vs LightObject size", "[core][light_object][object][benchmark][size]")
{
    WARN("SensorNode sizeof: " << sizeof(SensorNode));
    WARN("RuntimeLightSensorNode sizeof: " << sizeof(RuntimeLightSensorNode));
    WARN("ControllerNode sizeof: " << sizeof(ControllerNode));
    WARN("RuntimeLightControllerNode sizeof: " << sizeof(RuntimeLightControllerNode));

    SUCCEED();
}

#endif