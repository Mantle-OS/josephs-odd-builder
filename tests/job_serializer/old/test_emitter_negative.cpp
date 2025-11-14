#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include <emitter.h>
#include <schema.h>
#include <reader.h>
#include <job_logger.h>

using namespace job::serializer;
using namespace job::core;

static std::filesystem::path makeInvalidSchemaForEmit(const std::string &name, const std::string &content)
{
    const auto tmp = std::filesystem::temp_directory_path() / name;
    std::ofstream out(tmp);
    out << content;
    out.close();
    return tmp;
}

TEST_CASE("Emitter gracefully handles invalid schemas", "[emitter][negative]")
{
    JobLogger::instance().setLevel(LogLevel::Info);

    Reader reader;
    const auto outDir = std::filesystem::temp_directory_path() / "job_emitter_neg";

    std::filesystem::remove_all(outDir);
    std::filesystem::create_directories(outDir);

    Emitter emitter(std::filesystem::path{});

    SECTION("Missing required fields should fail early")
    {
        const auto badYaml = makeInvalidSchemaForEmit(
            "job_schema_emit_missing.yaml",
            R"(
version: 1
unit: core
base: basefile
c_struct: BrokenStruct
include_prefix: include/jobmsgpack
out_base: broken_out
fields:
  - key: 1
    name: f_str
    type: str
)");

        REQUIRE_FALSE(reader.load(SerializeFormat::File, badYaml.string()));
    }

    SECTION("Corrupt YAML should throw and be caught cleanly")
    {
        const auto badYaml = makeInvalidSchemaForEmit(
            "job_schema_emit_corrupt.yaml",
            R"(
tag: bad
version: 1
fields:
  - key: 1
    name: a
    type: [str
)"); // ← intentionally broken bracket

        REQUIRE_FALSE(reader.load(SerializeFormat::File, badYaml.string()));
    }

    SECTION("Emitter should refuse to emit invalid schema")
    {
        Schema schema{};
        schema.tag = "broken_emit";
        schema.version = 0; // Invalid version
        schema.unit = "core";
        schema.base = "broken";
        schema.c_struct = "InvalidStruct";
        schema.include_prefix = "include/jobmsgpack";
        schema.out_base = "broken_emit";

        REQUIRE_FALSE(emitter.emit(schema, outDir));
    }

    std::filesystem::remove_all(outDir);
}
// CHECKPOINT: v1
