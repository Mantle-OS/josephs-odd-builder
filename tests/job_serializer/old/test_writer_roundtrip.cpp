#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
// #include <sstream>

#include <schema.h>
#include <reader.h>
#include <writer.h>
#include <job_logger.h>

using namespace job::serializer;
using namespace job::core;

static std::filesystem::path makeSampleSchemaFile()
{
    const auto tmp = std::filesystem::temp_directory_path() / "job_writer_roundtrip.yaml";
    std::ofstream out(tmp);
    out << R"(
tag: writer_test
version: 2
unit: io
base: baseunit
c_struct: WriterStruct
include_prefix: include/jobmsgpack
out_base: writer_test
fields:
  - key: 1
    name: f_name
    type: str
    required: true
  - key: 2
    name: f_bytes
    type: bin[32]
  - key: 3
    name: f_ids
    type: list<str>
)";
    out.close();
    return tmp;
}

TEST_CASE("Writer produces reversible YAML and JSON", "[writer][roundtrip]")
{
    JobLogger::instance().setLevel(LogLevel::Info);

    Reader reader;
    Schema original;

    const auto schemaFile = makeSampleSchemaFile();
    REQUIRE(reader.loadFromFile(schemaFile));
    original = reader.schema();
    REQUIRE(original.isValid());

    const auto tmpDir = std::filesystem::temp_directory_path() / "job_writer_roundtrip";
    std::filesystem::remove_all(tmpDir);
    std::filesystem::create_directories(tmpDir);

    Writer writer;

    SECTION("YAML round-trip preserves schema equality")
    {
        const auto yamlOut = tmpDir / "roundtrip.yaml";
        REQUIRE(writer.write(original, yamlOut, WriterMode::Yaml));

        Schema reloaded;
        REQUIRE(reader.loadFromFile(yamlOut));
        reloaded = reader.schema();

        REQUIRE(reloaded == original);
    }

    SECTION("JSON round-trip preserves schema equality")
    {
        const auto jsonOut = tmpDir / "roundtrip.json";
        REQUIRE(writer.write(original, jsonOut, WriterMode::Json));

        Reader r2;
        Schema fromJson;
        REQUIRE(r2.loadFromFile(jsonOut));
        fromJson = r2.schema();

        REQUIRE(fromJson == original);
    }

    SECTION("YAML→JSON cross-conversion maintains logical equality")
    {
        const auto yamlOut = tmpDir / "cross.yaml";
        const auto jsonOut = tmpDir / "cross.json";

        REQUIRE(writer.write(original, yamlOut, WriterMode::Yaml));

        Reader rYaml;
        Schema sYaml;
        REQUIRE(rYaml.loadFromFile(yamlOut));
        sYaml = rYaml.schema();

        REQUIRE(writer.write(sYaml, jsonOut, WriterMode::Json));

        Reader rJson;
        Schema sJson;
        REQUIRE(rJson.loadFromFile(jsonOut));
        sJson = rJson.schema();

        REQUIRE(sYaml == sJson);
    }

    std::filesystem::remove_all(tmpDir);
    std::filesystem::remove(schemaFile);
}
// CHECKPOINT: v1
