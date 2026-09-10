#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <job_base_obj.h>
#include <job_obj_concept.h>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

// =============================================================================
// Test Helpers
// =============================================================================

template <BaseObjectType T>
static std::string serializeJobJson(const T &source)
{
    std::string json;
    REQUIRE(source.toJobJson(json));
    REQUIRE_FALSE(json.empty());
    return json;
}

template <BaseObjectType T>
static std::string serializeJobYaml(const T &source)
{
    std::string yaml;
    REQUIRE(source.toJobYaml(yaml));
    REQUIRE_FALSE(yaml.empty());
    return yaml;
}

template <BaseObjectType T>
static std::vector<std::uint8_t> serializeBinary(const T &source)
{
    std::vector<std::uint8_t> buffer;
    REQUIRE(source.toBinary(buffer));
    REQUIRE_FALSE(buffer.empty());
    return buffer;
}

template <BaseObjectType T>
static void roundTripJobJson(const T &source, T &restored)
{
    const std::string json = serializeJobJson(source);
    REQUIRE(restored.fromJobJson(json));
}

template <BaseObjectType T>
static void roundTripJobYaml(const T &source, T &restored)
{
    const std::string yaml = serializeJobYaml(source);
    REQUIRE(restored.fromJobYaml(yaml));
}

template <BaseObjectType T>
static void roundTripBinary(const T &source, T &restored)
{
    const std::vector<std::uint8_t> buffer = serializeBinary(source);
    REQUIRE(restored.fromBinary(std::span<const std::uint8_t>{buffer}));
}

// =============================================================================
// Block 1: Usage / Examples
// =============================================================================

TEST_CASE("BaseObject: JobJson serialization roundtrip with nested objects and smart pointers",
          "[core][base_obj][job_json][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "edge-inference-01";
    config.threadPoolSize = 32;
    config.memoryBudgetGb = 64.0;
    config.scalingFactors = {1.25f, 2.5f, 5.0f};

    config.primarySensor.sensorTag = "core_temp";
    config.primarySensor.sampleRateHz = 250.0f;
    config.primarySensor.calibrateOnBoot = false;
    config.primarySensor.initialMode = DeviceMode::Compute;

    auto aux1 = std::make_shared<SubSensorConfig>();
    aux1->sensorTag = "ambient_0";
    aux1->sampleRateHz = 10.0f;
    aux1->initialMode = DeviceMode::Idle;

    auto aux2 = std::make_shared<SubSensorConfig>();
    aux2->sensorTag = "vram_hotspot";
    aux2->sampleRateHz = 500.0f;
    aux2->initialMode = DeviceMode::Compute;

    config.auxiliarySensors.push_back(aux1);
    config.auxiliarySensors.push_back(aux2);

    const std::string json = serializeJobJson(config);

    CHECK(json.find("\"nodeName\"") != std::string::npos);
    CHECK(json.find("edge-inference-01") != std::string::npos);
    CHECK(json.find("\"auxiliarySensors\"") != std::string::npos);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJobJson(json));

    CHECK(restored.nodeName == "edge-inference-01");
    CHECK(restored.threadPoolSize == 32);
    CHECK(restored.memoryBudgetGb == 64.0);
    CHECK(restored.scalingFactors == std::vector<float>{1.25f, 2.5f, 5.0f});
    CHECK(restored.primarySensor.sensorTag == "core_temp");
    CHECK(restored.primarySensor.sampleRateHz == 250.0f);
    CHECK_FALSE(restored.primarySensor.calibrateOnBoot);
    CHECK(restored.primarySensor.initialMode == DeviceMode::Compute);

    REQUIRE(restored.auxiliarySensors.size() == 2);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    REQUIRE(restored.auxiliarySensors[1] != nullptr);

    CHECK(restored.auxiliarySensors[0]->sensorTag == "ambient_0");
    CHECK(restored.auxiliarySensors[0]->sampleRateHz == 10.0f);
    CHECK(restored.auxiliarySensors[1]->sensorTag == "vram_hotspot");
    CHECK(restored.auxiliarySensors[1]->sampleRateHz == 500.0f);
}

TEST_CASE("BaseObject: JobYaml serialization roundtrip with nested objects and smart pointers",
          "[core][base_obj][job_yaml][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "cluster-head";
    config.threadPoolSize = 48;
    config.memoryBudgetGb = 96.0;
    config.scalingFactors = {1.0f, 0.5f, 0.25f};

    config.primarySensor.sensorTag = "inlet_flow";
    config.primarySensor.sampleRateHz = 250.0f;
    config.primarySensor.calibrateOnBoot = false;
    config.primarySensor.initialMode = DeviceMode::Suspended;

    auto aux = std::make_shared<SubSensorConfig>();
    aux->sensorTag = "ambient";
    aux->sampleRateHz = 25.0f;
    config.auxiliarySensors.push_back(aux);

    const std::string yaml = serializeJobYaml(config);

    CHECK(yaml.find("nodeName:") != std::string::npos);
    CHECK(yaml.find("cluster-head") != std::string::npos);
    CHECK(yaml.find("auxiliarySensors:") != std::string::npos);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJobYaml(yaml));

    CHECK(restored.nodeName == "cluster-head");
    CHECK(restored.threadPoolSize == 48);
    CHECK(restored.memoryBudgetGb == 96.0);
    CHECK(restored.scalingFactors == config.scalingFactors);
    CHECK(restored.primarySensor.sensorTag == "inlet_flow");
    CHECK(restored.primarySensor.sampleRateHz == 250.0f);
    CHECK_FALSE(restored.primarySensor.calibrateOnBoot);
    CHECK(restored.primarySensor.initialMode == DeviceMode::Suspended);

    REQUIRE(restored.auxiliarySensors.size() == 1);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "ambient");
}

TEST_CASE("BaseObject: Binary serialization roundtrip",
          "[core][base_obj][binary][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "hpc-worker-99";
    config.threadPoolSize = 128;
    config.scalingFactors = {0.1f, 0.2f, 0.3f, 0.4f};
    config.primarySensor.sensorTag = "die_top";
    config.primarySensor.initialMode = DeviceMode::Compute;

    auto aux = std::make_shared<SubSensorConfig>();
    aux->sensorTag = "pcie_lane";
    config.auxiliarySensors.push_back(aux);

    const std::vector<std::uint8_t> buffer = serializeBinary(config);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromBinary(std::span<const std::uint8_t>{buffer}));

    CHECK(restored.nodeName == "hpc-worker-99");
    CHECK(restored.threadPoolSize == 128);
    CHECK(restored.scalingFactors.size() == 4);
    CHECK(restored.primarySensor.sensorTag == "die_top");

    REQUIRE(restored.auxiliarySensors.size() == 1);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "pcie_lane");
}

TEST_CASE("BaseObject: Optional values roundtrip across JOB formats",
          "[core][base_obj][optional][example]")
{
    OptionalConfig config;
    config.optionalValue = 42;
    config.optionalName = "optional-name";

    config.optionalSensor.emplace();
    config.optionalSensor->sensorTag = "optional-direct";

    config.optionalSharedSensor = std::make_shared<SubSensorConfig>();
    (*config.optionalSharedSensor)->sensorTag = "optional-shared";

    auto verify = [](const OptionalConfig &restored) {
        REQUIRE(restored.optionalValue);
        CHECK(*restored.optionalValue == 42);

        REQUIRE(restored.optionalName);
        CHECK(*restored.optionalName == "optional-name");

        REQUIRE(restored.optionalSensor);
        CHECK(restored.optionalSensor->sensorTag == "optional-direct");

        REQUIRE(restored.optionalSharedSensor);
        REQUIRE(*restored.optionalSharedSensor != nullptr);
        CHECK((*restored.optionalSharedSensor)->sensorTag == "optional-shared");
    };

    SECTION("JobJson")
    {
        OptionalConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        OptionalConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        OptionalConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: Shared and unique ownership roundtrip across JOB formats",
          "[core][base_obj][pointer][example]")
{
    PointerConfig config;

    config.sharedSensor = std::make_shared<SubSensorConfig>();
    config.sharedSensor->sensorTag = "shared";

    config.uniqueSensor = std::make_unique<SubSensorConfig>();
    config.uniqueSensor->sensorTag = "unique";

    auto verify = [](const PointerConfig &restored) {
        REQUIRE(restored.sharedSensor != nullptr);
        REQUIRE(restored.uniqueSensor != nullptr);

        CHECK(restored.sharedSensor->sensorTag == "shared");
        CHECK(restored.uniqueSensor->sensorTag == "unique");
    };

    SECTION("JobJson")
    {
        PointerConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        PointerConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        PointerConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

// =============================================================================
// Native JOB File API
// =============================================================================

TEST_CASE("BaseObject: JobJson file save and load roundtrip",
          "[core][base_obj][job_json][file]")
{
    const auto fileName = tempFilePath("job_core_base_obj_test.json");

    ComputeNodeConfig config;
    config.nodeName = "json-file-node";
    config.threadPoolSize = 24;
    config.primarySensor.sensorTag = "json_file_sensor";

    REQUIRE(config.saveToJobJsonFile(fileName.string()));

    ComputeNodeConfig restored;
    REQUIRE(restored.loadFromJobJsonFile(fileName.string()));

    CHECK(restored.nodeName == "json-file-node");
    CHECK(restored.threadPoolSize == 24);
    CHECK(restored.primarySensor.sensorTag == "json_file_sensor");

    std::filesystem::remove(fileName);
}

TEST_CASE("BaseObject: JobYaml file save and load roundtrip",
          "[core][base_obj][job_yaml][file]")
{
    const auto fileName = tempFilePath("job_core_base_obj_test.yaml");

    ComputeNodeConfig config;
    config.nodeName = "yaml-file-node";
    config.threadPoolSize = 48;
    config.primarySensor.sensorTag = "yaml_file_sensor";

    REQUIRE(config.saveToJobYamlFile(fileName.string()));

    ComputeNodeConfig restored;
    REQUIRE(restored.loadFromJobYamlFile(fileName.string()));

    CHECK(restored.nodeName == "yaml-file-node");
    CHECK(restored.threadPoolSize == 48);
    CHECK(restored.primarySensor.sensorTag == "yaml_file_sensor");

    std::filesystem::remove(fileName);
}

TEST_CASE("BaseObject: Binary file save and load roundtrip",
          "[core][base_obj][binary][file]")
{
    const auto fileName = tempFilePath("job_core_base_obj_test.bin");

    ComputeNodeConfig config;
    config.nodeName = "binary-file-node";
    config.threadPoolSize = 96;
    config.primarySensor.sensorTag = "binary_file_sensor";

    REQUIRE(config.saveToBinaryFile(fileName.string()));

    ComputeNodeConfig restored;
    REQUIRE(restored.loadFromBinaryFile(fileName.string()));

    CHECK(restored.nodeName == "binary-file-node");
    CHECK(restored.threadPoolSize == 96);
    CHECK(restored.primarySensor.sensorTag == "binary_file_sensor");

    std::filesystem::remove(fileName);
}

// =============================================================================
// Block 2: Edge Cases & Error Invariants
// =============================================================================

TEST_CASE("BaseObject: Empty containers and strings roundtrip",
          "[core][base_obj][edge_cases][empty]")
{
    ComputeNodeConfig config;
    config.nodeName.clear();
    config.scalingFactors.clear();
    config.auxiliarySensors.clear();

    auto verify = [](const ComputeNodeConfig &restored) {
        CHECK(restored.nodeName.empty());
        CHECK(restored.scalingFactors.empty());
        CHECK(restored.auxiliarySensors.empty());
    };

    SECTION("JobJson")
    {
        ComputeNodeConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        ComputeNodeConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        ComputeNodeConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: Null smart pointers roundtrip across JOB formats",
          "[core][base_obj][edge_cases][null_pointer]")
{
    ComputeNodeConfig config;
    config.auxiliarySensors = {
        nullptr,
        std::make_shared<SubSensorConfig>(),
        nullptr,
    };

    config.auxiliarySensors[1]->sensorTag = "middle";

    auto verify = [](const ComputeNodeConfig &restored) {
        REQUIRE(restored.auxiliarySensors.size() == 3);
        CHECK(restored.auxiliarySensors[0] == nullptr);

        REQUIRE(restored.auxiliarySensors[1] != nullptr);
        CHECK(restored.auxiliarySensors[1]->sensorTag == "middle");

        CHECK(restored.auxiliarySensors[2] == nullptr);
    };

    SECTION("JobJson")
    {
        ComputeNodeConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        ComputeNodeConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        ComputeNodeConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: Null optionals reset existing values across JOB formats",
          "[core][base_obj][edge_cases][optional][null]")
{
    OptionalConfig source;
    source.optionalValue.reset();
    source.optionalName.reset();
    source.optionalSensor.reset();
    source.optionalSharedSensor.reset();

    auto prepare = [](OptionalConfig &restored) {
        restored.optionalValue = 777;
        restored.optionalName = "must-reset";
        restored.optionalSensor.emplace();
        restored.optionalSharedSensor = std::make_shared<SubSensorConfig>();
    };

    auto verify = [](const OptionalConfig &restored) {
        CHECK_FALSE(restored.optionalValue.has_value());
        CHECK_FALSE(restored.optionalName.has_value());
        CHECK_FALSE(restored.optionalSensor.has_value());
        CHECK_FALSE(restored.optionalSharedSensor.has_value());
    };

    SECTION("JobJson")
    {
        OptionalConfig restored;
        prepare(restored);
        roundTripJobJson(source, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        OptionalConfig restored;
        prepare(restored);
        roundTripJobYaml(source, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        OptionalConfig restored;
        prepare(restored);
        roundTripBinary(source, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: Primitive special types roundtrip across JOB formats",
          "[core][base_obj][edge_cases][primitive]")
{
    PrimitiveConfig config;
    config.mode = DeviceMode::Suspended;
    config.rawByte = std::byte{0xA5};
    config.wideChar = L'Z';
    config.utf8Char = u8'Y';
    config.utf16Char = u'X';
    config.utf32Char = U'W';

    auto verify = [](const PrimitiveConfig &restored) {
        CHECK(restored.mode == DeviceMode::Suspended);
        CHECK(restored.rawByte == std::byte{0xA5});
        CHECK(restored.wideChar == L'Z');
        CHECK(restored.utf8Char == u8'Y');
        CHECK(restored.utf16Char == u'X');
        CHECK(restored.utf32Char == U'W');
    };

    SECTION("JobJson")
    {
        PrimitiveConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        PrimitiveConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        PrimitiveConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: Vector containers roundtrip at useful boundaries",
          "[core][base_obj][edge_cases][container][vector]")
{
    SECTION("Empty")
    {
        ContainerConfig config;

        ContainerConfig jsonRestored;
        roundTripJobJson(config, jsonRestored);
        CHECK(jsonRestored.values.empty());
        CHECK(jsonRestored.names.empty());

        ContainerConfig yamlRestored;
        roundTripJobYaml(config, yamlRestored);
        CHECK(yamlRestored.values.empty());
        CHECK(yamlRestored.names.empty());

        ContainerConfig binaryRestored;
        roundTripBinary(config, binaryRestored);
        CHECK(binaryRestored.values.empty());
        CHECK(binaryRestored.names.empty());
    }

    SECTION("Single element")
    {
        ContainerConfig config;
        config.values = {42};
        config.names = {"single"};

        ContainerConfig jsonRestored;
        roundTripJobJson(config, jsonRestored);
        CHECK(jsonRestored.values == config.values);
        CHECK(jsonRestored.names == config.names);

        ContainerConfig yamlRestored;
        roundTripJobYaml(config, yamlRestored);
        CHECK(yamlRestored.values == config.values);
        CHECK(yamlRestored.names == config.names);

        ContainerConfig binaryRestored;
        roundTripBinary(config, binaryRestored);
        CHECK(binaryRestored.values == config.values);
        CHECK(binaryRestored.names == config.names);
    }

    SECTION("Many elements")
    {
        ContainerConfig config;

        for (std::int32_t i = 0; i < 1024; ++i)
            config.values.push_back(i);

        ContainerConfig jsonRestored;
        roundTripJobJson(config, jsonRestored);

        REQUIRE(jsonRestored.values.size() == 1024);
        CHECK(jsonRestored.values.front() == 0);
        CHECK(jsonRestored.values.back() == 1023);

        ContainerConfig yamlRestored;
        roundTripJobYaml(config, yamlRestored);

        REQUIRE(yamlRestored.values.size() == 1024);
        CHECK(yamlRestored.values.front() == 0);
        CHECK(yamlRestored.values.back() == 1023);

        ContainerConfig binaryRestored;
        roundTripBinary(config, binaryRestored);

        REQUIRE(binaryRestored.values.size() == 1024);
        CHECK(binaryRestored.values.front() == 0);
        CHECK(binaryRestored.values.back() == 1023);
    }
}

TEST_CASE("BaseObject: Fixed arrays roundtrip across JOB formats",
          "[core][base_obj][edge_cases][container][array]")
{
    FixedContainerConfig config;
    config.values = {9, 8, 7, 6};

    SECTION("JobJson")
    {
        FixedContainerConfig restored;
        roundTripJobJson(config, restored);
        CHECK(restored.values == config.values);
    }

    SECTION("JobYaml")
    {
        FixedContainerConfig restored;
        roundTripJobYaml(config, restored);
        CHECK(restored.values == config.values);
    }

    SECTION("Binary")
    {
        FixedContainerConfig restored;
        roundTripBinary(config, restored);
        CHECK(restored.values == config.values);
    }
}

TEST_CASE("BaseObject: Fixed arrays reject size mismatches",
          "[core][base_obj][edge_cases][container][array][invalid]")
{
    SECTION("JobJson")
    {
        FixedContainerConfig restored;

        CHECK_FALSE(restored.fromJobJson(R"({"values":[1,2,3]})"));
        CHECK_FALSE(restored.lastErrorString.empty());
    }

    SECTION("JobYaml")
    {
        FixedContainerConfig restored;

        constexpr std::string_view yaml =
            "values:\n"
            "  - 1\n"
            "  - 2\n"
            "  - 3\n";

        CHECK_FALSE(restored.fromJobYaml(yaml));
        CHECK_FALSE(restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: Set containers roundtrip across JOB formats",
          "[core][base_obj][edge_cases][container][set]")
{
    SetConfig config;
    config.values = {5, 1, 9, 5};
    config.names = {"beta", "alpha", "gamma"};

    SECTION("JobJson")
    {
        SetConfig restored;
        roundTripJobJson(config, restored);

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }

    SECTION("JobYaml")
    {
        SetConfig restored;
        roundTripJobYaml(config, restored);

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }

    SECTION("Binary")
    {
        SetConfig restored;
        roundTripBinary(config, restored);

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }
}

TEST_CASE("BaseObject: Map containers roundtrip across JOB formats",
          "[core][base_obj][edge_cases][container][map]")
{
    MapConfig config;
    config.entries = {
                      {1, "cpu"},
                      {2, "cuda"},
                      {3, "vulkan"},
                      };

    SubSensorConfig cpu;
    cpu.sensorTag = "cpu";
    cpu.sampleRateHz = 100.0f;

    SubSensorConfig gpu;
    gpu.sensorTag = "gpu";
    gpu.sampleRateHz = 200.0f;

    config.sensors.emplace("cpu", cpu);
    config.sensors.emplace("gpu", gpu);

    auto verify = [&](const MapConfig &restored) {
        CHECK(restored.entries == config.entries);

        REQUIRE(restored.sensors.size() == 2);
        CHECK(restored.sensors.at("cpu").sensorTag == "cpu");
        CHECK(restored.sensors.at("gpu").sampleRateHz == 200.0f);
    };

    SECTION("JobJson")
    {
        MapConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        MapConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        MapConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: BinaryMapConfig remains valid across JOB formats",
          "[core][base_obj][edge_cases][map]")
{
    BinaryMapConfig config;
    config.entries = {
                      {1, "cpu"},
                      {2, "cuda"},
                      {3, "vulkan"},
                      };

    SECTION("JobJson")
    {
        BinaryMapConfig restored;
        roundTripJobJson(config, restored);
        CHECK(restored.entries == config.entries);
    }

    SECTION("JobYaml")
    {
        BinaryMapConfig restored;
        roundTripJobYaml(config, restored);
        CHECK(restored.entries == config.entries);
    }

    SECTION("Binary")
    {
        BinaryMapConfig restored;
        roundTripBinary(config, restored);
        CHECK(restored.entries == config.entries);
    }
}

TEST_CASE("BaseObject: Nested container combinations roundtrip across JOB formats",
          "[core][base_obj][edge_cases][container][nested]")
{
    NestedContainerConfig config;

    SubSensorConfig direct;
    direct.sensorTag = "optional-direct";
    config.optionalSensors.push_back(direct);
    config.optionalSensors.push_back(std::nullopt);

    auto shared = std::make_shared<SubSensorConfig>();
    shared->sensorTag = "shared-vector";
    config.sharedSensors.push_back(shared);
    config.sharedSensors.push_back(nullptr);

    auto named = std::make_shared<SubSensorConfig>();
    named->sensorTag = "named-shared";
    config.namedSensors.emplace("sensor-a", named);
    config.namedSensors.emplace("sensor-null", nullptr);

    auto verify = [](const NestedContainerConfig &restored) {
        REQUIRE(restored.optionalSensors.size() == 2);
        REQUIRE(restored.optionalSensors[0]);
        CHECK(restored.optionalSensors[0]->sensorTag == "optional-direct");
        CHECK_FALSE(restored.optionalSensors[1].has_value());

        REQUIRE(restored.sharedSensors.size() == 2);
        REQUIRE(restored.sharedSensors[0] != nullptr);
        CHECK(restored.sharedSensors[0]->sensorTag == "shared-vector");
        CHECK(restored.sharedSensors[1] == nullptr);

        REQUIRE(restored.namedSensors.size() == 2);
        REQUIRE(restored.namedSensors.at("sensor-a") != nullptr);
        CHECK(restored.namedSensors.at("sensor-a")->sensorTag == "named-shared");
        CHECK(restored.namedSensors.at("sensor-null") == nullptr);
    };

    SECTION("JobJson")
    {
        NestedContainerConfig restored;
        roundTripJobJson(config, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        NestedContainerConfig restored;
        roundTripJobYaml(config, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        NestedContainerConfig restored;
        roundTripBinary(config, restored);
        verify(restored);
    }
}



TEST_CASE("BaseObject serialization cross-compatibility gate",
          "[core][base_obj][serialization][compatibility][gate]")
{
    ComputeNodeConfig source;
    source.nodeName = "benchmark-compute-node";
    source.threadPoolSize = 64;
    source.memoryBudgetGb = 128.0;
    source.scalingFactors.resize(256, 1.0f);

    for (int i = 0; i < 16; ++i) {
        auto sensor = std::make_shared<SubSensorConfig>();
        sensor->sensorTag = "sensor_channel_" + std::to_string(i);
        sensor->sampleRateHz = static_cast<float>(1000.0 / (i + 1));
        sensor->calibrateOnBoot = (i % 2) == 0;
        sensor->initialMode = (i % 3) == 0 ? DeviceMode::Compute : DeviceMode::Idle;
        source.auxiliarySensors.push_back(std::move(sensor));
    }

    auto verify = [&](const ComputeNodeConfig &restored) {
        CHECK(restored.nodeName == source.nodeName);
        CHECK(restored.threadPoolSize == source.threadPoolSize);
        CHECK(restored.memoryBudgetGb == source.memoryBudgetGb);
        CHECK(restored.scalingFactors == source.scalingFactors);

        REQUIRE(restored.auxiliarySensors.size() == source.auxiliarySensors.size());

        for (std::size_t i = 0; i < source.auxiliarySensors.size(); ++i) {
            REQUIRE(source.auxiliarySensors[i] != nullptr);
            REQUIRE(restored.auxiliarySensors[i] != nullptr);

            CHECK(restored.auxiliarySensors[i]->sensorTag ==
                  source.auxiliarySensors[i]->sensorTag);

            CHECK(restored.auxiliarySensors[i]->sampleRateHz ==
                  source.auxiliarySensors[i]->sampleRateHz);

            CHECK(restored.auxiliarySensors[i]->calibrateOnBoot ==
                  source.auxiliarySensors[i]->calibrateOnBoot);

            CHECK(restored.auxiliarySensors[i]->initialMode ==
                  source.auxiliarySensors[i]->initialMode);
        }
    };

    std::string jobJson;
    REQUIRE(source.toJobJson(jobJson));

    const std::string nlohmannJson = source.toJson().dump();

    std::string jobYaml;
    REQUIRE(source.toJobYaml(jobYaml));

    const std::string yamlCpp = YAML::Dump(source.toYaml());

    std::vector<std::uint8_t> binary;
    REQUIRE(source.toBinary(binary));

    SECTION("JobJson parses nlohmann JSON text")
    {
        ComputeNodeConfig restored;

        REQUIRE(restored.fromJobJson(nlohmannJson));
        verify(restored);
    }

    SECTION("nlohmann parses JobJson text")
    {
        ComputeNodeConfig restored;

        REQUIRE(restored.fromJson(nlohmann::json::parse(jobJson)));
        verify(restored);
    }

    SECTION("JobYaml parses yaml-cpp YAML text")
    {
        ComputeNodeConfig restored;

        REQUIRE(restored.fromJobYaml(yamlCpp));
        verify(restored);
    }

    SECTION("yaml-cpp parses JobYaml text")
    {
        ComputeNodeConfig restored;

        REQUIRE(restored.fromYaml(YAML::Load(jobYaml)));
        verify(restored);
    }

    SECTION("JobBinary roundtrip")
    {
        ComputeNodeConfig restored;

        REQUIRE(restored.fromBinary(std::span<const std::uint8_t>{binary}));
        verify(restored);
    }
}


// =============================================================================
// Annotation / Reflection Policy
// =============================================================================

TEST_CASE("BaseObject: NoSerialize annotations exclude members from JOB persistence",
          "[core][base_obj][edge_cases][annotation]")
{
    AnnotationConfig config;
    config.serializedValue = "persistent";
    config.transientValue = "transient-secret";
    config.noResetValue = "still-serialized";
    config.runtimeValue = "runtime-secret";

    SECTION("JobJson")
    {
        const std::string json = serializeJobJson(config);

        CHECK(json.find("\"serializedValue\"") != std::string::npos);
        CHECK(json.find("\"noResetValue\"") != std::string::npos);
        CHECK(json.find("\"transientValue\"") == std::string::npos);
        CHECK(json.find("\"runtimeValue\"") == std::string::npos);
    }

    SECTION("JobYaml")
    {
        const std::string yaml = serializeJobYaml(config);

        CHECK(yaml.find("serializedValue:") != std::string::npos);
        CHECK(yaml.find("noResetValue:") != std::string::npos);
        CHECK(yaml.find("transientValue:") == std::string::npos);
        CHECK(yaml.find("runtimeValue:") == std::string::npos);
    }

    SECTION("Binary")
    {
        AnnotationConfig differentTransient;
        differentTransient.serializedValue = config.serializedValue;
        differentTransient.noResetValue = config.noResetValue;
        differentTransient.transientValue = "completely-different";
        differentTransient.runtimeValue = "also-different";

        const std::vector<std::uint8_t> first = serializeBinary(config);
        const std::vector<std::uint8_t> second = serializeBinary(differentTransient);

        CHECK(first == second);
    }
}

TEST_CASE("BaseObject: NoSerialize members survive deserialization",
          "[core][base_obj][edge_cases][annotation][deserialize]")
{
    AnnotationConfig source;
    source.serializedValue = "serialized-new";
    source.noResetValue = "no-reset-new";

    auto prepare = [](AnnotationConfig &restored) {
        restored.transientValue = "keep-transient";
        restored.runtimeValue = "keep-runtime";
    };

    auto verify = [](const AnnotationConfig &restored) {
        CHECK(restored.serializedValue == "serialized-new");
        CHECK(restored.noResetValue == "no-reset-new");
        CHECK(restored.transientValue == "keep-transient");
        CHECK(restored.runtimeValue == "keep-runtime");
    };

    SECTION("JobJson")
    {
        AnnotationConfig restored;
        prepare(restored);
        roundTripJobJson(source, restored);
        verify(restored);
    }

    SECTION("JobYaml")
    {
        AnnotationConfig restored;
        prepare(restored);
        roundTripJobYaml(source, restored);
        verify(restored);
    }

    SECTION("Binary")
    {
        AnnotationConfig restored;
        prepare(restored);
        roundTripBinary(source, restored);
        verify(restored);
    }
}

TEST_CASE("BaseObject: lastErrorString is runtime state and is not serialized",
          "[core][base_obj][edge_cases][error_state]")
{
    ErrorStateConfig config;
    config.value = "real-data";
    config.lastErrorString = "this must not serialize";

    SECTION("JobJson")
    {
        const std::string json = serializeJobJson(config);

        CHECK(json.find("\"value\"") != std::string::npos);
        CHECK(json.find("\"lastErrorString\"") == std::string::npos);
    }

    SECTION("JobYaml")
    {
        const std::string yaml = serializeJobYaml(config);

        CHECK(yaml.find("value:") != std::string::npos);
        CHECK(yaml.find("lastErrorString:") == std::string::npos);
    }

    SECTION("Binary")
    {
        const std::vector<std::uint8_t> withError = serializeBinary(config);

        ErrorStateConfig cleanConfig;
        cleanConfig.value = "real-data";

        const std::vector<std::uint8_t> withoutError = serializeBinary(cleanConfig);

        CHECK(withError == withoutError);
    }
}

// =============================================================================
// Missing Fields
// =============================================================================

TEST_CASE("BaseObject: Missing JobJson members preserve existing values",
          "[core][base_obj][job_json][missing_fields]")
{
    ComputeNodeConfig restored;
    restored.nodeName = "existing-node";
    restored.threadPoolSize = 777;

    REQUIRE(restored.fromJobJson(R"({"nodeName":"updated-node"})"));

    CHECK(restored.nodeName == "updated-node");
    CHECK(restored.threadPoolSize == 777);
}

TEST_CASE("BaseObject: Missing JobYaml members preserve existing values",
          "[core][base_obj][job_yaml][missing_fields]")
{
    ComputeNodeConfig restored;
    restored.nodeName = "existing-node";
    restored.threadPoolSize = 888;

    REQUIRE(restored.fromJobYaml("nodeName: updated-node\n"));

    CHECK(restored.nodeName == "updated-node");
    CHECK(restored.threadPoolSize == 888);
}

// =============================================================================
// Invalid Input / Error Propagation
// =============================================================================

TEST_CASE("BaseObject: JobJson non-object root fails gracefully",
          "[core][base_obj][job_json][invalid][root]")
{
    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobJson(R"("invalid_scalar_root")"));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: JobYaml non-mapping root fails gracefully",
          "[core][base_obj][job_yaml][invalid][root]")
{
    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobYaml("invalid_scalar_root\n"));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Invalid JobJson member type fails gracefully",
          "[core][base_obj][job_json][invalid]")
{
    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobJson(R"({"threadPoolSize":"not-an-integer"})"));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Invalid JobYaml member type fails gracefully",
          "[core][base_obj][job_yaml][invalid]")
{
    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobYaml("threadPoolSize: not-an-integer\n"));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Nested JobJson failures propagate to parent",
          "[core][base_obj][job_json][nested_error]")
{
    constexpr std::string_view json =
        R"({"nodeName":"nested-json","primarySensor":{"sampleRateHz":"not-a-float"}})";

    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobJson(json));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Nested JobYaml failures propagate to parent",
          "[core][base_obj][job_yaml][nested_error]")
{
    constexpr std::string_view yaml =
        "nodeName: nested-yaml\n"
        "primarySensor:\n"
        "  sampleRateHz: not-a-float\n";

    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobYaml(yaml));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Empty binary input fails gracefully",
          "[core][base_obj][binary][empty]")
{
    ComputeNodeConfig restored;
    const std::vector<std::uint8_t> buffer;

    CHECK_FALSE(restored.fromBinary(std::span<const std::uint8_t>{buffer}));
    CHECK_FALSE(restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Binary truncation fails gracefully",
          "[core][base_obj][binary][truncated]")
{
    ComputeNodeConfig config;
    config.nodeName = "boundary-test";
    config.scalingFactors = {1.0f, 2.0f, 3.0f, 4.0f};

    const std::vector<std::uint8_t> completeBuffer = serializeBinary(config);

    REQUIRE(completeBuffer.size() > 8);

    const std::array<std::size_t, 5> sizes{
        0,
        1,
        completeBuffer.size() / 4,
        completeBuffer.size() / 2,
        completeBuffer.size() - 1,
    };

    for (const std::size_t size : sizes) {
        const std::span<const std::uint8_t> truncated{
            completeBuffer.data(),
            size,
        };

        ComputeNodeConfig restored;

        INFO("Truncated buffer size: " << size);
        CHECK_FALSE(restored.fromBinary(truncated));
        CHECK_FALSE(restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: Binary nested object failure propagates",
          "[core][base_obj][binary][nested_error]")
{
    ComputeNodeConfig config;
    config.nodeName = "nested-binary-failure";
    config.primarySensor.sensorTag = "nested";

    std::vector<std::uint8_t> buffer = serializeBinary(config);

    REQUIRE(buffer.size() > 1);
    buffer.pop_back();

    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromBinary(std::span<const std::uint8_t>{buffer}));
    CHECK_FALSE(restored.lastErrorString.empty());
}

// =============================================================================
// File Failure Semantics
// =============================================================================

TEST_CASE("BaseObject: Missing JOB files populate lastErrorString",
          "[core][base_obj][edge_cases][file][failure]")
{
    SECTION("JobJson")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.json");

        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromJobJsonFile(fileName.string()));
        CHECK_FALSE(restored.lastErrorString.empty());
    }

    SECTION("JobYaml")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.yaml");

        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromJobYamlFile(fileName.string()));
        CHECK_FALSE(restored.lastErrorString.empty());
    }

    SECTION("Binary")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.bin");

        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromBinaryFile(fileName.string()));
        CHECK_FALSE(restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: Malformed JOB files fail gracefully",
          "[core][base_obj][edge_cases][file][malformed]")
{
    SECTION("JobJson")
    {
        const auto fileName = tempFilePath("job_core_malformed_base_obj.json");

        {
            std::ofstream file(fileName);
            REQUIRE(file.is_open());
            file << "{ definitely-not-valid-json";
        }

        ComputeNodeConfig restored;

        CHECK_FALSE(restored.loadFromJobJsonFile(fileName.string()));
        CHECK_FALSE(restored.lastErrorString.empty());

        std::filesystem::remove(fileName);
    }

    SECTION("JobYaml")
    {
        const auto fileName = tempFilePath("job_core_malformed_base_obj.yaml");

        {
            std::ofstream file(fileName);
            REQUIRE(file.is_open());
            file << "root: [unterminated";
        }

        ComputeNodeConfig restored;

        CHECK_FALSE(restored.loadFromJobYamlFile(fileName.string()));
        CHECK_FALSE(restored.lastErrorString.empty());

        std::filesystem::remove(fileName);
    }
}

// =============================================================================
// Current Mutation Semantics
// =============================================================================

TEST_CASE("BaseObject: Failed JobJson deserialization documents partial mutation semantics",
          "[core][base_obj][job_json][failure_state]")
{
    ComputeNodeConfig restored;
    restored.nodeName = "before";
    restored.threadPoolSize = 999;

    constexpr std::string_view json =
        R"({"nodeName":"after","primarySensor":{"sampleRateHz":"broken"}})";

    const bool success = restored.fromJobJson(json);

    REQUIRE_FALSE(success);

    // Deserialization currently mutates members as parsing proceeds.
    // A later failure does not roll back fields already written.
    CHECK(restored.nodeName == "after");
    CHECK_FALSE(restored.lastErrorString.empty());
}

// =============================================================================
// Scalar Parsing Coverage
// =============================================================================

TEST_CASE("BaseObject: JobYaml floating scalar spellings",
          "[core][base_obj][job_yaml][scalar][float]")
{
    SubSensorConfig restored;

    REQUIRE(restored.fromJobYaml("sampleRateHz: 250\n"));
    CHECK(restored.sampleRateHz == 250.0f);

    REQUIRE(restored.fromJobYaml("sampleRateHz: 250.0\n"));
    CHECK(restored.sampleRateHz == 250.0f);
}

TEST_CASE("BaseObject: JobYaml boolean scalar spellings",
          "[core][base_obj][job_yaml][scalar][bool]")
{
    SubSensorConfig restored;

    REQUIRE(restored.fromJobYaml("calibrateOnBoot: false\n"));
    CHECK_FALSE(restored.calibrateOnBoot);

    REQUIRE(restored.fromJobYaml("calibrateOnBoot: true\n"));
    CHECK(restored.calibrateOnBoot);
}

// =============================================================================
// Pointer Concept Coverage
// =============================================================================

TEST_CASE("BaseObject: Pointer concepts distinguish ownership semantics",
          "[core][base_obj][edge_cases][smart_pointer]")
{
    STATIC_REQUIRE(SmartPointer<std::shared_ptr<SubSensorConfig>>);
    STATIC_REQUIRE(SmartPointer<std::unique_ptr<SubSensorConfig>>);
    STATIC_REQUIRE_FALSE(SmartPointer<int>);

    STATIC_REQUIRE(SharedPointer<std::shared_ptr<SubSensorConfig>>);
    STATIC_REQUIRE(UniquePointer<std::unique_ptr<SubSensorConfig>>);
    STATIC_REQUIRE(WeakPointer<std::weak_ptr<SubSensorConfig>>);

    STATIC_REQUIRE(OwningSmartPointer<std::shared_ptr<SubSensorConfig>>);
    STATIC_REQUIRE(OwningSmartPointer<std::unique_ptr<SubSensorConfig>>);
    STATIC_REQUIRE_FALSE(OwningSmartPointer<std::weak_ptr<SubSensorConfig>>);

    STATIC_REQUIRE(UnsupportedPersistentPointer<SubSensorConfig *>);
    STATIC_REQUIRE(UnsupportedPersistentPointer<std::weak_ptr<SubSensorConfig>>);
}

// =============================================================================
// Temporary Third-Party Compatibility Coverage
//
// Delete this entire block when nlohmann/json and yaml-cpp leave BaseObject.
// =============================================================================

TEST_CASE("BaseObject: nlohmann JSON compatibility remains intact",
          "[core][base_obj][third_party][json]")
{
    ComputeNodeConfig config;
    config.nodeName = "legacy-json-node";
    config.threadPoolSize = 12;

    const nlohmann::json serialized = config.toJson();

    REQUIRE(serialized.is_object());
    CHECK(serialized["nodeName"] == "legacy-json-node");
    CHECK(serialized["threadPoolSize"] == 12);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJson(serialized));

    CHECK(restored.nodeName == config.nodeName);
    CHECK(restored.threadPoolSize == config.threadPoolSize);
}

TEST_CASE("BaseObject: nlohmann ADL compatibility remains intact",
          "[core][base_obj][third_party][json][integration]")
{
    ComputeNodeConfig config;
    config.nodeName = "adl-json-node";
    config.threadPoolSize = 12;

    nlohmann::json serialized = config;

    ComputeNodeConfig restored = serialized.get<ComputeNodeConfig>();

    CHECK(restored.nodeName == "adl-json-node");
    CHECK(restored.threadPoolSize == 12);
}

TEST_CASE("BaseObject: yaml-cpp compatibility remains intact",
          "[core][base_obj][third_party][yaml]")
{
    ComputeNodeConfig config;
    config.nodeName = "legacy-yaml-node";
    config.threadPoolSize = 20;

    const YAML::Node serialized = config.toYaml();

    REQUIRE(serialized.IsMap());

    ComputeNodeConfig restored;
    REQUIRE(restored.fromYaml(serialized));

    CHECK(restored.nodeName == config.nodeName);
    CHECK(restored.threadPoolSize == config.threadPoolSize);
}

TEST_CASE("BaseObject: YAML convert compatibility remains intact",
          "[core][base_obj][third_party][yaml][integration]")
{
    ComputeNodeConfig config;
    config.nodeName = "yaml-convert-node";
    config.threadPoolSize = 20;

    const YAML::Node serialized = YAML::convert<ComputeNodeConfig>::encode(config);

    ComputeNodeConfig restored;
    REQUIRE(YAML::convert<ComputeNodeConfig>::decode(serialized, restored));

    CHECK(restored.nodeName == "yaml-convert-node");
    CHECK(restored.threadPoolSize == 20);
}

// =============================================================================
// Block 3: Benchmarks / Stress
// =============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("BaseObject serialization benchmarks",
          "[core][base_obj][benchmark]")
{
    ComputeNodeConfig config;
    config.nodeName = "benchmark-compute-node";
    config.threadPoolSize = 64;
    config.memoryBudgetGb = 128.0;
    config.scalingFactors.resize(256, 1.0f);

    for (int i = 0; i < 16; ++i) {
        auto sensor = std::make_shared<SubSensorConfig>();
        sensor->sensorTag = "sensor_channel_" + std::to_string(i);
        sensor->sampleRateHz = static_cast<float>(1000.0 / (i + 1));
        config.auxiliarySensors.push_back(std::move(sensor));
    }

    std::vector<std::uint8_t> binaryPayload;
    REQUIRE(config.toBinary(binaryPayload));

    std::string jobJsonPayload;
    REQUIRE(config.toJobJson(jobJsonPayload));

    std::string jobYamlPayload;
    REQUIRE(config.toJobYaml(jobYamlPayload));

    const std::string nlohmannJsonPayload = config.toJson().dump();
    const std::string yamlCppPayload = YAML::Dump(config.toYaml());

    BENCHMARK("Binary Serialization")
    {
        std::vector<std::uint8_t> output;
        config.toBinary(output);
        return output.size();
    };

    BENCHMARK("Binary Deserialization")
    {
        ComputeNodeConfig restored;

        if (!restored.fromBinary(std::span<const std::uint8_t>{binaryPayload}))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("nlohmann JSON Serialization")
    {
        const std::string output = config.toJson().dump();
        return output.size();
    };

    BENCHMARK("nlohmann JSON Deserialization")
    {
        const nlohmann::json json = nlohmann::json::parse(nlohmannJsonPayload);

        ComputeNodeConfig restored;

        if (!restored.fromJson(json))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("JobJson Serialization")
    {
        std::string output;
        config.toJobJson(output);
        return output.size();
    };

    BENCHMARK("JobJson Deserialization")
    {
        ComputeNodeConfig restored;

        if (!restored.fromJobJson(jobJsonPayload))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("yaml-cpp Serialization")
    {
        const std::string output = YAML::Dump(config.toYaml());
        return output.size();
    };

    BENCHMARK("yaml-cpp Deserialization")
    {
        const YAML::Node yaml = YAML::Load(yamlCppPayload);

        ComputeNodeConfig restored;

        if (!restored.fromYaml(yaml))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("JobYaml Serialization")
    {
        std::string output;
        config.toJobYaml(output);
        return output.size();
    };

    BENCHMARK("JobYaml Deserialization")
    {
        ComputeNodeConfig restored;

        if (!restored.fromJobYaml(jobYamlPayload))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };
}

TEST_CASE("BaseObject nested serialization stress benchmark",
          "[core][base_obj][benchmark][stress]")
{
    ComputeNodeConfig config;
    config.nodeName = "nested-stress-node";
    config.threadPoolSize = 128;
    config.memoryBudgetGb = 256.0;
    config.scalingFactors.resize(4096, 0.5f);

    for (int i = 0; i < 256; ++i) {
        auto sensor = std::make_shared<SubSensorConfig>();
        sensor->sensorTag = "stress_sensor_" + std::to_string(i);
        sensor->sampleRateHz = static_cast<float>(i + 1);
        sensor->calibrateOnBoot = (i % 2) == 0;
        sensor->initialMode = (i % 3) == 0 ? DeviceMode::Compute : DeviceMode::Idle;
        config.auxiliarySensors.push_back(std::move(sensor));
    }

    BENCHMARK("Large nested Binary roundtrip")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ComputeNodeConfig restored;

        if (!restored.fromBinary(std::span<const std::uint8_t>{buffer}))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested nlohmann JSON roundtrip")
    {
        const std::string jsonPayload = config.toJson().dump();
        const nlohmann::json json = nlohmann::json::parse(jsonPayload);

        ComputeNodeConfig restored;

        if (!restored.fromJson(json))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested JobJson roundtrip")
    {
        std::string json;
        config.toJobJson(json);

        ComputeNodeConfig restored;

        if (!restored.fromJobJson(json))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested yaml-cpp roundtrip")
    {
        const std::string yamlPayload = YAML::Dump(config.toYaml());
        const YAML::Node yaml = YAML::Load(yamlPayload);

        ComputeNodeConfig restored;

        if (!restored.fromYaml(yaml))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested JobYaml roundtrip")
    {
        std::string yaml;
        config.toJobYaml(yaml);

        ComputeNodeConfig restored;

        if (!restored.fromJobYaml(yaml))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };
}

TEST_CASE("BaseObject mixed container stress benchmark",
          "[core][base_obj][benchmark][container][stress]")
{
    NestedContainerConfig config;

    for (int i = 0; i < 256; ++i) {
        SubSensorConfig direct;
        direct.sensorTag = "optional_" + std::to_string(i);
        direct.sampleRateHz = static_cast<float>(i + 1);
        config.optionalSensors.emplace_back(std::move(direct));

        auto shared = std::make_shared<SubSensorConfig>();
        shared->sensorTag = "shared_" + std::to_string(i);

        config.sharedSensors.push_back(shared);
        config.namedSensors.emplace("sensor_" + std::to_string(i), std::move(shared));
    }

    BENCHMARK("Mixed nested Binary roundtrip")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        NestedContainerConfig restored;

        if (!restored.fromBinary(std::span<const std::uint8_t>{buffer}))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested nlohmann JSON roundtrip")
    {
        const std::string jsonPayload = config.toJson().dump();
        const nlohmann::json json = nlohmann::json::parse(jsonPayload);

        NestedContainerConfig restored;

        if (!restored.fromJson(json))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested JobJson roundtrip")
    {
        std::string json;
        config.toJobJson(json);

        NestedContainerConfig restored;

        if (!restored.fromJobJson(json))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested yaml-cpp roundtrip")
    {
        const std::string yamlPayload = YAML::Dump(config.toYaml());
        const YAML::Node yaml = YAML::Load(yamlPayload);

        NestedContainerConfig restored;

        if (!restored.fromYaml(yaml))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested JobYaml roundtrip")
    {
        std::string yaml;
        config.toJobYaml(yaml);

        NestedContainerConfig restored;

        if (!restored.fromJobYaml(yaml))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };
}

#endif // JOB_TEST_BENCHMARKS
