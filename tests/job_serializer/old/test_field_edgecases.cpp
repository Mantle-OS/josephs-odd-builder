#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

#include <schema.h>
#include <job_logger.h>

using namespace job::serializer;
using namespace job::core;

static std::filesystem::path makeSchemaEdge(const std::string &name, const std::string &content)
{
    auto tmp = std::filesystem::temp_directory_path() / name;
    std::ofstream out(tmp);
    out << content;
    out.close();
    return tmp;
}

TEST_CASE("Schema parser handles field edge cases", "[schema][fields]")
{
    JobLogger::instance().setLevel(LogLevel::Debug);

    Schema s;
    const auto out = std::filesystem::temp_directory_path();

    SECTION("Missing type should fail gracefully")
    {
        const auto file = makeSchemaEdge("job_schema_missing_type.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - name: bad_field
)");
        REQUIRE_FALSE(loadSchemaFromFile(file, s));
    }

    SECTION("Invalid bin size should warn and continue")
    {
        const auto file = makeSchemaEdge("job_schema_bad_bin.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - key: 1
    name: bin_field
    type: bin[abc]
)");
        REQUIRE(loadSchemaFromFile(file, s));
        REQUIRE(s.fields.size() == 1);
        REQUIRE_FALSE(s.fields[0].size.has_value());
    }

    SECTION("Invalid negative list bin size")
    {
        const auto file = makeSchemaEdge("job_schema_list_neg.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - key: 1
    name: badlist
    type: list<bin[-5]>
)");
        REQUIRE(loadSchemaFromFile(file, s));
        REQUIRE_FALSE(s.fields[0].size.has_value());
    }

    SECTION("Unknown type falls back to Scalar kind")
    {
        const auto file = makeSchemaEdge("job_schema_unknown_type.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - key: 1
    name: weird
    type: banana
)");
        REQUIRE(loadSchemaFromFile(file, s));
        REQUIRE(s.fields[0].kind == FieldKind::Scalar);
    }

    SECTION("Duplicate field keys do not crash")
    {
        const auto file = makeSchemaEdge("job_schema_dup.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - key: 1
    name: one
    type: str
  - key: 1
    name: two
    type: str
)");
        REQUIRE(loadSchemaFromFile(file, s));
        REQUIRE(s.fields.size() == 2);
        REQUIRE(s.fields[0].key == s.fields[1].key);
    }

    SECTION("Malformed mixed field entry should not crash")
    {
        const auto file = makeSchemaEdge("job_schema_mixed.yaml", R"(
tag: test
version: 1
unit: core
base: base
c_struct: Foo
include_prefix: inc
out_base: foo
fields:
  - key: 1
    name: good
    type: str
  - 42
)");
        REQUIRE(loadSchemaFromFile(file, s)); // It succeeded but safely
        REQUIRE(s.fields.size() == 1);        // Only valid one kept
    }
}
// CHECKPOINT: v1
