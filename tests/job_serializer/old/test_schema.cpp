#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include <schema.h>
#include <job_logger.h>

using namespace job::core;
using namespace job::serializer;

static std::filesystem::path makeTempSchema()
{
    auto tmp = std::filesystem::temp_directory_path() / "job_schema_test.yaml";
    std::ofstream out(tmp);

    out << R"(
tag: test_tag
version: 1
unit: core
base: basefile
c_struct: test_struct
include_prefix: include/jobmsgpack
out_base: test_out
fields:
  - key: 1
    name: field_str
    type: str
    required: true
    comment: "Sample string field"
  - key: 2
    name: field_bin
    type: bin[16]
  - key: 3
    name: field_list
    type: list<str>
)";
    out.close();
    return tmp;
}

static std::filesystem::path makeBrokenSchema(const std::string &content,
                                              const std::string &name)
{
    auto tmp = std::filesystem::temp_directory_path() / name;
    std::ofstream out(tmp);
    out << content;
    out.close();
    return tmp;
}


TEST_CASE("Schema loads successfully", "[schema]")
{
    JobLogger::instance().setLevel(LogLevel::Debug);

    Schema s;
    const auto file = makeTempSchema();

    REQUIRE(std::filesystem::exists(file));
    REQUIRE(loadSchemaFromFile(file, s));
    REQUIRE(s.isValid());

    SECTION("Field contents are parsed correctly")
    {
        REQUIRE(s.fields.size() == 3);
        REQUIRE(s.fields[0].name == "field_str");
        REQUIRE(s.fields[0].kind == FieldKind::Scalar);
        REQUIRE(s.fields[1].kind == FieldKind::Bin);
        REQUIRE(s.fields[2].kind == FieldKind::ListScalar);
    }

    SECTION("Logger dump produces expected output")
    {
        dumpSchema(s);
        // Not asserting output text (logged to stderr), just confirming no crash.
        SUCCEED();
    }

    std::filesystem::remove(file);
}


TEST_CASE("Schema fails to load with malformed input", "[schema-negative]")
{
    JobLogger::instance().setLevel(LogLevel::Debug);

    SECTION("Missing required key")
    {
        const auto file = makeBrokenSchema(R"(
version: 1
unit: core
base: basefile
c_struct: test_struct
include_prefix: include/jobmsgpack
out_base: test_out
fields: []
)", "job_schema_missing_key.yaml");

        Schema s;
        REQUIRE_FALSE(loadSchemaFromFile(file, s));
        REQUIRE_FALSE(s.isValid());

        std::filesystem::remove(file);
    }

    SECTION("Invalid field type")
    {
        const auto file = makeBrokenSchema(R"(
tag: bad_tag
version: 1
unit: core
base: basefile
c_struct: test_struct
include_prefix: include/jobmsgpack
out_base: test_out
fields:
  - key: 1
    name: bad_field
    type: banana
)", "job_schema_invalid_type.yaml");

        Schema s;
        const bool ok = loadSchemaFromFile(file, s);

        // Loader shouldn't crash, but schema likely invalid
        REQUIRE(ok);
        REQUIRE_FALSE(s.fields.empty());
        REQUIRE(s.fields[0].kind == FieldKind::Scalar); // defaults safely
        REQUIRE(s.fields[0].type == "banana");

        dumpSchema(s);
        std::filesystem::remove(file);
    }

    SECTION("Completely invalid YAML")
    {
        const auto file = makeBrokenSchema(R"(
tag: test
version: :bad:
this_is_not_yaml:
    : lol:
)", "job_schema_invalid_yaml.yaml");

        Schema s;
        REQUIRE_FALSE(loadSchemaFromFile(file, s));
        std::filesystem::remove(file);
    }
}


TEST_CASE("Schema round-trip between YAML and JSON", "[schema-roundtrip]")
{
    JobLogger::instance().setLevel(LogLevel::Debug);

    const auto file = makeTempSchema();
    Schema original;

    REQUIRE(std::filesystem::exists(file));
    REQUIRE(loadSchemaFromFile(file, original));
    REQUIRE(original.isValid());

    nlohmann::json j = original;

    auto jsonFile = std::filesystem::temp_directory_path() / "job_schema_test.json";
    {
        std::ofstream out(jsonFile);
        out << j.dump(4);
        out.close();
    }

    std::ifstream in(jsonFile);
    nlohmann::json j2;
    in >> j2;
    Schema clone = j2.get<Schema>();

    REQUIRE(clone.tag == original.tag);
    REQUIRE(clone.version == original.version);
    REQUIRE(clone.unit == original.unit);
    REQUIRE(clone.base == original.base);
    REQUIRE(clone.c_struct == original.c_struct);
    REQUIRE(clone.include_prefix == original.include_prefix);
    REQUIRE(clone.out_base == original.out_base);
    REQUIRE(clone.fields.size() == original.fields.size());

    for (size_t i = 0; i < clone.fields.size(); ++i) {
        const auto &a = original.fields[i];
        const auto &b = clone.fields[i];
        REQUIRE(a.key == b.key);
        REQUIRE(a.name == b.name);
        REQUIRE(a.type == b.type);
        REQUIRE(a.kind == b.kind);
        REQUIRE(a.required == b.required);
        REQUIRE(a.comment == b.comment);
        REQUIRE(a.size == b.size);
        REQUIRE(a.ctype == b.ctype);
        REQUIRE(a.ref_include == b.ref_include);
        REQUIRE(a.ref_sym == b.ref_sym);
    }

    JOB_LOG_INFO("[schema] round-trip YAML→JSON→Schema success: {}", file.string());

    std::filesystem::remove(file);
    std::filesystem::remove(jsonFile);
}


static std::filesystem::path makeTempSchemaEq()
{
    auto tmp = std::filesystem::temp_directory_path() / "job_schema_eq.yaml";
    std::ofstream out(tmp);
    out << R"(
tag: equality_test
version: 2
unit: core
base: basefile
c_struct: eq_struct
include_prefix: include/jobmsgpack
out_base: eq_out
fields:
  - key: 1
    name: field_one
    type: str
  - key: 2
    name: field_two
    type: bin[8]
)";
    out.close();
    return tmp;
}

TEST_CASE("Schema equality and inequality", "[schema][operators]")
{
    JobLogger::instance().setLevel(LogLevel::Info);

    Schema s1, s2;
    const auto path = makeTempSchemaEq();

    REQUIRE(loadSchemaFromFile(path, s1));
    REQUIRE(loadSchemaFromFile(path, s2));

    SECTION("Schemas compare equal when identical")
    {
        REQUIRE(s1 == s2);
    }

    SECTION("Schemas compare unequal when changed")
    {
        s2.version = 999;
        REQUIRE_FALSE(s1 == s2);

        s2 = s1;
        REQUIRE(s1 == s2);

        s2.fields[0].name = "mutated_field";
        REQUIRE_FALSE(s1 == s2);
    }

    std::filesystem::remove(path);
}

// CHECKPOINT: v1
