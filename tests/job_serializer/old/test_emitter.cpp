#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include <job_logger.h>

#include <emitter.h>
#include <schema.h>
#include <reader.h>
#include <printer.h>

using namespace job::core;
using namespace job::serializer;

static std::filesystem::path makeTempSchemaForEmit()
{
    auto tmp = std::filesystem::temp_directory_path() / "job_schema_emit.yaml";
    std::ofstream out(tmp);

    out << R"(
tag: emit_test
version: 1
unit: core
base: basefile
c_struct: EmitStruct
include_prefix: include/jobmsgpack
out_base: emit_test
fields:
  - key: 1
    name: f_str
    type: str
    required: true
  - key: 2
    name: f_bin
    type: bin[8]
  - key: 3
    name: f_list
    type: list<str>
)";
    out.close();
    return tmp;
}

TEST_CASE("Emitter generates deterministic header/source", "[emitter]")
{
    JobLogger::instance().setLevel(LogLevel::Debug);

    Reader reader;
    const auto schemaFile = makeTempSchemaForEmit();

    // ✅ Use unified Reader::load() instead of legacy loadSchemaFromFile
    REQUIRE(reader.load(DataType::File, schemaFile.string()));
    REQUIRE(reader.isLoaded());

    const Schema &schema = reader.schema();
    REQUIRE(schema.isValid());

    const auto outDir = std::filesystem::temp_directory_path() / "job_emitter_test";
    Emitter emitter(std::filesystem::path{});

    // Clean up before run
    std::filesystem::remove_all(outDir);
    std::filesystem::create_directories(outDir);

    REQUIRE(emitter.emit(schema, outDir));

    const auto hdr = outDir / "emit_test.hpp";
    const auto src = outDir / "emit_test.cpp";

    REQUIRE(std::filesystem::exists(hdr));
    REQUIRE(std::filesystem::exists(src));

    SECTION("Header and source files are deterministic")
    {
        std::ifstream h1(hdr);
        std::ifstream s1(src);
        std::ostringstream hb, sb;
        hb << h1.rdbuf();
        sb << s1.rdbuf();

        const auto firstHeader = hb.str();
        const auto firstSource = sb.str();

        // Emit again to verify no content change
        REQUIRE(emitter.emit(schema, outDir));

        std::ifstream h2(hdr);
        std::ifstream s2(src);
        std::ostringstream hb2, sb2;
        hb2 << h2.rdbuf();
        sb2 << s2.rdbuf();

        REQUIRE(firstHeader == hb2.str());
        REQUIRE(firstSource == sb2.str());
    }

    SECTION("Logs and structure look valid")
    {
        JOB_LOG_INFO("[test] Verifying emitter structure dump:");
        Printer p;
        p.print(schema);
        SUCCEED();
    }

    std::filesystem::remove_all(outDir);
    std::filesystem::remove(schemaFile);
}
// CHECKPOINT: v1
