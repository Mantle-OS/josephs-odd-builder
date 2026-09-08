#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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
#include <vector>

#include <job_base_obj.h>
#include <job_obj_concept.h>

#include <job_yaml_node.h>

#include "test_job_object_fixtures.h"

using namespace job::core;
using namespace job::core::tests;

// =============================================================================
// Block 1: Usage / Examples
// =============================================================================

TEST_CASE("BaseObject: JSON serialization roundtrip with nested objects and smart pointers", "[core][base_obj][json][example]")
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

    const nlohmann::json serializedJson = config.toJson();
    REQUIRE(serializedJson.is_object());
    REQUIRE(serializedJson["nodeName"] == "edge-inference-01");
    REQUIRE(serializedJson["threadPoolSize"] == 32);
    REQUIRE(serializedJson["auxiliarySensors"].is_array());
    REQUIRE(serializedJson["auxiliarySensors"].size() == 2);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJson(serializedJson));

    CHECK(restored.nodeName == "edge-inference-01");
    CHECK(restored.threadPoolSize == 32);
    CHECK(restored.memoryBudgetGb == 64.0);
    CHECK(restored.scalingFactors == std::vector<float>{1.25f, 2.5f, 5.0f});
    CHECK(restored.primarySensor.sensorTag == "core_temp");
    CHECK(restored.primarySensor.sampleRateHz == 250.0f);
    CHECK(restored.primarySensor.calibrateOnBoot == false);
    CHECK(restored.primarySensor.initialMode == DeviceMode::Compute);

    REQUIRE(restored.auxiliarySensors.size() == 2);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "ambient_0");
    CHECK(restored.auxiliarySensors[0]->sampleRateHz == 10.0f);
    REQUIRE(restored.auxiliarySensors[1] != nullptr);
    CHECK(restored.auxiliarySensors[1]->sensorTag == "vram_hotspot");
    CHECK(restored.auxiliarySensors[1]->sampleRateHz == 500.0f);
}

TEST_CASE("BaseObject: YAML serialization roundtrip", "[core][base_obj][yaml][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "cluster-head";
    config.primarySensor.sensorTag = "inlet_flow";
    config.primarySensor.initialMode = DeviceMode::Suspended;

    const YAML::Node yamlNode = config.toYaml();
    REQUIRE(yamlNode.IsMap());
    REQUIRE(yamlNode["nodeName"].as<std::string>() == "cluster-head");

    ComputeNodeConfig restored;
    REQUIRE(restored.fromYaml(yamlNode));

    CHECK(restored.nodeName == "cluster-head");
    CHECK(restored.primarySensor.sensorTag == "inlet_flow");
    CHECK(restored.primarySensor.initialMode == DeviceMode::Suspended);
}

TEST_CASE("BaseObject: Binary serialization roundtrip across span stream", "[core][base_obj][binary][example]")
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

    std::vector<std::uint8_t> buffer;
    config.toBinary(buffer);
    REQUIRE(!buffer.empty());

    ComputeNodeConfig restored;
    std::span<const std::uint8_t> streamSpan(buffer);
    REQUIRE(restored.fromBinary(streamSpan));

    CHECK(streamSpan.empty());
    CHECK(restored.nodeName == "hpc-worker-99");
    CHECK(restored.threadPoolSize == 128);
    CHECK(restored.scalingFactors.size() == 4);
    CHECK(restored.primarySensor.sensorTag == "die_top");
    REQUIRE(restored.auxiliarySensors.size() == 1);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "pcie_lane");
}

TEST_CASE("BaseObject: Optional values roundtrip across supported formats", "[core][base_obj][optional][example]")
{
    OptionalConfig config;
    config.optionalValue = 42;
    config.optionalName = "optional-name";
    config.optionalSensor.emplace();
    config.optionalSensor->sensorTag = "optional-direct";

    config.optionalSharedSensor = std::make_shared<SubSensorConfig>();
    (*config.optionalSharedSensor)->sensorTag = "optional-shared";

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        OptionalConfig restored;
        REQUIRE(restored.fromJson(serialized));

        REQUIRE(restored.optionalValue);
        CHECK(*restored.optionalValue == 42);
        REQUIRE(restored.optionalName);
        CHECK(*restored.optionalName == "optional-name");
        REQUIRE(restored.optionalSensor);
        CHECK(restored.optionalSensor->sensorTag == "optional-direct");
        REQUIRE(restored.optionalSharedSensor);
        REQUIRE(*restored.optionalSharedSensor != nullptr);
        CHECK((*restored.optionalSharedSensor)->sensorTag == "optional-shared");
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        OptionalConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        REQUIRE(restored.optionalValue);
        CHECK(*restored.optionalValue == 42);
        REQUIRE(restored.optionalName);
        CHECK(*restored.optionalName == "optional-name");
        REQUIRE(restored.optionalSensor);
        CHECK(restored.optionalSensor->sensorTag == "optional-direct");
        REQUIRE(restored.optionalSharedSensor);
        REQUIRE(*restored.optionalSharedSensor != nullptr);
        CHECK((*restored.optionalSharedSensor)->sensorTag == "optional-shared");
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        OptionalConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        REQUIRE(restored.optionalValue);
        CHECK(*restored.optionalValue == 42);
        REQUIRE(restored.optionalName);
        CHECK(*restored.optionalName == "optional-name");
        REQUIRE(restored.optionalSensor);
        CHECK(restored.optionalSensor->sensorTag == "optional-direct");
        REQUIRE(restored.optionalSharedSensor);
        REQUIRE(*restored.optionalSharedSensor != nullptr);
        CHECK((*restored.optionalSharedSensor)->sensorTag == "optional-shared");
    }
}

TEST_CASE("BaseObject: Shared and unique ownership roundtrip", "[core][base_obj][pointer][example]")
{
    PointerConfig config;

    config.sharedSensor = std::make_shared<SubSensorConfig>();
    config.sharedSensor->sensorTag = "shared";

    config.uniqueSensor = std::make_unique<SubSensorConfig>();
    config.uniqueSensor->sensorTag = "unique";

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        PointerConfig restored;
        REQUIRE(restored.fromJson(serialized));

        REQUIRE(restored.sharedSensor != nullptr);
        REQUIRE(restored.uniqueSensor != nullptr);
        CHECK(restored.sharedSensor->sensorTag == "shared");
        CHECK(restored.uniqueSensor->sensorTag == "unique");
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        PointerConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        REQUIRE(restored.sharedSensor != nullptr);
        REQUIRE(restored.uniqueSensor != nullptr);
        CHECK(restored.sharedSensor->sensorTag == "shared");
        CHECK(restored.uniqueSensor->sensorTag == "unique");
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        PointerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        REQUIRE(restored.sharedSensor != nullptr);
        REQUIRE(restored.uniqueSensor != nullptr);
        CHECK(restored.sharedSensor->sensorTag == "shared");
        CHECK(restored.uniqueSensor->sensorTag == "unique");
    }
}

TEST_CASE("BaseObject: JSON file save and load roundtrip", "[core][base_obj][json][file][example]")
{
    const auto fileName = tempFilePath("job_core_base_obj_test.json");

    ComputeNodeConfig config;
    config.nodeName = "json-file-node";
    config.threadPoolSize = 24;
    config.primarySensor.sensorTag = "json_file_sensor";

    REQUIRE(config.saveToJsonFile(fileName.string()));

    ComputeNodeConfig restored;
    REQUIRE(restored.loadFromJsonFile(fileName.string()));

    CHECK(restored.nodeName == "json-file-node");
    CHECK(restored.threadPoolSize == 24);
    CHECK(restored.primarySensor.sensorTag == "json_file_sensor");

    std::filesystem::remove(fileName);
}

TEST_CASE("BaseObject: YAML file save and load roundtrip", "[core][base_obj][yaml][file][example]")
{
    const auto fileName = tempFilePath("job_core_base_obj_test.yaml");

    ComputeNodeConfig config;
    config.nodeName = "yaml-file-node";
    config.threadPoolSize = 48;
    config.primarySensor.sensorTag = "yaml_file_sensor";

    REQUIRE(config.saveToYamlFile(fileName.string()));

    ComputeNodeConfig restored;
    REQUIRE(restored.loadFromYamlFile(fileName.string()));

    CHECK(restored.nodeName == "yaml-file-node");
    CHECK(restored.threadPoolSize == 48);
    CHECK(restored.primarySensor.sensorTag == "yaml_file_sensor");

    std::filesystem::remove(fileName);
}

TEST_CASE("BaseObject: Binary file save and load roundtrip", "[core][base_obj][binary][file][example]")
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

TEST_CASE("BaseObject: nlohmann JSON integration uses BaseObject serializer", "[core][base_obj][json][integration][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "adl-json-node";
    config.threadPoolSize = 12;

    nlohmann::json serialized = config;

    REQUIRE(serialized.is_object());
    CHECK(serialized["nodeName"] == "adl-json-node");
    CHECK(serialized["threadPoolSize"] == 12);

    ComputeNodeConfig restored = serialized.get<ComputeNodeConfig>();

    CHECK(restored.nodeName == "adl-json-node");
    CHECK(restored.threadPoolSize == 12);
}

TEST_CASE("BaseObject: YAML convert integration uses BaseObject serializer", "[core][base_obj][yaml][integration][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "yaml-convert-node";
    config.threadPoolSize = 20;

    YAML::Node serialized = YAML::convert<ComputeNodeConfig>::encode(config);

    REQUIRE(serialized.IsMap());
    CHECK(serialized["nodeName"].as<std::string>() == "yaml-convert-node");
    CHECK(serialized["threadPoolSize"].as<std::uint32_t>() == 20);

    ComputeNodeConfig restored;
    REQUIRE(YAML::convert<ComputeNodeConfig>::decode(serialized, restored));

    CHECK(restored.nodeName == "yaml-convert-node");
    CHECK(restored.threadPoolSize == 20);
}

// =============================================================================
// Block 2: Edge Cases & Error Invariants
// =============================================================================

TEST_CASE("BaseObject: Edge cases in serialization", "[core][base_obj][edge_cases]")
{
    SECTION("Empty containers and empty strings serialize and deserialize cleanly")
    {
        ComputeNodeConfig emptyConfig;
        emptyConfig.nodeName.clear();
        emptyConfig.scalingFactors.clear();
        emptyConfig.auxiliarySensors.clear();

        std::vector<std::uint8_t> buffer;
        emptyConfig.toBinary(buffer);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(restored.nodeName.empty());
        CHECK(restored.scalingFactors.empty());
        CHECK(restored.auxiliarySensors.empty());
    }

    SECTION("Null smart pointers serialize cleanly and restore as nullptr")
    {
        ComputeNodeConfig config;
        config.auxiliarySensors.push_back(nullptr);
        config.auxiliarySensors.push_back(std::make_shared<SubSensorConfig>());
        config.auxiliarySensors.push_back(nullptr);

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        REQUIRE(restored.auxiliarySensors.size() == 3);
        CHECK(restored.auxiliarySensors[0] == nullptr);
        CHECK(restored.auxiliarySensors[1] != nullptr);
        CHECK(restored.auxiliarySensors[2] == nullptr);
    }

    SECTION("Binary buffer truncation fails gracefully and sets lastErrorString")
    {
        ComputeNodeConfig config;
        config.nodeName = "truncated-test-node";
        config.scalingFactors = {1.0f, 2.0f, 3.0f};

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);
        REQUIRE(buffer.size() > 10);

        buffer.resize(buffer.size() / 2);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> truncatedSpan(buffer);
        const bool success = restored.fromBinary(truncatedSpan);

        CHECK_FALSE(success);
        CHECK(!restored.lastErrorString.empty());
    }

    SECTION("JSON non-object root fails gracefully")
    {
        ComputeNodeConfig restored;
        const nlohmann::json value = "invalid_scalar_root";

        const bool success = restored.fromJson(value);

        CHECK_FALSE(success);
        CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("object"));
    }

    SECTION("YAML parsing non-map root node fails gracefully")
    {
        ComputeNodeConfig restored;
        YAML::Node scalarNode = YAML::Load("invalid_scalar_root");

        const bool success = restored.fromYaml(scalarNode);

        CHECK_FALSE(success);
        CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("map"));
    }
}

TEST_CASE("BaseObject: Pointer concepts distinguish ownership semantics", "[core][base_obj][edge_cases][smart_pointer]")
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

TEST_CASE("BaseObject: Nested deserialization failures propagate to the parent", "[core][base_obj][edge_cases][nested_error]")
{
    SECTION("JSON nested object failure propagates")
    {
        ComputeNodeConfig config;
        nlohmann::json serializedJson = config.toJson();

        serializedJson["primarySensor"]["sampleRateHz"] = "not-a-float";

        ComputeNodeConfig restored;
        const bool success = restored.fromJson(serializedJson);

        CHECK_FALSE(success);
        CHECK(!restored.lastErrorString.empty());
    }

    SECTION("YAML nested object failure propagates")
    {
        ComputeNodeConfig config;
        YAML::Node yamlNode = config.toYaml();

        yamlNode["primarySensor"]["sampleRateHz"] = "not-a-float";

        ComputeNodeConfig restored;
        const bool success = restored.fromYaml(yamlNode);

        CHECK_FALSE(success);
        CHECK(!restored.lastErrorString.empty());
    }

    SECTION("Binary nested object failure propagates")
    {
        ComputeNodeConfig config;
        config.nodeName = "nested-binary-failure";
        config.primarySensor.sensorTag = "nested";

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);
        REQUIRE(buffer.size() > 1);

        buffer.pop_back();

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        const bool success = restored.fromBinary(streamSpan);

        CHECK_FALSE(success);
        CHECK(!restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: Missing JSON and YAML members preserve existing values", "[core][base_obj][edge_cases][missing_fields]")
{
    SECTION("JSON missing members remain unchanged")
    {
        ComputeNodeConfig restored;
        restored.nodeName = "existing-node";
        restored.threadPoolSize = 777;

        nlohmann::json json = nlohmann::json::object();
        json["nodeName"] = "updated-node";

        REQUIRE(restored.fromJson(json));

        CHECK(restored.nodeName == "updated-node");
        CHECK(restored.threadPoolSize == 777);
    }

    SECTION("YAML missing members remain unchanged")
    {
        ComputeNodeConfig restored;
        restored.nodeName = "existing-node";
        restored.threadPoolSize = 888;

        YAML::Node node(YAML::NodeType::Map);
        node["nodeName"] = "updated-node";

        REQUIRE(restored.fromYaml(node));

        CHECK(restored.nodeName == "updated-node");
        CHECK(restored.threadPoolSize == 888);
    }
}

TEST_CASE("BaseObject: Null smart pointers roundtrip across supported formats", "[core][base_obj][edge_cases][null_pointer]")
{
    SECTION("JSON")
    {
        ComputeNodeConfig config;
        config.auxiliarySensors = {nullptr};

        const nlohmann::json serialized = config.toJson();

        ComputeNodeConfig restored;
        REQUIRE(restored.fromJson(serialized));

        REQUIRE(restored.auxiliarySensors.size() == 1);
        CHECK(restored.auxiliarySensors[0] == nullptr);
    }

    SECTION("YAML")
    {
        ComputeNodeConfig config;
        config.auxiliarySensors = {nullptr};

        const YAML::Node serialized = config.toYaml();

        ComputeNodeConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        REQUIRE(restored.auxiliarySensors.size() == 1);
        CHECK(restored.auxiliarySensors[0] == nullptr);
    }

    SECTION("Binary")
    {
        ComputeNodeConfig config;
        config.auxiliarySensors = {nullptr};

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        REQUIRE(restored.auxiliarySensors.size() == 1);
        CHECK(restored.auxiliarySensors[0] == nullptr);
    }
}

TEST_CASE("BaseObject: Null optionals reset existing values across supported formats", "[core][base_obj][edge_cases][optional][null]")
{
    SECTION("JSON")
    {
        OptionalConfig restored;
        restored.optionalValue = 777;

        nlohmann::json serialized = restored.toJson();
        serialized["optionalValue"] = nullptr;

        REQUIRE(restored.fromJson(serialized));
        CHECK_FALSE(restored.optionalValue.has_value());
    }

    SECTION("YAML")
    {
        OptionalConfig restored;
        restored.optionalValue = 777;

        YAML::Node serialized = restored.toYaml();
        serialized["optionalValue"] = YAML::Node(YAML::NodeType::Null);

        REQUIRE(restored.fromYaml(serialized));
        CHECK_FALSE(restored.optionalValue.has_value());
    }

    SECTION("Binary")
    {
        OptionalConfig source;
        source.optionalValue.reset();

        std::vector<std::uint8_t> buffer;
        source.toBinary(buffer);

        OptionalConfig restored;
        restored.optionalValue = 777;

        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK_FALSE(restored.optionalValue.has_value());
    }
}

TEST_CASE("BaseObject: Primitive special types roundtrip", "[core][base_obj][edge_cases][primitive]")
{
    PrimitiveConfig config;
    config.mode = DeviceMode::Suspended;
    config.rawByte = std::byte{0xA5};
    config.wideChar = L'Z';
    config.utf8Char = u8'Y';
    config.utf16Char = u'X';
    config.utf32Char = U'W';

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        PrimitiveConfig restored;
        REQUIRE(restored.fromJson(serialized));

        CHECK(restored.mode == DeviceMode::Suspended);
        CHECK(restored.rawByte == std::byte{0xA5});
        CHECK(restored.wideChar == L'Z');
        CHECK(restored.utf8Char == u8'Y');
        CHECK(restored.utf16Char == u'X');
        CHECK(restored.utf32Char == U'W');
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        PrimitiveConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        CHECK(restored.mode == DeviceMode::Suspended);
        CHECK(restored.rawByte == std::byte{0xA5});
        CHECK(restored.wideChar == L'Z');
        CHECK(restored.utf8Char == u8'Y');
        CHECK(restored.utf16Char == u'X');
        CHECK(restored.utf32Char == U'W');
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        PrimitiveConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.mode == DeviceMode::Suspended);
        CHECK(restored.rawByte == std::byte{0xA5});
        CHECK(restored.wideChar == L'Z');
        CHECK(restored.utf8Char == u8'Y');
        CHECK(restored.utf16Char == u'X');
        CHECK(restored.utf32Char == U'W');
    }
}

TEST_CASE("BaseObject: Vector containers roundtrip at useful boundaries", "[core][base_obj][edge_cases][container][vector]")
{
    SECTION("Empty containers")
    {
        ContainerConfig config;

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ContainerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(restored.values.empty());
        CHECK(restored.names.empty());
    }

    SECTION("Single element containers")
    {
        ContainerConfig config;
        config.values = {42};
        config.names = {"single"};

        const nlohmann::json json = config.toJson();

        ContainerConfig restored;
        REQUIRE(restored.fromJson(json));

        CHECK(restored.values == std::vector<std::int32_t>{42});
        CHECK(restored.names == std::vector<std::string>{"single"});
    }

    SECTION("Many element containers")
    {
        ContainerConfig config;

        for (std::int32_t i = 0; i < 1024; ++i)
            config.values.push_back(i);

        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ContainerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        REQUIRE(restored.values.size() == 1024);
        CHECK(restored.values.front() == 0);
        CHECK(restored.values.back() == 1023);
    }
}

TEST_CASE("BaseObject: Fixed arrays roundtrip across supported formats", "[core][base_obj][edge_cases][container][array]")
{
    FixedContainerConfig config;
    config.values = {9, 8, 7, 6};

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        FixedContainerConfig restored;
        REQUIRE(restored.fromJson(serialized));
        CHECK(restored.values == config.values);
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        FixedContainerConfig restored;
        REQUIRE(restored.fromYaml(serialized));
        CHECK(restored.values == config.values);
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        FixedContainerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.values == config.values);
    }
}

TEST_CASE("BaseObject: Fixed arrays reject size mismatches", "[core][base_obj][edge_cases][container][array][invalid]")
{
    SECTION("JSON")
    {
        FixedContainerConfig restored;
        nlohmann::json serialized = nlohmann::json::object();
        serialized["values"] = nlohmann::json::array({1, 2, 3});

        CHECK_FALSE(restored.fromJson(serialized));
        CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("size"));
    }

    SECTION("YAML")
    {
        FixedContainerConfig restored;
        YAML::Node serialized(YAML::NodeType::Map);
        serialized["values"].push_back(1);
        serialized["values"].push_back(2);
        serialized["values"].push_back(3);

        CHECK_FALSE(restored.fromYaml(serialized));
        CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("size"));
    }
}

TEST_CASE("BaseObject: Set containers roundtrip across supported formats", "[core][base_obj][edge_cases][container][set]")
{
    SetConfig config;
    config.values = {5, 1, 9, 5};
    config.names = {"beta", "alpha", "gamma"};

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        SetConfig restored;
        REQUIRE(restored.fromJson(serialized));

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        SetConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        SetConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }
}

TEST_CASE("BaseObject: Map containers roundtrip across supported formats", "[core][base_obj][edge_cases][container][map]")
{
    MapConfig config;
    config.entries = {
        {1, "cpu"},
        {2, "cuda"},
        {3, "vulkan"}
    };

    SubSensorConfig cpu;
    cpu.sensorTag = "cpu";
    cpu.sampleRateHz = 100.0f;

    SubSensorConfig gpu;
    gpu.sensorTag = "gpu";
    gpu.sampleRateHz = 200.0f;

    config.sensors.emplace("cpu", cpu);
    config.sensors.emplace("gpu", gpu);

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        REQUIRE(serialized["entries"].is_array());
        REQUIRE(serialized["sensors"].is_array());

        MapConfig restored;
        REQUIRE(restored.fromJson(serialized));

        CHECK(restored.entries == config.entries);
        REQUIRE(restored.sensors.size() == 2);
        CHECK(restored.sensors.at("cpu").sensorTag == "cpu");
        CHECK(restored.sensors.at("gpu").sampleRateHz == 200.0f);
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        REQUIRE(serialized["entries"].IsSequence());
        REQUIRE(serialized["sensors"].IsSequence());

        MapConfig restored;
        REQUIRE(restored.fromYaml(serialized));

        CHECK(restored.entries == config.entries);
        REQUIRE(restored.sensors.size() == 2);
        CHECK(restored.sensors.at("cpu").sensorTag == "cpu");
        CHECK(restored.sensors.at("gpu").sampleRateHz == 200.0f);
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        MapConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.entries == config.entries);
        REQUIRE(restored.sensors.size() == 2);
        CHECK(restored.sensors.at("cpu").sensorTag == "cpu");
        CHECK(restored.sensors.at("gpu").sampleRateHz == 200.0f);
    }
}

TEST_CASE("BaseObject: Legacy BinaryMapConfig remains valid across all formats", "[core][base_obj][edge_cases][map]")
{
    BinaryMapConfig config;
    config.entries = {
        {1, "cpu"},
        {2, "cuda"},
        {3, "vulkan"}
    };

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        BinaryMapConfig restored;
        REQUIRE(restored.fromJson(serialized));
        CHECK(restored.entries == config.entries);
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        BinaryMapConfig restored;
        REQUIRE(restored.fromYaml(serialized));
        CHECK(restored.entries == config.entries);
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        BinaryMapConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.entries == config.entries);
    }
}

TEST_CASE("BaseObject: Nested container combinations roundtrip", "[core][base_obj][edge_cases][container][nested]")
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

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        NestedContainerConfig restored;
        REQUIRE(restored.fromJson(serialized));

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
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        NestedContainerConfig restored;
        REQUIRE(restored.fromYaml(serialized));

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
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        NestedContainerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
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
    }
}

TEST_CASE("BaseObject: NoSerialize annotations exclude members from persistence", "[core][base_obj][edge_cases][annotation]")
{
    AnnotationConfig config;
    config.serializedValue = "persistent";
    config.transientValue = "transient-secret";
    config.noResetValue = "still-serialized";
    config.runtimeValue = "runtime-secret";

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        CHECK(serialized.contains("serializedValue"));
        CHECK(serialized.contains("noResetValue"));
        CHECK_FALSE(serialized.contains("transientValue"));
        CHECK_FALSE(serialized.contains("runtimeValue"));
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        CHECK(static_cast<bool>(serialized["serializedValue"]));
        CHECK(static_cast<bool>(serialized["noResetValue"]));
        CHECK_FALSE(static_cast<bool>(serialized["transientValue"]));
        CHECK_FALSE(static_cast<bool>(serialized["runtimeValue"]));
    }

    SECTION("Binary")
    {
        AnnotationConfig withTransient;
        withTransient.serializedValue = "persistent";
        withTransient.noResetValue = "still-serialized";
        withTransient.transientValue = "one";
        withTransient.runtimeValue = "two";

        AnnotationConfig differentTransient;
        differentTransient.serializedValue = "persistent";
        differentTransient.noResetValue = "still-serialized";
        differentTransient.transientValue = "completely-different";
        differentTransient.runtimeValue = "also-different";

        std::vector<std::uint8_t> first;
        std::vector<std::uint8_t> second;
        withTransient.toBinary(first);
        differentTransient.toBinary(second);

        CHECK(first == second);
    }
}

TEST_CASE("BaseObject: NoSerialize members are preserved during deserialization", "[core][base_obj][edge_cases][annotation][deserialize]")
{
    AnnotationConfig source;
    source.serializedValue = "serialized-new";
    source.noResetValue = "no-reset-new";

    SECTION("JSON")
    {
        const nlohmann::json serialized = source.toJson();

        AnnotationConfig restored;
        restored.transientValue = "keep-transient";
        restored.runtimeValue = "keep-runtime";

        REQUIRE(restored.fromJson(serialized));

        CHECK(restored.serializedValue == "serialized-new");
        CHECK(restored.noResetValue == "no-reset-new");
        CHECK(restored.transientValue == "keep-transient");
        CHECK(restored.runtimeValue == "keep-runtime");
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = source.toYaml();

        AnnotationConfig restored;
        restored.transientValue = "keep-transient";
        restored.runtimeValue = "keep-runtime";

        REQUIRE(restored.fromYaml(serialized));

        CHECK(restored.serializedValue == "serialized-new");
        CHECK(restored.noResetValue == "no-reset-new");
        CHECK(restored.transientValue == "keep-transient");
        CHECK(restored.runtimeValue == "keep-runtime");
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> buffer;
        source.toBinary(buffer);

        AnnotationConfig restored;
        restored.transientValue = "keep-transient";
        restored.runtimeValue = "keep-runtime";

        std::span<const std::uint8_t> streamSpan(buffer);
        REQUIRE(restored.fromBinary(streamSpan));

        CHECK(streamSpan.empty());
        CHECK(restored.serializedValue == "serialized-new");
        CHECK(restored.noResetValue == "no-reset-new");
        CHECK(restored.transientValue == "keep-transient");
        CHECK(restored.runtimeValue == "keep-runtime");
    }
}

TEST_CASE("BaseObject: lastErrorString is runtime state and is not serialized", "[core][base_obj][edge_cases][error_state]")
{
    ErrorStateConfig config;
    config.value = "real-data";
    config.lastErrorString = "this must not serialize";

    SECTION("JSON")
    {
        const nlohmann::json serialized = config.toJson();

        REQUIRE(serialized.contains("value"));
        CHECK_FALSE(serialized.contains("lastErrorString"));
    }

    SECTION("YAML")
    {
        const YAML::Node serialized = config.toYaml();

        REQUIRE(serialized["value"]);
        CHECK_FALSE(serialized["lastErrorString"]);
    }

    SECTION("Binary")
    {
        std::vector<std::uint8_t> withError;
        config.toBinary(withError);

        ErrorStateConfig cleanConfig;
        cleanConfig.value = "real-data";

        std::vector<std::uint8_t> withoutError;
        cleanConfig.toBinary(withoutError);

        CHECK(withError == withoutError);
    }
}

TEST_CASE("BaseObject: Invalid JSON member types fail gracefully", "[core][base_obj][edge_cases][json][invalid]")
{
    ComputeNodeConfig config;
    nlohmann::json serialized = config.toJson();

    serialized["threadPoolSize"] = "not-an-integer";

    ComputeNodeConfig restored;
    const bool success = restored.fromJson(serialized);

    CHECK_FALSE(success);
    CHECK(!restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Invalid YAML member types fail gracefully", "[core][base_obj][edge_cases][yaml][invalid]")
{
    ComputeNodeConfig config;
    YAML::Node serialized = config.toYaml();

    serialized["threadPoolSize"] = "not-an-integer";

    ComputeNodeConfig restored;
    const bool success = restored.fromYaml(serialized);

    CHECK_FALSE(success);
    CHECK(!restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Empty binary input fails gracefully", "[core][base_obj][edge_cases][binary][empty]")
{
    ComputeNodeConfig restored;

    const std::vector<std::uint8_t> buffer;
    std::span<const std::uint8_t> streamSpan(buffer);

    const bool success = restored.fromBinary(streamSpan);

    CHECK_FALSE(success);
    CHECK(!restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: Binary truncation at multiple boundaries fails gracefully", "[core][base_obj][edge_cases][binary][truncated]")
{
    ComputeNodeConfig config;
    config.nodeName = "boundary-test";
    config.scalingFactors = {1.0f, 2.0f, 3.0f, 4.0f};

    std::vector<std::uint8_t> completeBuffer;
    config.toBinary(completeBuffer);

    REQUIRE(completeBuffer.size() > 8);

    const std::vector<std::size_t> sizes = {
        0,
        1,
        completeBuffer.size() / 4,
        completeBuffer.size() / 2,
        completeBuffer.size() - 1
    };

    for (const std::size_t size : sizes) {
        std::vector<std::uint8_t> truncated(completeBuffer.begin(), completeBuffer.begin() + size);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(truncated);

        INFO("Truncated buffer size: " << size);
        CHECK_FALSE(restored.fromBinary(streamSpan));
        CHECK(!restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: File loading failures populate lastErrorString", "[core][base_obj][edge_cases][file][failure]")
{
    SECTION("Missing JSON file")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.json");
        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromJsonFile(fileName.string()));
        CHECK(!restored.lastErrorString.empty());
    }

    SECTION("Missing YAML file")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.yaml");
        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromYamlFile(fileName.string()));
        CHECK(!restored.lastErrorString.empty());
    }

    SECTION("Missing binary file")
    {
        ComputeNodeConfig restored;
        const auto fileName = tempFilePath("job_core_missing_base_obj_file.bin");
        std::filesystem::remove(fileName);

        CHECK_FALSE(restored.loadFromBinaryFile(fileName.string()));
        CHECK(!restored.lastErrorString.empty());
    }
}

TEST_CASE("BaseObject: Malformed serialized files fail gracefully", "[core][base_obj][edge_cases][file][malformed]")
{
    SECTION("Malformed JSON file")
    {
        const auto fileName = tempFilePath("job_core_malformed_base_obj.json");

        {
            std::ofstream file(fileName);
            REQUIRE(file.is_open());
            file << "{ definitely-not-valid-json";
        }

        ComputeNodeConfig restored;
        CHECK_FALSE(restored.loadFromJsonFile(fileName.string()));
        CHECK(!restored.lastErrorString.empty());

        std::filesystem::remove(fileName);
    }

    SECTION("Malformed YAML file")
    {
        const auto fileName = tempFilePath("job_core_malformed_base_obj.yaml");

        {
            std::ofstream file(fileName);
            REQUIRE(file.is_open());
            file << "root: [unterminated";
        }

        ComputeNodeConfig restored;
        CHECK_FALSE(restored.loadFromYamlFile(fileName.string()));
        CHECK(!restored.lastErrorString.empty());

        std::filesystem::remove(fileName);
    }
}

TEST_CASE("BaseObject: Failed deserialization documents partial mutation semantics", "[core][base_obj][edge_cases][failure_state]")
{
    ComputeNodeConfig restored;
    restored.nodeName = "before";
    restored.threadPoolSize = 999;

    nlohmann::json serialized = restored.toJson();
    serialized["nodeName"] = "after";
    serialized["primarySensor"]["sampleRateHz"] = "broken";

    const bool success = restored.fromJson(serialized);

    REQUIRE_FALSE(success);

    // Deserialization currently mutates members in reflection order.
    // A later failure does not roll back fields already written.
    CHECK(restored.nodeName == "after");
    CHECK(!restored.lastErrorString.empty());
}



// =============================================================================
// JobYaml challenger coverage
// =============================================================================
TEST_CASE("BaseObject: JobYaml floating scalar spellings",
          "[core][base_obj][job_yaml][scalar][float]")
{
    float value = 0.0f;

    CHECK(job::yaml::YamlSink::scalar(value, "250"));
    CHECK(value == 250.0f);

    CHECK(job::yaml::YamlSink::scalar(value, "250.0"));
    CHECK(value == 250.0f);
}

TEST_CASE("BaseObject: JobYaml boolean scalar spellings",
          "[core][base_obj][job_yaml][scalar][bool]")
{
    bool value = true;

    REQUIRE(job::yaml::YamlSink::scalar(value, "false"));
    CHECK_FALSE(value);

    REQUIRE(job::yaml::YamlSink::scalar(value, "true"));
    CHECK(value);
}



TEST_CASE("BaseObject: JobYaml direct nested object roundtrip",
          "[core][base_obj][job_yaml][nested][debug]")
{
    SubSensorConfig config;
    config.sensorTag = "direct-sensor";
    config.sampleRateHz = 250.0f;
    config.calibrateOnBoot = false;
    config.initialMode = DeviceMode::Compute;

    const job::yaml::YamlNode serialized = config.toJobYaml();

    REQUIRE(serialized.isMapping());

    const auto *sensorTag = serialized.member("sensorTag");
    REQUIRE(sensorTag != nullptr);
    REQUIRE(sensorTag->isScalar());
    CHECK(sensorTag->scalar() == "direct-sensor");

    const auto *sampleRateHz = serialized.member("sampleRateHz");
    REQUIRE(sampleRateHz != nullptr);
    REQUIRE(sampleRateHz->isScalar());

    const auto *calibrateOnBoot = serialized.member("calibrateOnBoot");
    REQUIRE(calibrateOnBoot != nullptr);
    REQUIRE(calibrateOnBoot->isScalar());

    const auto *initialMode = serialized.member("initialMode");
    REQUIRE(initialMode != nullptr);
    REQUIRE(initialMode->isScalar());

    WARN("sensorTag=" << sensorTag->scalar());
    WARN("sampleRateHz=" << sampleRateHz->scalar());
    WARN("calibrateOnBoot=" << calibrateOnBoot->scalar());
    WARN("initialMode=" << initialMode->scalar());

    SubSensorConfig restored;
    const bool success = restored.fromJobYaml(serialized);

    WARN("lastErrorString=" << restored.lastErrorString);
    REQUIRE(success);

    CHECK(restored.sensorTag == config.sensorTag);
    CHECK(restored.sampleRateHz == config.sampleRateHz);
    CHECK(restored.calibrateOnBoot == config.calibrateOnBoot);
    CHECK(restored.initialMode == config.initialMode);
}




TEST_CASE("BaseObject: JobYaml serialization roundtrip with nested objects and smart pointers",
          "[core][base_obj][job_yaml][example]")
{
    ComputeNodeConfig config;
    config.nodeName = "job-yaml-compute-node";
    config.threadPoolSize = 64;
    config.memoryBudgetGb = 128.0;
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

    const job::yaml::YamlNode serialized = config.toJobYaml();

    REQUIRE(serialized.isMapping());

    const auto *nodeName = serialized.member("nodeName");
    REQUIRE(nodeName != nullptr);
    REQUIRE(nodeName->isScalar());
    CHECK(nodeName->scalar() == "job-yaml-compute-node");

    const auto *threadPoolSize = serialized.member("threadPoolSize");
    REQUIRE(threadPoolSize != nullptr);
    REQUIRE(threadPoolSize->isScalar());
    CHECK(threadPoolSize->scalar() == "64");

    const auto *sensors = serialized.member("auxiliarySensors");
    REQUIRE(sensors != nullptr);
    REQUIRE(sensors->isSequence());
    REQUIRE(sensors->sequence().size() == 2);

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJobYaml(serialized));

    CHECK(restored.nodeName == config.nodeName);
    CHECK(restored.threadPoolSize == config.threadPoolSize);
    CHECK(restored.memoryBudgetGb == config.memoryBudgetGb);
    CHECK(restored.scalingFactors == config.scalingFactors);
    CHECK(restored.primarySensor.sensorTag == config.primarySensor.sensorTag);
    CHECK(restored.primarySensor.sampleRateHz == config.primarySensor.sampleRateHz);
    CHECK(restored.primarySensor.calibrateOnBoot == config.primarySensor.calibrateOnBoot);
    CHECK(restored.primarySensor.initialMode == config.primarySensor.initialMode);

    REQUIRE(restored.auxiliarySensors.size() == 2);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    REQUIRE(restored.auxiliarySensors[1] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "ambient_0");
    CHECK(restored.auxiliarySensors[0]->sampleRateHz == 10.0f);
    CHECK(restored.auxiliarySensors[1]->sensorTag == "vram_hotspot");
    CHECK(restored.auxiliarySensors[1]->sampleRateHz == 500.0f);
}

TEST_CASE("BaseObject: JobYaml optional and ownership semantics",
          "[core][base_obj][job_yaml][optional][pointer]")
{
    OptionalConfig config;
    config.optionalValue = 42;
    config.optionalName = "optional-name";

    config.optionalSensor.emplace();
    config.optionalSensor->sensorTag = "optional-direct";

    config.optionalSharedSensor = std::make_shared<SubSensorConfig>();
    (*config.optionalSharedSensor)->sensorTag = "optional-shared";

    const job::yaml::YamlNode serialized = config.toJobYaml();

    OptionalConfig restored;
    REQUIRE(restored.fromJobYaml(serialized));

    REQUIRE(restored.optionalValue);
    CHECK(*restored.optionalValue == 42);

    REQUIRE(restored.optionalName);
    CHECK(*restored.optionalName == "optional-name");

    REQUIRE(restored.optionalSensor);
    CHECK(restored.optionalSensor->sensorTag == "optional-direct");

    REQUIRE(restored.optionalSharedSensor);
    REQUIRE(*restored.optionalSharedSensor != nullptr);
    CHECK((*restored.optionalSharedSensor)->sensorTag == "optional-shared");

    OptionalConfig nullSource;
    nullSource.optionalValue.reset();
    nullSource.optionalName.reset();
    nullSource.optionalSensor.reset();
    nullSource.optionalSharedSensor.reset();

    const job::yaml::YamlNode nullSerialized = nullSource.toJobYaml();

    restored.optionalValue = 777;
    restored.optionalName = "must-reset";
    restored.optionalSensor.emplace();
    restored.optionalSharedSensor = std::make_shared<SubSensorConfig>();

    REQUIRE(restored.fromJobYaml(nullSerialized));

    CHECK_FALSE(restored.optionalValue.has_value());
    CHECK_FALSE(restored.optionalName.has_value());
    CHECK_FALSE(restored.optionalSensor.has_value());
    CHECK_FALSE(restored.optionalSharedSensor.has_value());
}

TEST_CASE("BaseObject: JobYaml shared and unique ownership roundtrip",
          "[core][base_obj][job_yaml][pointer]")
{
    PointerConfig config;

    config.sharedSensor = std::make_shared<SubSensorConfig>();
    config.sharedSensor->sensorTag = "shared";

    config.uniqueSensor = std::make_unique<SubSensorConfig>();
    config.uniqueSensor->sensorTag = "unique";

    const job::yaml::YamlNode serialized = config.toJobYaml();

    PointerConfig restored;
    REQUIRE(restored.fromJobYaml(serialized));

    REQUIRE(restored.sharedSensor != nullptr);
    REQUIRE(restored.uniqueSensor != nullptr);
    CHECK(restored.sharedSensor->sensorTag == "shared");
    CHECK(restored.uniqueSensor->sensorTag == "unique");
}

TEST_CASE("BaseObject: JobYaml primitive special types roundtrip",
          "[core][base_obj][job_yaml][primitive]")
{
    PrimitiveConfig config;
    config.mode = DeviceMode::Suspended;
    config.rawByte = std::byte{0xA5};
    config.wideChar = L'Z';
    config.utf8Char = u8'Y';
    config.utf16Char = u'X';
    config.utf32Char = U'W';

    const job::yaml::YamlNode serialized = config.toJobYaml();

    PrimitiveConfig restored;
    REQUIRE(restored.fromJobYaml(serialized));

    CHECK(restored.mode == DeviceMode::Suspended);
    CHECK(restored.rawByte == std::byte{0xA5});
    CHECK(restored.wideChar == L'Z');
    CHECK(restored.utf8Char == u8'Y');
    CHECK(restored.utf16Char == u'X');
    CHECK(restored.utf32Char == U'W');
}

TEST_CASE("BaseObject: JobYaml container semantics",
          "[core][base_obj][job_yaml][container]")
{
    SECTION("Fixed array")
    {
        FixedContainerConfig config;
        config.values = {9, 8, 7, 6};

        const job::yaml::YamlNode serialized = config.toJobYaml();

        FixedContainerConfig restored;
        REQUIRE(restored.fromJobYaml(serialized));
        CHECK(restored.values == config.values);
    }

    SECTION("Set")
    {
        SetConfig config;
        config.values = {5, 1, 9, 5};
        config.names = {"beta", "alpha", "gamma"};

        const job::yaml::YamlNode serialized = config.toJobYaml();

        SetConfig restored;
        REQUIRE(restored.fromJobYaml(serialized));

        CHECK(restored.values == config.values);
        CHECK(restored.names == config.names);
    }

    SECTION("Map")
    {
        MapConfig config;
        config.entries = {
            {1, "cpu"},
            {2, "cuda"},
            {3, "vulkan"}
        };

        SubSensorConfig cpu;
        cpu.sensorTag = "cpu";
        cpu.sampleRateHz = 100.0f;

        SubSensorConfig gpu;
        gpu.sensorTag = "gpu";
        gpu.sampleRateHz = 200.0f;

        config.sensors.emplace("cpu", cpu);
        config.sensors.emplace("gpu", gpu);

        const job::yaml::YamlNode serialized = config.toJobYaml();

        const auto *entries = serialized.member("entries");
        REQUIRE(entries != nullptr);
        REQUIRE(entries->isSequence());
        REQUIRE(entries->sequence().size() == 3);

        for (const auto &entry : entries->sequence()) {
            REQUIRE(entry.isSequence());
            REQUIRE(entry.sequence().size() == 2);
        }

        MapConfig restored;
        REQUIRE(restored.fromJobYaml(serialized));

        CHECK(restored.entries == config.entries);
        REQUIRE(restored.sensors.size() == 2);
        CHECK(restored.sensors.at("cpu").sensorTag == "cpu");
        CHECK(restored.sensors.at("gpu").sampleRateHz == 200.0f);
    }
}

TEST_CASE("BaseObject: JobYaml nested container combinations roundtrip",
          "[core][base_obj][job_yaml][container][nested]")
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

    const job::yaml::YamlNode serialized = config.toJobYaml();

    NestedContainerConfig restored;
    REQUIRE(restored.fromJobYaml(serialized));

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
}

TEST_CASE("BaseObject: JobYaml NoSerialize annotations preserve persistence semantics",
          "[core][base_obj][job_yaml][annotation]")
{
    AnnotationConfig config;
    config.serializedValue = "persistent";
    config.transientValue = "transient-secret";
    config.noResetValue = "still-serialized";
    config.runtimeValue = "runtime-secret";

    const job::yaml::YamlNode serialized = config.toJobYaml();

    CHECK(serialized.member("serializedValue") != nullptr);
    CHECK(serialized.member("noResetValue") != nullptr);
    CHECK(serialized.member("transientValue") == nullptr);
    CHECK(serialized.member("runtimeValue") == nullptr);

    AnnotationConfig restored;
    restored.transientValue = "keep-transient";
    restored.runtimeValue = "keep-runtime";

    REQUIRE(restored.fromJobYaml(serialized));

    CHECK(restored.serializedValue == "persistent");
    CHECK(restored.noResetValue == "still-serialized");
    CHECK(restored.transientValue == "keep-transient");
    CHECK(restored.runtimeValue == "keep-runtime");
}

TEST_CASE("BaseObject: JobYaml missing members preserve existing values",
          "[core][base_obj][job_yaml][missing_fields]")
{
    job::yaml::YamlNode serialized;
    serialized.setMapping();
    serialized.setMemberScalar("nodeName", "updated-node");

    ComputeNodeConfig restored;
    restored.nodeName = "existing-node";
    restored.threadPoolSize = 888;

    REQUIRE(restored.fromJobYaml(serialized));

    CHECK(restored.nodeName == "updated-node");
    CHECK(restored.threadPoolSize == 888);
}

TEST_CASE("BaseObject: JobYaml invalid scalar member fails gracefully",
          "[core][base_obj][job_yaml][invalid]")
{
    ComputeNodeConfig config;
    job::yaml::YamlNode serialized = config.toJobYaml();

    auto *threadPoolSize = serialized.member("threadPoolSize");
    REQUIRE(threadPoolSize != nullptr);

    threadPoolSize->setScalar("not-an-integer");

    ComputeNodeConfig restored;
    const bool success = restored.fromJobYaml(serialized);

    CHECK_FALSE(success);
    CHECK(!restored.lastErrorString.empty());
}

TEST_CASE("BaseObject: JobYaml non-mapping root fails gracefully",
          "[core][base_obj][job_yaml][invalid][root]")
{
    job::yaml::YamlNode node{"invalid_scalar_root"};

    ComputeNodeConfig restored;

    CHECK_FALSE(restored.fromJobYaml(node));
    CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("mapping"));
}

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

    std::string serializedJson;
    REQUIRE(config.toJobJson(serializedJson));
    REQUIRE_FALSE(serializedJson.empty());

    ComputeNodeConfig restored;
    REQUIRE(restored.fromJobJson(serializedJson));

    CHECK(restored.nodeName == "edge-inference-01");
    CHECK(restored.threadPoolSize == 32);
    CHECK(restored.memoryBudgetGb == 64.0);
    CHECK(restored.scalingFactors == std::vector<float>{1.25f, 2.5f, 5.0f});
    CHECK(restored.primarySensor.sensorTag == "core_temp");
    CHECK(restored.primarySensor.sampleRateHz == 250.0f);
    CHECK(restored.primarySensor.calibrateOnBoot == false);
    CHECK(restored.primarySensor.initialMode == DeviceMode::Compute);

    REQUIRE(restored.auxiliarySensors.size() == 2);
    REQUIRE(restored.auxiliarySensors[0] != nullptr);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "ambient_0");
    CHECK(restored.auxiliarySensors[0]->sampleRateHz == 10.0f);
    REQUIRE(restored.auxiliarySensors[1] != nullptr);
    CHECK(restored.auxiliarySensors[1]->sensorTag == "vram_hotspot");
    CHECK(restored.auxiliarySensors[1]->sampleRateHz == 500.0f);
}




// =============================================================================
// Block 3: Benchmarks / Stress
// =============================================================================
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("BaseObject serialization benchmarks", "[core][base_obj][benchmark]")
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

    std::vector<std::uint8_t> binaryBuffer;
    config.toBinary(binaryBuffer);

    std::string jsonPayload;
    REQUIRE(config.toJobJson(jsonPayload));

    const YAML::Node yamlPayload = config.toYaml();
    const job::yaml::YamlNode jobYamlPayload = config.toJobYaml();

    BENCHMARK("Binary Serialization (toBinary)")
    {
        std::vector<std::uint8_t> buf;
        config.toBinary(buf);
        return buf.size();
    };

    BENCHMARK("Binary Deserialization (fromBinary)")
    {
        ComputeNodeConfig restored;
        std::span<const std::uint8_t> span(binaryBuffer);

        if (!restored.fromBinary(span))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("nlohmann JSON Serialization")
    {
        return config.toJson().dump();
    };

    BENCHMARK("nlohmann JSON Deserialization")
    {
        const nlohmann::json json = nlohmann::json::parse(jsonPayload);

        ComputeNodeConfig restored;
        restored.fromJson(json);

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("JobJson Serialization")
    {
        std::string output;
        config.toJobJson(output);
        return output;
    };

    BENCHMARK("JobJson Deserialization")
    {
        ComputeNodeConfig restored;
        restored.fromJobJson(jsonPayload);

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("YAML-cpp Serialization (toYaml)")
    {
        return config.toYaml();
    };

    BENCHMARK("YAML-cpp Deserialization (fromYaml)")
    {
        ComputeNodeConfig restored;

        if (!restored.fromYaml(yamlPayload))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("JobYaml Serialization (toJobYaml)")
    {
        return config.toJobYaml();
    };

    BENCHMARK("JobYaml Deserialization (fromJobYaml)")
    {
        ComputeNodeConfig restored;

        if (!restored.fromJobYaml(jobYamlPayload))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };
}

TEST_CASE("BaseObject nested serialization stress benchmark", "[core][base_obj][benchmark][stress]")
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

    BENCHMARK("Large nested binary roundtrip")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        ComputeNodeConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);

        if (!restored.fromBinary(streamSpan))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Nlohmann Large nested JSON roundtrip")
    {
        const std::string json = config.toJson().dump();

        const nlohmann::json parsed = nlohmann::json::parse(json);

        ComputeNodeConfig restored;
        restored.fromJson(parsed);

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("JobJson Large nested JSON roundtrip")
    {
        std::string json;
        config.toJobJson(json);

        ComputeNodeConfig restored;
        restored.fromJobJson(json);

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested YAML-cpp roundtrip")
    {
        const YAML::Node yaml = config.toYaml();

        ComputeNodeConfig restored;

        if (!restored.fromYaml(yaml))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };

    BENCHMARK("Large nested JobYaml roundtrip")
    {
        const job::yaml::YamlNode yaml = config.toJobYaml();

        ComputeNodeConfig restored;

        if (!restored.fromJobYaml(yaml))
            return std::size_t{0};

        return restored.scalingFactors.size() +
               restored.auxiliarySensors.size() +
               restored.threadPoolSize;
    };
}

TEST_CASE("BaseObject mixed container stress benchmark", "[core][base_obj][benchmark][container][stress]")
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

    BENCHMARK("Mixed nested binary roundtrip")
    {
        std::vector<std::uint8_t> buffer;
        config.toBinary(buffer);

        NestedContainerConfig restored;
        std::span<const std::uint8_t> streamSpan(buffer);

        if (!restored.fromBinary(streamSpan))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Nlohmann Mixed nested JSON roundtrip")
    {
        const std::string json = config.toJson().dump();
        const nlohmann::json parsed = nlohmann::json::parse(json);

        NestedContainerConfig restored;
        restored.fromJson(parsed);

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("JobJson Mixed nested JSON roundtrip")
    {
        std::string json;
        config.toJobJson(json);

        NestedContainerConfig restored;
        restored.fromJobJson(json);

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested YAML-cpp roundtrip")
    {
        const YAML::Node yaml = config.toYaml();

        NestedContainerConfig restored;

        if (!restored.fromYaml(yaml))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };

    BENCHMARK("Mixed nested JobYaml roundtrip")
    {
        const job::yaml::YamlNode yaml = config.toJobYaml();

        NestedContainerConfig restored;

        if (!restored.fromJobYaml(yaml))
            return std::size_t{0};

        return restored.optionalSensors.size() +
               restored.sharedSensors.size() +
               restored.namedSensors.size();
    };
}

#endif





// // =============================================================================
// // Block 3: Benchmarks / Stress
// // =============================================================================
// #ifdef JOB_TEST_BENCHMARKS
// TEST_CASE("BaseObject serialization benchmarks", "[core][base_obj][benchmark]")
// {
//     ComputeNodeConfig config;
//     config.nodeName = "benchmark-compute-node";
//     config.threadPoolSize = 64;
//     config.memoryBudgetGb = 128.0;
//     config.scalingFactors.resize(256, 1.0f);

//     for (int i = 0; i < 16; ++i) {
//         auto sensor = std::make_shared<SubSensorConfig>();
//         sensor->sensorTag = "sensor_channel_" + std::to_string(i);
//         sensor->sampleRateHz = static_cast<float>(1000.0 / (i + 1));
//         config.auxiliarySensors.push_back(std::move(sensor));
//     }

//     std::vector<std::uint8_t> binaryBuffer;
//     config.toBinary(binaryBuffer);

//     const nlohmann::json jsonPayload = config.toJson();
//     const YAML::Node yamlPayload = config.toYaml();
//     const job::yaml::YamlNode jobYamlPayload = config.toJobYaml();

//     BENCHMARK("Binary Serialization (toBinary)")
//     {
//         std::vector<std::uint8_t> buf;
//         config.toBinary(buf);
//         return buf.size();
//     };

//     BENCHMARK("Binary Deserialization (fromBinary)")
//     {
//         ComputeNodeConfig restored;
//         std::span<const std::uint8_t> span(binaryBuffer);

//         if (!restored.fromBinary(span))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("JSON Serialization (toJson)")
//     {
//         return config.toJson();
//     };

//     BENCHMARK("JSON Deserialization (fromJson)")
//     {
//         ComputeNodeConfig restored;

//         if (!restored.fromJson(jsonPayload))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("YAML-cpp Serialization (toYaml)")
//     {
//         return config.toYaml();
//     };

//     BENCHMARK("YAML-cpp Deserialization (fromYaml)")
//     {
//         ComputeNodeConfig restored;

//         if (!restored.fromYaml(yamlPayload))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("JobYaml Serialization (toJobYaml)")
//     {
//         return config.toJobYaml();
//     };

//     BENCHMARK("JobYaml Deserialization (fromJobYaml)")
//     {
//         ComputeNodeConfig restored;

//         if (!restored.fromJobYaml(jobYamlPayload))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };
// }
// TEST_CASE("BaseObject nested serialization stress benchmark", "[core][base_obj][benchmark][stress]")
// {
//     ComputeNodeConfig config;
//     config.nodeName = "nested-stress-node";
//     config.threadPoolSize = 128;
//     config.memoryBudgetGb = 256.0;
//     config.scalingFactors.resize(4096, 0.5f);

//     for (int i = 0; i < 256; ++i) {
//         auto sensor = std::make_shared<SubSensorConfig>();
//         sensor->sensorTag = "stress_sensor_" + std::to_string(i);
//         sensor->sampleRateHz = static_cast<float>(i + 1);
//         sensor->calibrateOnBoot = (i % 2) == 0;
//         sensor->initialMode = (i % 3) == 0 ? DeviceMode::Compute : DeviceMode::Idle;
//         config.auxiliarySensors.push_back(std::move(sensor));
//     }

//     BENCHMARK("Large nested binary roundtrip")
//     {
//         std::vector<std::uint8_t> buffer;
//         config.toBinary(buffer);

//         ComputeNodeConfig restored;
//         std::span<const std::uint8_t> streamSpan(buffer);

//         if (!restored.fromBinary(streamSpan))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("Large nested JSON roundtrip")
//     {
//         const nlohmann::json json = config.toJson();

//         ComputeNodeConfig restored;

//         if (!restored.fromJson(json))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("Large nested YAML-cpp roundtrip")
//     {
//         const YAML::Node yaml = config.toYaml();

//         ComputeNodeConfig restored;

//         if (!restored.fromYaml(yaml))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };

//     BENCHMARK("Large nested JobYaml roundtrip")
//     {
//         const job::yaml::YamlNode yaml = config.toJobYaml();

//         ComputeNodeConfig restored;

//         if (!restored.fromJobYaml(yaml))
//             return std::size_t{0};

//         return restored.scalingFactors.size() +
//                restored.auxiliarySensors.size() +
//                restored.threadPoolSize;
//     };
// }

// TEST_CASE("BaseObject mixed container stress benchmark", "[core][base_obj][benchmark][container][stress]")
// {
//     NestedContainerConfig config;

//     for (int i = 0; i < 256; ++i) {
//         SubSensorConfig direct;
//         direct.sensorTag = "optional_" + std::to_string(i);
//         direct.sampleRateHz = static_cast<float>(i + 1);
//         config.optionalSensors.emplace_back(std::move(direct));

//         auto shared = std::make_shared<SubSensorConfig>();
//         shared->sensorTag = "shared_" + std::to_string(i);
//         config.sharedSensors.push_back(shared);
//         config.namedSensors.emplace("sensor_" + std::to_string(i), std::move(shared));
//     }

//     BENCHMARK("Mixed nested binary roundtrip")
//     {
//         std::vector<std::uint8_t> buffer;
//         config.toBinary(buffer);

//         NestedContainerConfig restored;
//         std::span<const std::uint8_t> streamSpan(buffer);

//         if (!restored.fromBinary(streamSpan))
//             return std::size_t{0};

//         return restored.optionalSensors.size() +
//                restored.sharedSensors.size() +
//                restored.namedSensors.size();
//     };

//     BENCHMARK("Mixed nested JSON roundtrip")
//     {
//         const nlohmann::json json = config.toJson();

//         NestedContainerConfig restored;

//         if (!restored.fromJson(json))
//             return std::size_t{0};

//         return restored.optionalSensors.size() +
//                restored.sharedSensors.size() +
//                restored.namedSensors.size();
//     };

//     BENCHMARK("Mixed nested YAML-cpp roundtrip")
//     {
//         const YAML::Node yaml = config.toYaml();

//         NestedContainerConfig restored;

//         if (!restored.fromYaml(yaml))
//             return std::size_t{0};

//         return restored.optionalSensors.size() +
//                restored.sharedSensors.size() +
//                restored.namedSensors.size();
//     };

//     BENCHMARK("Mixed nested JobYaml roundtrip")
//     {
//         const job::yaml::YamlNode yaml = config.toJobYaml();

//         NestedContainerConfig restored;

//         if (!restored.fromJobYaml(yaml))
//             return std::size_t{0};

//         return restored.optionalSensors.size() +
//                restored.sharedSensors.size() +
//                restored.namedSensors.size();
//     };
// }
// #endif
