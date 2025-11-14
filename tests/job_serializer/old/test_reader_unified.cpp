#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

#include <schema.h>
#include <reader.h>
#include <job_logger.h>

using namespace job::serializer;
using namespace job::core;

static std::string makeSampleYaml()
{
    return R"(
tag: unified_test
version: 1
unit: core
base: basefile
c_struct: UnifiedStruct
include_prefix: include/jobmsgpack
out_base: unified_out
fields:
  - key: 1
    name: f_str
    type: str
    required: true
  - key: 2
    name: f_bin
    type: bin[4]
  - key: 3
    name: f_list
    type: list<str>
)";
}

static std::filesystem::path makeTempYamlFile()
{
    const auto tmp = std::filesystem::temp_directory_path() / "job_schema_unified.yaml";
    std::ofstream out(tmp);
    out << makeSampleYaml();
    out.close();
    return tmp;
}

TEST_CASE("Reader handles all DataType modes consistently", "[reader][unified]")
{
    JobLogger::instance().setLevel(LogLevel::Info);

    Reader readerFile;
    Reader readerText;
    Reader readerJson;

    // === FILE MODE ===
    const auto yamlPath = makeTempYamlFile();
    REQUIRE(readerFile.load(SerializeFormat::File, yamlPath.string()));
    REQUIRE(readerFile.isLoaded());
    const auto &schemaFile = readerFile.schema();
    REQUIRE(schemaFile.isValid());

    // === TEXT MODE ===
    const std::string yamlText = makeSampleYaml();
    REQUIRE(readerText.load(SerializeFormat::Text, yamlText));
    REQUIRE(readerText.isLoaded());
    const auto &schemaText = readerText.schema();
    REQUIRE(schemaText.isValid());

    // === JSON MODE ===
    nlohmann::json j = schemaFile;
    REQUIRE(readerJson.load(SerializeFormat::Json, j.dump()));
    REQUIRE(readerJson.isLoaded());
    const auto &schemaJson = readerJson.schema();
    REQUIRE(schemaJson.isValid());

    // === EQUIVALENCE ===
    REQUIRE(schemaFile == schemaText);
    REQUIRE(schemaFile == schemaJson);

    // === INVALID JSON ===
    Reader badReader;
    std::string badJson = "{ \"tag\": 123, "; // intentionally truncated
    REQUIRE_FALSE(badReader.load(SerializeFormat::Json, badJson));

    std::filesystem::remove(yamlPath);
}
// CHECKPOINT: v1
