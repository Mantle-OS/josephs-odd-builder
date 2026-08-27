#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <concepts>
#include <memory>
#include <type_traits>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

// =============================================================================
// Block 1: Usage / Examples
// =============================================================================

TEST_CASE("Object: Factory creation, UID, validity, and reflected signal connection", "[core][object][example]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeShared<ControllerNode>();

    REQUIRE(sensor);
    REQUIRE(controller);

    CHECK(sensor->uid() != 0);
    CHECK(controller->uid() != 0);
    CHECK(controller->uid() > sensor->uid());

    CHECK(sensor->isValid());
    CHECK_FALSE(controller->isValid());

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);
    REQUIRE(connection.connected());
    REQUIRE(connection.id() != 0);

    REQUIRE(sensor->readingEmitted.connectionCount() == 1);
    REQUIRE(controller->connectionCount() == 1);
    REQUIRE_FALSE(sensor->readingEmitted.empty());

    sensor->emitReading(4, 98.6);

    CHECK(controller->lastChannel == 4);
    CHECK(controller->lastValue == 98.6);
    CHECK(controller->invocationCount == 1);
    CHECK(controller->isValid());

    sensor->emitReading(5, 101.3);

    CHECK(controller->lastChannel == 5);
    CHECK(controller->lastValue == 101.3);
    CHECK(controller->invocationCount == 2);
    CHECK(controller->isValid());
}

TEST_CASE("Object: Multiple senders can connect to one receiver", "[core][object][example][fanin]")
{
    auto firstSensor = makeUniq<SensorNode>();
    auto secondSensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    REQUIRE(firstSensor->isValid());
    REQUIRE(secondSensor->isValid());
    REQUIRE_FALSE(controller->isValid());

    auto firstConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*firstSensor, *controller);

    auto secondConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*secondSensor, *controller);

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);

    REQUIRE(firstSensor->readingEmitted.connectionCount() == 1);
    REQUIRE(secondSensor->readingEmitted.connectionCount() == 1);
    REQUIRE(controller->connectionCount() == 2);

    firstSensor->emitReading(1, 10.0);

    CHECK(controller->lastChannel == 1);
    CHECK(controller->lastValue == 10.0);
    CHECK(controller->invocationCount == 1);
    CHECK(controller->isValid());

    secondSensor->emitReading(2, 20.0);

    CHECK(controller->lastChannel == 2);
    CHECK(controller->lastValue == 20.0);
    CHECK(controller->invocationCount == 2);
    CHECK(controller->isValid());
}

// =============================================================================
// Block 2: Validity
// =============================================================================

TEST_CASE("Object: Validity is independent from construction", "[core][object][validity][edge_cases]")
{
    SensorNode sensor;
    ControllerNode controller;

    REQUIRE(sensor.isValid());
    REQUIRE_FALSE(controller.isValid());

    sensor.sensorName.clear();

    CHECK_FALSE(sensor.isValid());

    sensor.sensorName = "temperature_zone_2";

    CHECK(sensor.isValid());

    controller.handleReading(0, 42.0);

    CHECK(controller.isValid());
}

// =============================================================================
// Block 2: Signal Blocking
// =============================================================================

TEST_CASE("Object: Signals are unblocked by default", "[core][object][blocking]")
{
    SensorNode sensor;

    CHECK_FALSE(sensor.signalsBlocked());
}

TEST_CASE("Object: blockSignals returns the previous blocked state", "[core][object][blocking]")
{
    SensorNode sensor;

    REQUIRE_FALSE(sensor.signalsBlocked());

    const bool firstPrevious = sensor.blockSignals(true);

    CHECK_FALSE(firstPrevious);
    CHECK(sensor.signalsBlocked());

    const bool secondPrevious = sensor.blockSignals(true);

    CHECK(secondPrevious);
    CHECK(sensor.signalsBlocked());

    const bool thirdPrevious = sensor.blockSignals(false);

    CHECK(thirdPrevious);
    CHECK_FALSE(sensor.signalsBlocked());

    const bool fourthPrevious = sensor.blockSignals(false);

    CHECK_FALSE(fourthPrevious);
    CHECK_FALSE(sensor.signalsBlocked());
}

TEST_CASE("Object: Blocking signals suppresses reflected signal delivery", "[core][object][blocking][signal]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);
    REQUIRE_FALSE(sensor.signalsBlocked());

    sensor.emitReading(1, 10.0);

    REQUIRE(controller.invocationCount == 1);

    CHECK_FALSE(sensor.blockSignals(true));
    REQUIRE(sensor.signalsBlocked());

    sensor.emitReading(2, 20.0);
    sensor.emitReading(3, 30.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 1);
    CHECK(controller.lastValue == 10.0);

    CHECK(sensor.blockSignals(false));
    REQUIRE_FALSE(sensor.signalsBlocked());

    sensor.emitReading(4, 40.0);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.lastChannel == 4);
    CHECK(controller.lastValue == 40.0);
}

TEST_CASE("Object: Blocking applies to all signals owned by the Object", "[core][object][blocking][multiple_signals]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto readingConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    auto statusConnection =
        connect<&SensorNode::statusEmitted, &ControllerNode::handleStatus>(sensor, controller);

    REQUIRE(readingConnection);
    REQUIRE(statusConnection);

    sensor.emitReading(1, 10.0);
    sensor.emitStatus(100);

    REQUIRE(controller.invocationCount == 1);
    REQUIRE(controller.statusInvocationCount == 1);

    CHECK_FALSE(sensor.blockSignals(true));

    sensor.emitReading(2, 20.0);
    sensor.emitStatus(200);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.statusInvocationCount == 1);
    CHECK(controller.lastChannel == 1);
    CHECK(controller.lastStatus == 100);

    CHECK(sensor.blockSignals(false));

    sensor.emitReading(3, 30.0);
    sensor.emitStatus(300);

    CHECK(controller.invocationCount == 2);
    CHECK(controller.statusInvocationCount == 2);
    CHECK(controller.lastChannel == 3);
    CHECK(controller.lastStatus == 300);
}

TEST_CASE("Object: Blocking signals does not disconnect connections", "[core][object][blocking][connection]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(sensor, controller);

    REQUIRE(connection);
    REQUIRE(sensor.readingEmitted.connectionCount() == 1);
    REQUIRE(controller.connectionCount() == 1);

    CHECK_FALSE(sensor.blockSignals(true));

    CHECK(connection.connected());
    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 0);

    CHECK(sensor.blockSignals(false));

    CHECK(connection.connected());

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 1);
}

TEST_CASE("Object: Blocked SingleShot connection remains armed until actual delivery", "[core][object][blocking][single_shot]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
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

    CHECK(sensor.blockSignals(false));

    sensor.emitReading(3, 30.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 3);
    CHECK(controller.lastValue == 30.0);

    CHECK_FALSE(connection.connected());
    CHECK(sensor.readingEmitted.empty());

    sensor.emitReading(4, 40.0);

    CHECK(controller.invocationCount == 1);
}

// =============================================================================
// Block 2: Connection Handles & Object-Level Disconnect
// =============================================================================

TEST_CASE("Object: Reflected connect returns a usable Connection handle", "[core][object][connection]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);
    REQUIRE(connection.connected());
    REQUIRE(connection.id() != 0);

    CHECK(sensor->readingEmitted.connectionCount() == 1);
    CHECK(controller->connectionCount() == 1);

    connection.disconnect();

    CHECK_FALSE(connection);
    CHECK_FALSE(connection.connected());

    CHECK(sensor->readingEmitted.empty());
    CHECK(sensor->readingEmitted.connectionCount() == 0);
    CHECK(controller->connectionCount() == 0);
}

TEST_CASE("Object: Receiver destruction automatically disconnects signal", "[core][object][lifetime][receiver]")
{
    auto sensor = makeUniq<SensorNode>();

    REQUIRE(sensor->readingEmitted.empty());

    Connection connection;

    {
        auto controller = makeUniq<ControllerNode>();

        connection =
            connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

        REQUIRE(connection);
        REQUIRE(connection.connected());

        REQUIRE(sensor->readingEmitted.connectionCount() == 1);
        REQUIRE(controller->connectionCount() == 1);

        sensor->emitReading(1, 42.0);

        CHECK(controller->invocationCount == 1);
        CHECK(controller->lastChannel == 1);
        CHECK(controller->lastValue == 42.0);
        CHECK(controller->isValid());
    }

    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);

    CHECK(sensor->readingEmitted.connectionCount() == 0);
    CHECK(sensor->readingEmitted.empty());

    REQUIRE_NOTHROW(sensor->emitReading(2, 84.0));
}

TEST_CASE("Object: Sender destruction before receiver is safe", "[core][object][lifetime][sender]")
{
    auto controller = makeUniq<ControllerNode>();

    Connection connection;

    {
        auto sensor = makeUniq<SensorNode>();

        connection =
            connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

        REQUIRE(connection);
        REQUIRE(connection.connected());

        REQUIRE(sensor->readingEmitted.connectionCount() == 1);
        REQUIRE(controller->connectionCount() == 1);

        sensor->emitReading(1, 42.0);

        CHECK(controller->invocationCount == 1);
        CHECK(controller->isValid());
    }

    CHECK_FALSE(connection.connected());
    CHECK_FALSE(connection);

    CHECK(controller->connectionCount() == 0);

    REQUIRE_NOTHROW(connection.disconnect());
    REQUIRE_NOTHROW(controller->disconnectAll());

    CHECK(controller->connectionCount() == 0);
}

TEST_CASE("Object: Connection handle can outlive both sender and receiver", "[core][object][connection][lifetime]")
{
    Connection connection;

    {
        auto sensor = makeUniq<SensorNode>();
        auto controller = makeUniq<ControllerNode>();

        connection =
            connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

        REQUIRE(connection);
        REQUIRE(connection.connected());

        sensor->emitReading(1, 42.0);

        CHECK(controller->invocationCount == 1);
    }

    CHECK_FALSE(connection);
    CHECK_FALSE(connection.connected());

    REQUIRE_NOTHROW(connection.disconnect());
    REQUIRE_NOTHROW(connection.disconnect());
}

TEST_CASE("Object: Explicit Connection disconnect removes receiver registration", "[core][object][connection][disconnect]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);

    REQUIRE(sensor->readingEmitted.connectionCount() == 1);
    REQUIRE(controller->connectionCount() == 1);

    connection.disconnect();

    CHECK_FALSE(connection);

    CHECK(sensor->readingEmitted.connectionCount() == 0);
    CHECK(sensor->readingEmitted.empty());
    CHECK(controller->connectionCount() == 0);

    sensor->emitReading(1, 10.0);

    CHECK(controller->invocationCount == 0);
    CHECK_FALSE(controller->isValid());
}

TEST_CASE("Object: disconnectAll removes every registered incoming connection", "[core][object][connection][disconnect_all]")
{
    auto firstSensor = makeUniq<SensorNode>();
    auto secondSensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    auto firstConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*firstSensor, *controller);

    auto secondConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*secondSensor, *controller);

    auto statusConnection =
        connect<&SensorNode::statusEmitted, &ControllerNode::handleStatus>(*firstSensor, *controller);

    REQUIRE(firstConnection);
    REQUIRE(secondConnection);
    REQUIRE(statusConnection);

    REQUIRE(firstSensor->readingEmitted.connectionCount() == 1);
    REQUIRE(secondSensor->readingEmitted.connectionCount() == 1);
    REQUIRE(firstSensor->statusEmitted.connectionCount() == 1);
    REQUIRE(controller->connectionCount() == 3);

    controller->disconnectAll();

    CHECK_FALSE(firstConnection.connected());
    CHECK_FALSE(secondConnection.connected());
    CHECK_FALSE(statusConnection.connected());

    CHECK(firstSensor->readingEmitted.empty());
    CHECK(secondSensor->readingEmitted.empty());
    CHECK(firstSensor->statusEmitted.empty());
    CHECK(controller->connectionCount() == 0);

    firstSensor->emitReading(1, 10.0);
    secondSensor->emitReading(2, 20.0);
    firstSensor->emitStatus(100);

    CHECK(controller->invocationCount == 0);
    CHECK(controller->statusInvocationCount == 0);
    CHECK_FALSE(controller->isValid());
}

TEST_CASE("Object: Repeated disconnectAll is safe", "[core][object][connection][disconnect_all][edge_cases]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);
    REQUIRE(sensor->readingEmitted.connectionCount() == 1);
    REQUIRE(controller->connectionCount() == 1);

    controller->disconnectAll();

    CHECK_FALSE(connection.connected());
    CHECK(sensor->readingEmitted.empty());
    CHECK(controller->connectionCount() == 0);

    REQUIRE_NOTHROW(controller->disconnectAll());
    REQUIRE_NOTHROW(controller->disconnectAll());

    CHECK(sensor->readingEmitted.empty());
    CHECK(controller->connectionCount() == 0);
}

TEST_CASE("Object: Receiver can be disconnected and connected again", "[core][object][connection][reconnect]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    REQUIRE_FALSE(controller->isValid());

    auto firstConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(firstConnection);
    REQUIRE(controller->connectionCount() == 1);

    sensor->emitReading(1, 10.0);

    REQUIRE(controller->invocationCount == 1);
    REQUIRE(controller->isValid());

    controller->disconnectAll();

    CHECK_FALSE(firstConnection.connected());
    CHECK(controller->connectionCount() == 0);

    sensor->emitReading(2, 20.0);

    CHECK(controller->invocationCount == 1);
    CHECK(controller->isValid());

    auto secondConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(secondConnection);
    REQUIRE(secondConnection.id() != firstConnection.id());
    REQUIRE(controller->connectionCount() == 1);

    sensor->emitReading(3, 30.0);

    CHECK(controller->invocationCount == 2);
    CHECK(controller->lastChannel == 3);
    CHECK(controller->lastValue == 30.0);
    CHECK(controller->isValid());
}

TEST_CASE("Object: Receiver connectionCount reports only live connections", "[core][object][connection][count]")
{
    auto firstSensor = makeUniq<SensorNode>();
    auto secondSensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    auto firstConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*firstSensor, *controller);

    auto secondConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*secondSensor, *controller);

    REQUIRE(controller->connectionCount() == 2);

    firstConnection.disconnect();

    CHECK_FALSE(firstConnection.connected());
    CHECK(secondConnection.connected());
    CHECK(controller->connectionCount() == 1);

    secondSensor.reset();

    CHECK_FALSE(secondConnection.connected());
    CHECK(controller->connectionCount() == 0);
}

TEST_CASE("Object: Registering a new connection prunes stale receiver handles", "[core][object][connection][prune]")
{
    SensorNode firstSensor;
    SensorNode secondSensor;
    ControllerNode controller;

    auto firstConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(firstSensor, controller);

    REQUIRE(firstConnection);
    REQUIRE(controller.connectionCount() == 1);

    firstConnection.disconnect();

    REQUIRE_FALSE(firstConnection);
    REQUIRE(controller.connectionCount() == 0);

    auto secondConnection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(secondSensor, controller);

    REQUIRE(secondConnection);
    CHECK(controller.connectionCount() == 1);

    secondSensor.emitReading(9, 90.0);

    CHECK(controller.invocationCount == 1);
}

// =============================================================================
// Block 2: Reflected Unique Connections
// =============================================================================

TEST_CASE("Object: Reflected Unique connection rejects the same sender receiver and slot", "[core][object][connection][unique]")
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

    auto duplicate =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    CHECK_FALSE(duplicate);
    CHECK(duplicate.id() == 0);

    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
}

TEST_CASE("Object: Reflected Unique connection permits a different receiver", "[core][object][connection][unique]")
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

    CHECK(sensor.readingEmitted.connectionCount() == 2);
    CHECK(firstController.connectionCount() == 1);
    CHECK(secondController.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(firstController.invocationCount == 1);
    CHECK(secondController.invocationCount == 1);
}

TEST_CASE("Object: Reflected Unique connection permits a different slot", "[core][object][connection][unique]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto first =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    auto second =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleAlternateReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(first);
    REQUIRE(second);

    CHECK(sensor.readingEmitted.connectionCount() == 2);
    CHECK(controller.connectionCount() == 2);

    sensor.emitReading(4, 44.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.alternateInvocationCount == 1);

    CHECK(controller.lastChannel == 4);
    CHECK(controller.alternateLastChannel == 4);
    CHECK(controller.lastValue == 44.0);
    CHECK(controller.alternateLastValue == 44.0);
}

TEST_CASE("Object: Reflected Unique connection can be recreated after disconnect", "[core][object][connection][unique][disconnect]")
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
    REQUIRE(controller.connectionCount() == 0);

    auto second =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::Unique);

    REQUIRE(second);

    CHECK(second.id() != first.id());
    CHECK(second.isUnique());
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(5, 55.0);

    CHECK(controller.invocationCount == 1);
}

// =============================================================================
// Block 2: Reflected SingleShot Connections
// =============================================================================

TEST_CASE("Object: Reflected SingleShot connection executes once", "[core][object][connection][single_shot]")
{
    SensorNode sensor;
    ControllerNode controller;

    auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
            sensor,
            controller,
            ConnectionFlag::SingleShot);

    REQUIRE(connection);
    REQUIRE(connection.isSingleShot());

    CHECK(sensor.readingEmitted.connectionCount() == 1);
    CHECK(controller.connectionCount() == 1);

    sensor.emitReading(1, 10.0);

    CHECK(controller.invocationCount == 1);
    CHECK(controller.lastChannel == 1);
    CHECK(controller.lastValue == 10.0);

    CHECK_FALSE(connection.connected());
    CHECK(sensor.readingEmitted.empty());
    CHECK(sensor.readingEmitted.connectionCount() == 0);
    CHECK(controller.connectionCount() == 0);

    sensor.emitReading(2, 20.0);

    CHECK(controller.invocationCount == 1);
}

TEST_CASE("Object: Reflected Unique and SingleShot flags compose", "[core][object][connection][unique][single_shot]")
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
    CHECK(controller.connectionCount() == 0);

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
    CHECK(controller.connectionCount() == 0);
}

// =============================================================================
// Block 2: UID & Type Invariants
// =============================================================================

TEST_CASE("Object: UID values are nonzero and unique", "[core][object][uid][edge_cases]")
{
    auto first = makeUniq<SensorNode>();
    auto second = makeUniq<SensorNode>();
    auto third = makeUniq<ControllerNode>();

    CHECK(first->uid() != 0);
    CHECK(second->uid() != 0);
    CHECK(third->uid() != 0);

    CHECK(first->uid() != second->uid());
    CHECK(first->uid() != third->uid());
    CHECK(second->uid() != third->uid());

    CHECK(second->uid() > first->uid());
    CHECK(third->uid() > second->uid());
}

TEST_CASE("Object: UID remains stable for the lifetime of an object", "[core][object][uid][edge_cases]")
{
    auto sensor = makeUniq<SensorNode>();

    const auto uid = sensor->uid();

    REQUIRE(uid != 0);

    sensor->emitReading(1, 10.0);
    CHECK(sensor->uid() == uid);

    sensor->disconnectAll();
    CHECK(sensor->uid() == uid);

    CHECK_FALSE(sensor->blockSignals(true));
    CHECK(sensor->uid() == uid);

    CHECK(sensor->blockSignals(false));
    CHECK(sensor->uid() == uid);

    sensor->sensorName.clear();

    CHECK_FALSE(sensor->isValid());
    CHECK(sensor->uid() == uid);
}

TEST_CASE("Object: Concept and address-pinned invariants", "[core][object][concept][edge_cases]")
{
    STATIC_CHECK(ObjectType<SensorNode>);
    STATIC_CHECK(ObjectType<ControllerNode>);

    STATIC_CHECK(BaseObjectType<SensorNode>);
    STATIC_CHECK(BaseObjectType<ControllerNode>);

    STATIC_CHECK(std::default_initializable<SensorNode>);
    STATIC_CHECK(std::destructible<SensorNode>);

    STATIC_CHECK_FALSE(std::copy_constructible<SensorNode>);
    STATIC_CHECK_FALSE(std::is_copy_assignable_v<SensorNode>);
    STATIC_CHECK_FALSE(std::move_constructible<SensorNode>);
    STATIC_CHECK_FALSE(std::is_move_assignable_v<SensorNode>);

    STATIC_CHECK(std::default_initializable<ControllerNode>);
    STATIC_CHECK(std::destructible<ControllerNode>);

    STATIC_CHECK_FALSE(std::copy_constructible<ControllerNode>);
    STATIC_CHECK_FALSE(std::is_copy_assignable_v<ControllerNode>);
    STATIC_CHECK_FALSE(std::move_constructible<ControllerNode>);
    STATIC_CHECK_FALSE(std::is_move_assignable_v<ControllerNode>);
}

TEST_CASE("Object: Factory helpers create the requested concrete type", "[core][object][factory][edge_cases]")
{
    auto uniqueSensor = makeUniq<SensorNode>();
    auto sharedSensor = makeShared<SensorNode>();

    STATIC_CHECK(std::same_as<decltype(uniqueSensor), std::unique_ptr<SensorNode>>);
    STATIC_CHECK(std::same_as<decltype(sharedSensor), std::shared_ptr<SensorNode>>);

    REQUIRE(uniqueSensor);
    REQUIRE(sharedSensor);

    CHECK(uniqueSensor->sensorName == "temperature_zone_1");
    CHECK(sharedSensor->sensorName == "temperature_zone_1");

    CHECK(uniqueSensor->isValid());
    CHECK(sharedSensor->isValid());

    CHECK_FALSE(uniqueSensor->signalsBlocked());
    CHECK_FALSE(sharedSensor->signalsBlocked());
}

// =============================================================================
// Block 3: Benchmarks
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Object benchmarks", "[core][object][benchmark]")
{
    auto sensor = makeUniq<SensorNode>();
    auto controller = makeUniq<ControllerNode>();

    const auto connection =
        connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(*sensor, *controller);

    REQUIRE(connection);

    BENCHMARK("Connected Object slot invocation")
    {
        sensor->emitReading(1, 42.0);
        return controller->invocationCount;
    };

    BENCHMARK("Object isValid - valid SensorNode")
    {
        benchmarkEscape(*sensor);
        return sensor->isValid();
    };

    SensorNode invalidSensor;
    invalidSensor.sensorName.clear();

    BENCHMARK("Object isValid - invalid SensorNode")
    {
        benchmarkEscape(invalidSensor);
        return invalidSensor.isValid();
    };

    BENCHMARK("Object connectionCount - one live connection")
    {
        return controller->connectionCount();
    };

    BENCHMARK("Object signalsBlocked - unblocked")
    {
        return sensor->signalsBlocked();
    };

    BENCHMARK("Object blockSignals toggle")
    {
        const bool previous = sensor->blockSignals(!sensor->signalsBlocked());
        return previous;
    };

    BENCHMARK("Object makeUniq allocation and destruction")
    {
        auto object = makeUniq<SensorNode>();
        return object->uid();
    };

    BENCHMARK("Object makeShared allocation and destruction")
    {
        auto object = makeShared<SensorNode>();
        return object->uid();
    };
}

TEST_CASE("Object connection flag benchmarks", "[core][object][benchmark][connection][flags]")
{
    BENCHMARK("Object reflected Unique connect and disconnect")
    {
        SensorNode sensor;
        ControllerNode controller;

        auto connection =
            connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
                sensor,
                controller,
                ConnectionFlag::Unique);

        connection.disconnect();

        return controller.connectionCount();
    };

    BENCHMARK("Object reflected SingleShot connect and emit")
    {
        SensorNode sensor;
        ControllerNode controller;

        auto connection =
            connect<&SensorNode::readingEmitted, &ControllerNode::handleReading>(
                sensor,
                controller,
                ConnectionFlag::SingleShot);

        sensor.emitReading(1, 42.0);

        return controller.invocationCount + static_cast<int>(connection.connected());
    };
}

#endif