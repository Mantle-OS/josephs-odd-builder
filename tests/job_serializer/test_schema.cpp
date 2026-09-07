#include <catch2/catch_all.hpp>

#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

#include <job_serializer_utils.h>
#include <schema.h>

using namespace job::serializer;
using json = nlohmann::json;

static const std::string VALID_SCHEMA_YAML = R"(
tag: "TestMsg"
version: 1
unit: "tests"
base: "BaseStruct"
c_struct: "TestMsg_t"
include_prefix: "job_tests"
out_base: "test_msg"
fields:
  - name: "count"
    type: "u32"
  - name: "data"
    type: "bin[32]"
)";

static const json VALID_SCHEMA_JSON = json::parse(R"({
    "tag": "TestMsg",
    "version": 1,
    "unit": "tests",
    "base": "BaseStruct",
    "c_struct": "TestMsg_t",
    "include_prefix": "job_tests",
    "out_base": "test_msg",
    "fields": [
        {
            "name": "count",
            "type": "u32"
        },
        {
            "name": "data",
            "type": "bin[32]"
        }
    ]
})");

TEST_CASE("Schema::isValid() correctly validates schema requirements", "[schema]")
{
    Schema s{};

    s.tag = "TestMsg";
    s.version = 1;
    s.unit = "tests";
    s.base = "BaseStruct";
    s.c_struct = "TestMsg_t";
    s.include_prefix = "job_tests";
    s.out_base = "test_msg";

    Field f1{};
    f1.name = "field1";
    f1.type = "i32";
    f1.kind = deduceFieldKind(f1.type);

    s.fields.push_back(f1);

    SECTION("Valid schema")
    {
        REQUIRE(s.isValid());
    }

    SECTION("Invalid: Missing Tag")
    {
        s.tag.clear();
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Zero Version")
    {
        s.version = 0;
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Empty Fields")
    {
        s.fields.clear();
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Missing C struct name")
    {
        s.c_struct.clear();
        REQUIRE_FALSE(s.isValid());
    }
}

TEST_CASE("Schema::parse correctly handles YAML input", "[schema][from_yaml]")
{
    Schema s{};
    YAML::Node root = YAML::Load(VALID_SCHEMA_YAML);

    SECTION("Successful YAML parse")
    {
        REQUIRE(Schema::parse(root, s));
        REQUIRE(s.isValid());
        REQUIRE(s.tag == "TestMsg");
        REQUIRE(s.version == 1);
        REQUIRE(s.fields.size() == 2);
        REQUIRE(s.fields[1].name == "data");
        REQUIRE(s.fields[1].kind == FieldKind::Bin);

        REQUIRE(s.hdr_name.string() == "test_msg.hpp");
        REQUIRE(s.src_name.string() == "test_msg.cpp");
    }

    SECTION("YAML parse failure - Missing fields")
    {
        const std::string badYaml = R"(
            tag: "TestMsg"
            version: 1
            unit: "tests"
            base: "BaseStruct"
            c_struct: "TestMsg_t"
            include_prefix: "job_tests"
            out_base: "test_msg"
        )";

        YAML::Node badRoot = YAML::Load(badYaml);

        REQUIRE_FALSE(Schema::parse(badRoot, s));
    }

    SECTION("YAML parse failure - Invalid version")
    {
        const std::string badYaml = R"(
            tag: "TestMsg"
            version: 0
            unit: "tests"
            base: "BaseStruct"
            c_struct: "TestMsg_t"
            include_prefix: "job_tests"
            out_base: "test_msg"
            fields:
              - name: "count"
                type: "u32"
        )";

        YAML::Node badRoot = YAML::Load(badYaml);

        REQUIRE_FALSE(Schema::parse(badRoot, s));
    }
}

TEST_CASE("Schema::parse correctly handles JSON input", "[schema][from_json]")
{
    Schema s{};

    SECTION("Successful JSON parse (using ADL)")
    {
        REQUIRE(Schema::parse(VALID_SCHEMA_JSON, s));
        REQUIRE(s.isValid());
        REQUIRE(s.tag == "TestMsg");
        REQUIRE(s.version == 1);
        REQUIRE(s.fields.size() == 2);
        REQUIRE(s.fields[1].name == "data");
        REQUIRE(s.fields[1].kind == FieldKind::Bin);

        REQUIRE(s.hdr_name.string() == "test_msg.hpp");
        REQUIRE(s.src_name.string() == "test_msg.cpp");
    }

    SECTION("JSON parse failure - Missing Field Data")
    {
        json badJson = VALID_SCHEMA_JSON;
        badJson["fields"] = json::array({
            {
                {"name", "f1"}
            }
        });

        REQUIRE_FALSE(Schema::parse(badJson, s));
    }

    SECTION("JSON parse failure - Invalid version (0)")
    {
        json badJson = VALID_SCHEMA_JSON;
        badJson["version"] = 0;

        REQUIRE_FALSE(Schema::parse(badJson, s));
    }
}

TEST_CASE("Schema to/from round trip verification", "[schema][roundtrip]")
{
    Schema sIn{};
    YAML::Node root = YAML::Load(VALID_SCHEMA_YAML);

    REQUIRE(Schema::parse(root, sIn));

    sIn.fields.push_back({
        .key = 100,
        .name = "nested_struct",
        .type = "struct",
        .kind = FieldKind::Struct,
        .size{},
        .ctype{},
        .ref_include = "path/to/other.hpp",
        .ref_sym{},
        .required = true,
        .comment = "Test comment for roundtrip"
    });

    YAML::Emitter yamlEmitter;
    Schema::to_yaml(yamlEmitter, sIn);

    json j;
    Schema::to_json(j, sIn);

    SECTION("Parsers correctly normalize 'ref_sym'")
    {
        Schema sYamlOut{};
        YAML::Node yamlRoundtrip = YAML::Load(yamlEmitter.c_str());

        REQUIRE(Schema::parse(yamlRoundtrip, sYamlOut));

        Schema sJsonOut{};
        REQUIRE(Schema::parse(j, sJsonOut));

        const auto &fYaml = sYamlOut.fields.back();
        const auto &fJson = sJsonOut.fields.back();

        REQUIRE(fYaml.ref_include == "path/to/other.hpp");
        REQUIRE(fYaml.ref_sym == "other");

        REQUIRE(fJson.ref_include == "path/to/other.hpp");
        REQUIRE(fJson.ref_sym == "other");

        REQUIRE(sYamlOut == sJsonOut);
    }
}