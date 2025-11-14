#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include <writer.h>
#include <schema.h>
#include <reader.h>
#include <job_logger.h>

using namespace job::serializer;
using namespace job::core;

TEST_CASE("Writer gracefully handles invalid or unwritable schemas", "[writer][negative]")
{
    JobLogger::instance().setLevel(LogLevel::Info);

    Writer writer;
    Reader reader;
    Schema schema;

    const auto tmpDir = std::filesystem::temp_directory_path() / "job_writer_negative";
    std::filesystem::remove_all(tmpDir);
    std::filesystem::create_directories(tmpDir);

    SECTION("Fails when schema is invalid (missing tag or fields)")
    {
        schema.version = 1;
        schema.unit = "core";
        schema.base = "bad";
        schema.c_struct = "BrokenStruct";
        schema.include_prefix = "include/jobmsgpack";
        schema.out_base = "bad_out";

        const auto yamlOut = tmpDir / "invalid_schema.yaml";
        REQUIRE_FALSE(writer.write(schema, yamlOut, SerializeFormat::Yaml));
    }

    SECTION("Fails when file path is invalid (nonexistent directory)")
    {
        schema.tag = "bad_path";
        schema.version = 1;
        schema.unit = "core";
        schema.base = "basefile";
        schema.c_struct = "BrokenStruct";
        schema.include_prefix = "include/jobmsgpack";
        schema.out_base = "broken_out";

        Field f;
        f.key = 1;
        f.name = "f_field";
        f.type = "str";
        schema.fields.push_back(f);

        const auto badDir = tmpDir / "nonexistent_dir" / "nested" / "file.yaml";
        REQUIRE_FALSE(writer.write(schema, badDir, SerializeFormat::Yaml));
    }

    SECTION("Fails when JSON/YAML mode misconfigured (manual override)")
    {
        schema.tag = "writer_test";
        schema.version = 1;
        schema.unit = "core";
        schema.base = "basefile";
        schema.c_struct = "BrokenStruct";
        schema.include_prefix = "include/jobmsgpack";
        schema.out_base = "writer_test";

        Field f1{};
        f1.key = 1;
        f1.name = "f1";
        f1.type = "str";
        f1.kind = FieldKind::Scalar;
        schema.fields.push_back(f1);

        const auto outFile = tmpDir / "force_invalid.ext"; // extension not mapped
        REQUIRE(writer.write(schema, outFile, SerializeFormat::Yaml)); // should warn, fallback to YAML
        REQUIRE(std::filesystem::exists(outFile));                // file created
    }

    SECTION("Fails cleanly if unable to open for write (read-only directory)")
    {
        // Skip on systems without permission simulation support
        const auto protectedDir = tmpDir / "readonly";
        std::filesystem::create_directories(protectedDir);
        std::filesystem::permissions(protectedDir,
                                     std::filesystem::perms::owner_read |
                                         std::filesystem::perms::group_read |
                                         std::filesystem::perms::others_read,
                                     std::filesystem::perm_options::replace);

        schema.tag = "read_only";
        schema.version = 1;
        schema.unit = "core";
        schema.base = "test";
        schema.c_struct = "TestStruct";
        schema.include_prefix = "include/jobmsgpack";
        schema.out_base = "read_only";

        Field f2{};
        f2.key = 1;
        f2.name = "f";
        f2.type = "str";
        f2.kind = FieldKind::Scalar;
        schema.fields.push_back(f2);

        const auto outFile = protectedDir / "no_write.yaml";
        REQUIRE_FALSE(writer.write(schema, outFile, SerializeFormat::Yaml));

        // Restore permissions so cleanup succeeds
        std::filesystem::permissions(protectedDir,
                                     std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::replace);
    }

    std::filesystem::remove_all(tmpDir);
}
// CHECKPOINT: v1
