#include <catch2/catch_all.hpp>
#include <string>

#include <yaml-cpp/yaml.h>

#include <nlohmann/json.hpp>

#include <job_serializer_utils.h>
#include <schema.h>

using namespace job::serializer;
using json = nlohmann::json;

// Helper to create a minimal, valid Schema in YAML format
const std::string VALID_SCHEMA_YAML = R"(
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

// Helper to create a minimal, valid Schema in JSON format
const json VALID_SCHEMA_JSON = json::parse(R"({"tag": "TestMsg", "version": 1, "unit": "tests", "base": "BaseStruct", "c_struct": "TestMsg_t", "include_prefix": "job_tests", "out_base": "test_msg", "fields": [{"name": "count", "type": "u32"}, {"name": "data", "type": "bin[32]"}]})");


TEST_CASE("Schema::isValid() correctly validates schema requirements", "[schema]") {
    Schema s{};

    s.tag = "TestMsg";
    s.version = 1;
    s.unit = "tests";
    s.base = "BaseStruct";
    s.c_struct = "TestMsg_t";
    s.include_prefix = "job_tests";
    s.out_base = "test_msg";

    Field f1;
    f1.name = "field1";
    f1.type = "i32";
    f1.kind = deduceFieldKind(f1.type);
    s.fields.push_back(f1);

    SECTION("Valid schema") {
        REQUIRE(s.isValid());
    }

    SECTION("Invalid: Missing Tag") {
        s.tag = "";
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Zero Version") {
        s.tag = "TestMsg";
        s.version = 0;
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Empty Fields") {
        s.version = 1;
        s.fields.clear();
        REQUIRE_FALSE(s.isValid());
    }

    SECTION("Invalid: Missing C struct name") {
        s.c_struct = "";
        REQUIRE_FALSE(s.isValid());
    }
}

TEST_CASE("Schema::parse correctly handles YAML input", "[schema][from_yaml]") {
    Schema s{};
    YAML::Node root = YAML::Load(VALID_SCHEMA_YAML);

    SECTION("Successful YAML parse") {
        REQUIRE(Schema::parse(root, s));
        REQUIRE(s.isValid());
        REQUIRE(s.tag == "TestMsg");
        REQUIRE(s.version == 1);
        REQUIRE(s.fields.size() == 2);
        REQUIRE(s.fields[1].name == "data");
        REQUIRE(s.fields[1].kind == FieldKind::Bin);

        // Check Smart Defaults for filenames
        REQUIRE(s.hdr_name.string() == "test_msg.hpp");
        REQUIRE(s.src_name.string() == "test_msg.cpp");
    }

    SECTION("YAML parse failure - Missing fields") {
        std::string bad_yaml = R"(
            tag: "TestMsg"
            version: 1
            unit: "tests"
            base: "BaseStruct"
            c_struct: "TestMsg_t"
            include_prefix: "job_tests"
            out_base: "test_msg"
            # fields: deliberately missing or empty
        )";
        YAML::Node bad_root = YAML::Load(bad_yaml);
        REQUIRE_FALSE(Schema::parse(bad_root, s));
    }

    SECTION("YAML parse failure - Invalid version") {
        std::string bad_yaml = R"(
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
        YAML::Node bad_root = YAML::Load(bad_yaml);
        REQUIRE_FALSE(Schema::parse(bad_root, s));
    }
}

TEST_CASE("Schema::parse correctly handles JSON input", "[schema][from_json]") {
    Schema s{};

    SECTION("Successful JSON parse (using ADL)") {
        REQUIRE(Schema::parse(VALID_SCHEMA_JSON, s));
        REQUIRE(s.isValid());
        REQUIRE(s.tag == "TestMsg");
        REQUIRE(s.version == 1);
        REQUIRE(s.fields.size() == 2);
        REQUIRE(s.fields[1].name == "data");
        REQUIRE(s.fields[1].kind == FieldKind::Bin);

        // Check that optional paths default correctly
        REQUIRE(s.hdr_name.string() == "test_msg.hpp");
        REQUIRE(s.src_name.string() == "test_msg.cpp");
    }

    SECTION("JSON parse failure - Missing Field Data") {
        json bad_json = VALID_SCHEMA_JSON;
        // Tamper with fields to cause a failure in Field::from_json or validation
        bad_json["fields"] = json::array({
            {"name", "f1"} // Missing 'type'
        });

        REQUIRE_FALSE(Schema::parse(bad_json, s));
    }

    SECTION("JSON parse failure - Invalid version (0)") {
        json bad_json = VALID_SCHEMA_JSON;
        bad_json["version"] = 0;
        REQUIRE_FALSE(Schema::parse(bad_json, s));
    }
}

TEST_CASE("Schema to/from round trip verification", "[schema][roundtrip]") {
    Schema s_in{};
    YAML::Node root = YAML::Load(VALID_SCHEMA_YAML);
    REQUIRE(Schema::parse(root, s_in));
    s_in.fields.push_back({
        .key = 100,
        .name = "nested_struct",
        .type = "struct",
        .kind = FieldKind::Struct,
        .size = {},
        .ctype = {},
        .ref_include = "path/to/other.hpp",
        .ref_sym = {},
        .required = true,
        .comment = "Test comment for roundtrip"
    });

    YAML::Emitter yaml_emitter;
    Schema::to_yaml(yaml_emitter, s_in);

    json j;
    Schema::to_json(j, s_in);

    SECTION("Parsers correctly normalize 'ref_sym'") {
        Schema s_yaml_out{};
        YAML::Node yaml_roundtrip = YAML::Load(yaml_emitter.c_str());
        REQUIRE(Schema::parse(yaml_roundtrip, s_yaml_out));

        Schema s_json_out{};
        REQUIRE(Schema::parse(j, s_json_out));

        const auto& f_yaml = s_yaml_out.fields.back();
        const auto& f_json = s_json_out.fields.back();

        REQUIRE(f_yaml.ref_include == "path/to/other.hpp");
        REQUIRE(f_yaml.ref_sym == "other");

        REQUIRE(f_json.ref_include == "path/to/other.hpp");
        REQUIRE(f_json.ref_sym == "other");
        REQUIRE(s_yaml_out == s_json_out);
    }

}
// CHECKPOINT: v1
