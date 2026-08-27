#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include <job_base_obj.h>
#include <job_obj_concept.h>

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

    nlohmann::json serializedJson = config.toJson();
    REQUIRE(serializedJson.is_object());
    REQUIRE(serializedJson["nodeName"] == "edge-inference-01");
    REQUIRE(serializedJson["threadPoolSize"] == 32);
    REQUIRE(serializedJson["auxiliarySensors"].is_array());
    REQUIRE(serializedJson["auxiliarySensors"].size() == 2);

    ComputeNodeConfig restored;
    const bool success = restored.fromJson(serializedJson);

    REQUIRE(success);
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

    YAML::Node yamlNode = config.toYaml();
    REQUIRE(yamlNode.IsMap());
    REQUIRE(yamlNode["nodeName"].as<std::string>() == "cluster-head");

    ComputeNodeConfig restored;
    const bool success = restored.fromYaml(yamlNode);

    REQUIRE(success);
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
    const bool success = restored.fromBinary(streamSpan);

    REQUIRE(success);
    CHECK(streamSpan.empty());
    CHECK(restored.nodeName == "hpc-worker-99");
    CHECK(restored.threadPoolSize == 128);
    CHECK(restored.scalingFactors.size() == 4);
    CHECK(restored.primarySensor.sensorTag == "die_top");
    REQUIRE(restored.auxiliarySensors.size() == 1);
    CHECK(restored.auxiliarySensors[0]->sensorTag == "pcie_lane");
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

    SECTION("YAML parsing non-map root node fails gracefully")
    {
        ComputeNodeConfig restored;
        YAML::Node scalarNode = YAML::Load("invalid_scalar_root");

        const bool success = restored.fromYaml(scalarNode);

        CHECK_FALSE(success);
        CHECK_THAT(restored.lastErrorString, Catch::Matchers::ContainsSubstring("map"));
    }
}

TEST_CASE("BaseObject: SmartPointer concept includes shared and unique ownership", "[core][base_obj][edge_cases][smart_pointer]")
{
    STATIC_REQUIRE(SmartPointer<std::shared_ptr<SubSensorConfig>>);
    STATIC_REQUIRE(SmartPointer<std::unique_ptr<SubSensorConfig>>);
    STATIC_REQUIRE_FALSE(SmartPointer<int>);
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

        YAML::Node node;
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

TEST_CASE("BaseObject: Containers roundtrip at useful boundaries", "[core][base_obj][edge_cases][container]")
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

        for (std::int32_t i = 0; i < 1024; ++i) {
            config.values.push_back(i);
        }

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

TEST_CASE("BaseObject: Binary map containers roundtrip", "[core][base_obj][edge_cases][map][binary]")
{
    BinaryMapConfig config;
    config.entries = {
        {1, "cpu"},
        {2, "cuda"},
        {3, "vulkan"}
    };

    std::vector<std::uint8_t> buffer;
    config.toBinary(buffer);

    BinaryMapConfig restored;
    std::span<const std::uint8_t> streamSpan(buffer);
    REQUIRE(restored.fromBinary(streamSpan));

    CHECK(streamSpan.empty());
    CHECK(restored.entries == config.entries);
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

    nlohmann::json jsonPayload = config.toJson();
    YAML::Node yamlPayload = config.toYaml();

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
        return restored.fromBinary(span);
    };

    BENCHMARK("JSON Serialization (toJson)")
    {
        return config.toJson();
    };

    BENCHMARK("JSON Deserialization (fromJson)")
    {
        ComputeNodeConfig restored;
        return restored.fromJson(jsonPayload);
    };

    BENCHMARK("YAML Serialization (toYaml)")
    {
        return config.toYaml();
    };

    BENCHMARK("YAML Deserialization (fromYaml)")
    {
        ComputeNodeConfig restored;
        return restored.fromYaml(yamlPayload);
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
        return restored.fromBinary(streamSpan);
    };

    BENCHMARK("Large nested JSON roundtrip")
    {
        const nlohmann::json json = config.toJson();

        ComputeNodeConfig restored;
        return restored.fromJson(json);
    };

    BENCHMARK("Large nested YAML roundtrip")
    {
        const YAML::Node yaml = config.toYaml();

        ComputeNodeConfig restored;
        return restored.fromYaml(yaml);
    };
}

#endif