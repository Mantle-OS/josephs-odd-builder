#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <job_base_obj.h>
#include <job_light_object.h>
#include <job_obj_annotation.h>
#include <job_obj_concept.h>
#include <job_object.h>
#include <job_signal.h>

namespace job::core::tests {

template <typename T>
inline void benchmarkEscape(T &value)
{
    asm volatile("" : : "g"(&value) : "memory");
}

// =============================================================================
// Object / Signal Fixtures
// =============================================================================

class SensorNode : public Object {
public:
    SensorNode() = default;
    ~SensorNode() override = default;

    SensorNode(const SensorNode &) = delete;
    SensorNode &operator=(const SensorNode &) = delete;
    SensorNode(SensorNode &&) = delete;
    SensorNode &operator=(SensorNode &&) = delete;

    void emitReading(int channel, double value)
    {
        readingEmitted.emit(channel, value);
    }

    void emitStatus(int status)
    {
        statusEmitted.emit(status);
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return !sensorName.empty();
    }

    std::string sensorName{"temperature_zone_1"};

    Signal<int, double> readingEmitted;
    Signal<int> statusEmitted;
};

class ControllerNode : public Object {
public:
    ControllerNode() = default;
    ~ControllerNode() override = default;

    ControllerNode(const ControllerNode &) = delete;
    ControllerNode &operator=(const ControllerNode &) = delete;
    ControllerNode(ControllerNode &&) = delete;
    ControllerNode &operator=(ControllerNode &&) = delete;

    void handleReading(int channel, double value)
    {
        lastChannel = channel;
        lastValue = value;
        ++invocationCount;
    }

    void handleAlternateReading(int channel, double value)
    {
        alternateLastChannel = channel;
        alternateLastValue = value;
        ++alternateInvocationCount;
    }

    void handleStatus(int status)
    {
        lastStatus = status;
        ++statusInvocationCount;
    }

    [[nodiscard]] bool isValid() const noexcept
    {
        return lastChannel >= 0 && invocationCount > 0;
    }

    int lastChannel{-1};
    double lastValue{0.0};
    int invocationCount{0};

    int alternateLastChannel{-1};
    double alternateLastValue{0.0};
    int alternateInvocationCount{0};

    int lastStatus{-1};
    int statusInvocationCount{0};
};

class LightSensorNode : public LightObject {
public:
    LightSensorNode() = default;
    ~LightSensorNode() override = default;

    LightSensorNode(const LightSensorNode &) = delete;
    LightSensorNode &operator=(const LightSensorNode &) = delete;
    LightSensorNode(LightSensorNode &&) = delete;
    LightSensorNode &operator=(LightSensorNode &&) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return enabled;
    }

    bool enabled{true};
    Signal<int> readingEmitted;
};

static_assert(ObjectType<SensorNode>);
static_assert(ObjectType<ControllerNode>);
static_assert(LightObjectType<LightSensorNode>);

static_assert(BaseObjectType<SensorNode>);
static_assert(BaseObjectType<ControllerNode>);
static_assert(!BaseObjectType<LightSensorNode>);

static_assert(std::derived_from<SensorNode, Object>);
static_assert(std::derived_from<ControllerNode, Object>);
static_assert(std::derived_from<LightSensorNode, LightObject>);

static_assert(std::same_as<decltype(makeUniq<SensorNode>()), std::unique_ptr<SensorNode>>);
static_assert(std::same_as<decltype(makeShared<SensorNode>()), std::shared_ptr<SensorNode>>);

static_assert(std::same_as<decltype(Object::createUniq<SensorNode>()), std::unique_ptr<SensorNode>>);
static_assert(std::same_as<decltype(Object::createShared<SensorNode>()), std::shared_ptr<SensorNode>>);
static_assert(std::same_as<decltype(LightObject::createUniq<LightSensorNode>()), std::unique_ptr<LightSensorNode>>);
static_assert(std::same_as<decltype(LightObject::createShared<LightSensorNode>()), std::shared_ptr<LightSensorNode>>);

// =============================================================================
// BaseObject Serialization Fixtures
// =============================================================================

enum class DeviceMode : std::uint8_t {
    Idle = 0,
    Compute,
    Suspended
};

class SubSensorConfig : public BaseObject {
public:
    std::string sensorTag{"thermal_zone_0"};
    float sampleRateHz{100.0f};
    bool calibrateOnBoot{true};
    DeviceMode initialMode{DeviceMode::Idle};
};

class ComputeNodeConfig : public BaseObject {
public:
    std::string nodeName{"worker-node-alpha"};
    std::uint32_t threadPoolSize{16};
    double memoryBudgetGb{32.5};
    std::vector<float> scalingFactors{1.0f, 0.5f, 0.25f};

    SubSensorConfig primarySensor;
    std::vector<std::shared_ptr<SubSensorConfig>> auxiliarySensors;
};

class PrimitiveConfig : public BaseObject {
public:
    DeviceMode mode{DeviceMode::Idle};
    std::byte rawByte{std::byte{0}};
    wchar_t wideChar{L'A'};
    char8_t utf8Char{u8'B'};
    char16_t utf16Char{u'C'};
    char32_t utf32Char{U'D'};
};

// BaseObject runtime/error state must not become part of serialized payload state.
class ErrorStateConfig : public BaseObject {
public:
    std::string value{"payload"};
};

class ContainerConfig : public BaseObject {
public:
    std::vector<std::int32_t> values;
    std::vector<std::string> names;
};

class BinaryMapConfig : public BaseObject {
public:
    std::map<std::uint32_t, std::string> entries;
};

class OptionalConfig : public BaseObject {
public:
    std::optional<std::int32_t> optionalValue;
    std::optional<std::string> optionalName;
    std::optional<SubSensorConfig> optionalSensor;
    std::optional<std::shared_ptr<SubSensorConfig>> optionalSharedSensor;
};

class PointerConfig : public BaseObject {
public:
    std::shared_ptr<SubSensorConfig> sharedSensor;
    std::unique_ptr<SubSensorConfig> uniqueSensor;
};

class FixedContainerConfig : public BaseObject {
public:
    std::array<std::int32_t, 4> values{1, 2, 3, 4};
};

class SetConfig : public BaseObject {
public:
    std::set<std::int32_t> values;
    std::set<std::string> names;
};

class MapConfig : public BaseObject {
public:
    std::map<std::uint32_t, std::string> entries;
    std::map<std::string, SubSensorConfig> sensors;
};

class NestedContainerConfig : public BaseObject {
public:
    std::vector<std::optional<SubSensorConfig>> optionalSensors;
    std::vector<std::shared_ptr<SubSensorConfig>> sharedSensors;
    std::map<std::string, std::shared_ptr<SubSensorConfig>> namedSensors;
};

class AnnotationConfig : public BaseObject {
public:
    std::string serializedValue{"serialized"};

    [[=NoSerialize{}]]
        std::string transientValue{"transient"};

    [[=NoReset{}]]
        std::string noResetValue{"no-reset"};

    [[=NoSerialize{}]]
        [[=NoReset{}]]
        std::string runtimeValue{"runtime"};
};

static_assert(BaseObjectType<SubSensorConfig>);
static_assert(BaseObjectType<ComputeNodeConfig>);
static_assert(BaseObjectType<PrimitiveConfig>);
static_assert(BaseObjectType<ErrorStateConfig>);
static_assert(BaseObjectType<ContainerConfig>);
static_assert(BaseObjectType<BinaryMapConfig>);
static_assert(BaseObjectType<OptionalConfig>);
static_assert(BaseObjectType<PointerConfig>);
static_assert(BaseObjectType<FixedContainerConfig>);
static_assert(BaseObjectType<SetConfig>);
static_assert(BaseObjectType<MapConfig>);
static_assert(BaseObjectType<NestedContainerConfig>);
static_assert(BaseObjectType<AnnotationConfig>);

static_assert(!BaseObjectType<int>);
static_assert(!BaseObjectType<std::string>);

// =============================================================================
// Pointer Concept Coverage
// =============================================================================

static_assert(SmartPointer<std::shared_ptr<SubSensorConfig>>);
static_assert(SmartPointer<std::unique_ptr<SubSensorConfig>>);
static_assert(!SmartPointer<int>);

static_assert(SharedPointer<std::shared_ptr<SubSensorConfig>>);
static_assert(!SharedPointer<std::unique_ptr<SubSensorConfig>>);

static_assert(UniquePointer<std::unique_ptr<SubSensorConfig>>);
static_assert(!UniquePointer<std::shared_ptr<SubSensorConfig>>);

static_assert(WeakPointer<std::weak_ptr<SubSensorConfig>>);
static_assert(!WeakPointer<std::shared_ptr<SubSensorConfig>>);

static_assert(OwningSmartPointer<std::shared_ptr<SubSensorConfig>>);
static_assert(OwningSmartPointer<std::unique_ptr<SubSensorConfig>>);
static_assert(!OwningSmartPointer<std::weak_ptr<SubSensorConfig>>);

static_assert(UnsupportedPersistentPointer<SubSensorConfig *>);
static_assert(UnsupportedPersistentPointer<std::weak_ptr<SubSensorConfig>>);
static_assert(!UnsupportedPersistentPointer<std::shared_ptr<SubSensorConfig>>);

// =============================================================================
// Optional / Container Concept Coverage
// =============================================================================

static_assert(OptionalType<std::optional<int>>);
static_assert(!OptionalType<int>);

static_assert(ReflectableContainer<std::vector<int>>);
static_assert(ReflectableContainer<std::array<int, 4>>);
static_assert(ReflectableContainer<std::set<int>>);
static_assert(ReflectableContainer<std::map<int, int>>);
static_assert(!ReflectableContainer<std::string>);

static_assert(MapContainer<std::map<int, int>>);
static_assert(!MapContainer<std::vector<int>>);

static_assert(SetContainer<std::set<int>>);
static_assert(!SetContainer<std::vector<int>>);
static_assert(!SetContainer<std::map<int, int>>);

static_assert(StdArrayType<std::array<int, 4>>);
static_assert(FixedSequenceContainer<std::array<int, 4>>);
static_assert(stdArraySizeV<std::array<int, 4>> == 4);

static_assert(PushBackSequenceContainer<std::vector<int>>);
static_assert(!PushBackSequenceContainer<std::array<int, 4>>);
static_assert(!PushBackSequenceContainer<std::set<int>>);
static_assert(!PushBackSequenceContainer<std::map<int, int>>);

static_assert(InsertSequenceContainer<std::set<int>>);
static_assert(!InsertSequenceContainer<std::vector<int>>);
static_assert(!InsertSequenceContainer<std::array<int, 4>>);
static_assert(!InsertSequenceContainer<std::map<int, int>>);

static_assert(PersistentContainer<std::vector<int>>);
static_assert(PersistentContainer<std::array<int, 4>>);
static_assert(PersistentContainer<std::set<int>>);
static_assert(PersistentContainer<std::map<int, int>>);

// =============================================================================
// Test Utilities
// =============================================================================

inline std::filesystem::path tempFilePath(const std::string &name)
{
    return std::filesystem::temp_directory_path() / name;
}

// =============================================================================
// Standalone Signal Fixtures
// =============================================================================

struct TestReceiver {
    int receivedInt{0};
    std::string receivedStr;

    void handleEvent(int val, const std::string &str)
    {
        receivedInt = val;
        receivedStr = str;
    }

    void handleIntOnly(int val)
    {
        receivedInt += val;
    }
};

} // namespace job::core::tests
