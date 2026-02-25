#include <catch2/catch_all.hpp>

#include <string>

#include <yaml-cpp/yaml.h>

#include <job_serializer_utils.h>
#include <job_field.h>

using namespace job::serializer;
TEST_CASE("Field::parseBinSize extracts size from type string", "[job_field]") {
    SECTION("Valid inputs") {
        REQUIRE(Field::parseBinSize("bin[32]") == 32);
        REQUIRE(Field::parseBinSize("bin[1024]") == 1024);
    }
    SECTION("Invalid inputs") {
        REQUIRE(Field::parseBinSize("bin") == std::nullopt);
        REQUIRE(Field::parseBinSize("bin[abc]") == std::nullopt);
        REQUIRE(Field::parseBinSize("bin[0]") == std::nullopt);
        REQUIRE(Field::parseBinSize("bin[32") == std::nullopt);
        REQUIRE(Field::parseBinSize("bin[]") == std::nullopt);
        REQUIRE(Field::parseBinSize("list<bin[32]>") == std::nullopt);
    }
}

TEST_CASE("Field::parseListBinSize extracts size from type string", "[job_field]")
{
    SECTION("Valid inputs") {
        REQUIRE(Field::parseListBinSize("list<bin[64]>") == 64);
        REQUIRE(Field::parseListBinSize("list<bin[4096]>") == 4096);
    }
    SECTION("Invalid inputs") {
        REQUIRE(Field::parseListBinSize("list<bin>") == std::nullopt);
        REQUIRE(Field::parseListBinSize("list<bin[abc]>") == std::nullopt);
        REQUIRE(Field::parseListBinSize("list<bin[0]>") == std::nullopt);
        REQUIRE(Field::parseListBinSize("list<bin[64") == std::nullopt);
        REQUIRE(Field::parseListBinSize("bin[64]>") == std::nullopt);
    }
}

TEST_CASE("Field operator== compares two Field objects", "[job_field]") {
    Field f1{};
    f1.key = 1;
    f1.name = "test";
    f1.type = "i32";
    f1.kind = FieldKind::Scalar;

    Field f2{};
    f2.key = 1;
    f2.name = "test";
    f2.type = "i32";
    f2.kind = FieldKind::Scalar;

    SECTION("Identical fields") {
        REQUIRE(f1 == f2);
    }
    SECTION("Different name") {
        f2.name = "test2";
        REQUIRE_FALSE(f1 == f2);
    }
    SECTION("Different key") {
        f2.key = 2;
        REQUIRE_FALSE(f1 == f2);
    }
    SECTION("Different optional (size)") {
        f1.size = 10;
        REQUIRE_FALSE(f1 == f2);
        f2.size = 10;
        REQUIRE(f1 == f2);
        f2.size = 20;
        REQUIRE_FALSE(f1 == f2);
    }
    SECTION("Different optional (comment)") {
        f1.comment = "hello";
        REQUIRE_FALSE(f1 == f2);
        f2.comment = "hello";
        REQUIRE(f1 == f2);
        f2.comment = "world";
        REQUIRE_FALSE(f1 == f2);
    }
}

TEST_CASE("Field::from_yaml (single) populates Field struct from YAML", "[job_field]") {
    Field f{};

    SECTION("Minimal valid field") {
        std::string yaml = R"(
            name: "test_name"
            type: "i32"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.name == "test_name");
        REQUIRE(f.type == "i32");
        REQUIRE(f.kind == FieldKind::Scalar);
        REQUIRE(f.required == false);
        REQUIRE_FALSE(f.size.has_value());
    }

    SECTION("All properties field") {
        std::string yaml = R"(
            name: "full_field"
            type: "struct"
            required: true
            comment: "This is a test comment."
            size: 10
            ctype: "my_c_type_t"
            ref_include: "path/to/my_c_type.h"
            ref_sym: "MyCType"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.name == "full_field");
        REQUIRE(f.type == "struct");
        REQUIRE(f.kind == FieldKind::Struct);
        REQUIRE(f.required == true);
        REQUIRE(f.comment == "This is a test comment.");
        REQUIRE(f.size == 10);
        REQUIRE(f.ctype == "my_c_type_t");
        REQUIRE(f.ref_include == "path/to/my_c_type.h");
        REQUIRE(f.ref_sym == "MyCType");
    }

    SECTION("Smart size from bin type") {
        std::string yaml = R"(
            name: "data"
            type: "bin[32]"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.name == "data");
        REQUIRE(f.type == "bin[32]");
        REQUIRE(f.kind == FieldKind::Bin);
        REQUIRE(f.size == 32);
    }

    SECTION("Smart size from list<bin> type") {
        std::string yaml = R"(
            name: "packets"
            type: "list<bin[128]>"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.name == "packets");
        REQUIRE(f.type == "list<bin[128]>");
        REQUIRE(f.kind == FieldKind::ListBin);
        REQUIRE(f.size == 128);
    }

    SECTION("Explicit size overrides smart size") {
        std::string yaml = R"(
            name: "data"
            type: "bin[32]"
            size: 99
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.size == 99); // Explicit 'size' wins
    }

    SECTION("Smart ref_sym from ref_include") {
        std::string yaml = R"(
            name: "header"
            type: "struct"
            ref_include: "mpkg_mp_header.hpp"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.ref_include == "mpkg_mp_header.hpp");
        REQUIRE(f.ref_sym == "header"); // Smart default
    }

    SECTION("Explicit ref_sym overrides smart ref_sym") {
        std::string yaml = R"(
            name: "header"
            type: "struct"
            ref_include: "mpkg_mp_header.hpp"
            ref_sym: "MyCustomHeader"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, f));
        REQUIRE(f.ref_sym == "MyCustomHeader"); // Explicit 'ref_sym' wins
    }

    SECTION("Invalid fields") {
        Field temp_f{};
        REQUIRE_FALSE(Field::from_yaml(YAML::Load(""), temp_f));
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("- 1"), temp_f));
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("type: 'i32'"), temp_f));
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("name: 'test'"), temp_f));
    }
}

TEST_CASE("Field::from_yaml (vector) populates vector from YAML", "[job_field]")
{

    std::vector<Field> fields;

    SECTION("YAML Sequence format") {
        std::string yaml = R"(
            - name: "field_one"
              type: "i32"
            - name: "field_two"
              type: "str"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, fields));
        REQUIRE(fields.size() == 2);
        REQUIRE(fields[0].name == "field_one");
        REQUIRE(fields[1].name == "field_two");
        REQUIRE(fields[0].key == 0); // Key is 0 when not specified
        REQUIRE(fields[1].key == 0);
    }

    SECTION("YAML Map format") {
        std::string yaml = R"(
            10:
              name: "field_ten"
              type: "u32"
            20:
              name: "field_twenty"
              type: "bin[16]"
        )";
        YAML::Node node = YAML::Load(yaml);
        REQUIRE(Field::from_yaml(node, fields));
        REQUIRE(fields.size() == 2);
        // NOTE: std::unordered_map parsing means order is not guaranteed
        bool found10 = false;
        bool found20 = false;
        for(const auto& f : fields) {
            if (f.key == 10) {
                found10 = true;
                REQUIRE(f.name == "field_ten");
                REQUIRE(f.kind == FieldKind::Scalar);
            }
            if (f.key == 20) {
                found20 = true;
                REQUIRE(f.name == "field_twenty");
                REQUIRE(f.kind == FieldKind::Bin);
                REQUIRE(f.size == 16);
            }
        }
        REQUIRE(found10);
        REQUIRE(found20);
    }

    SECTION("Invalid node types") {
        fields.clear();
        REQUIRE_FALSE(Field::from_yaml(YAML::Load(""), fields));
        REQUIRE(fields.empty());

        fields.clear();
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("a_scalar"), fields));
        REQUIRE(fields.empty());
    }

    SECTION("Invalid map key") {
        fields.clear();
        std::string yaml = R"(
            not_a_number:
              name: "field_one"
              type: "i32"
        )";
        // parseFields returns true if *any* fields are parsed, but this one
        // should fail on the key and not add any.
        REQUIRE_FALSE(Field::from_yaml(YAML::Load(yaml), fields));
        REQUIRE(fields.empty());
    }

    SECTION("Empty map or sequence") {
        fields.clear();
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("{}"), fields));
        REQUIRE(fields.empty());

        fields.clear();
        REQUIRE_FALSE(Field::from_yaml(YAML::Load("[]"), fields));
        REQUIRE(fields.empty());
    }
}

TEST_CASE("Field JSON serialization (to/from_json)", "[job_field]") {
    Field f_in{};
    f_in.key = 42;
    f_in.name = "json_test";
    f_in.type = "bin[12]";
    f_in.kind = FieldKind::Bin; // Manually set for comparison
    f_in.required = true;
    f_in.size = 12; // Manually set for comparison
    f_in.ctype = "uint8_t*";
    f_in.comment = "A test comment";

    nlohmann::json j;
    Field::to_json(j, f_in);

    SECTION("to_json verification") {
        REQUIRE(j["key"] == 42);
        REQUIRE(j["name"] == "json_test");
        REQUIRE(j["type"] == "bin[12]");
        REQUIRE_FALSE(j.contains("kind"));
        REQUIRE(j["required"] == true);
        REQUIRE(j["size"] == 12);
        REQUIRE(j["ctype"] == "uint8_t*");
        REQUIRE(j["comment"] == "A test comment");
        REQUIRE_FALSE(j.contains("ref_include"));
        REQUIRE_FALSE(j.contains("ref_sym"));
    }

    SECTION("from_json round-trip verification") {
        Field f_out{};
        Field::from_json(j, f_out);

        // This works because from_json deduces the
        // 'kind' and 'size' to match f_in.
        REQUIRE(f_in == f_out);
    }

    SECTION("from_json minimal") {
        nlohmann::json j_minimal = {
            {"name", "min"},
            {"type", "str"}
        };
        Field f_min{};
        Field::from_json(j_minimal, f_min);

        REQUIRE(f_min.name == "min");
        REQUIRE(f_min.type == "str");
        REQUIRE(f_min.kind == FieldKind::Scalar); // Check deduced kind
        REQUIRE(f_min.key == 0);
        REQUIRE(f_min.required == false);
    }

    SECTION("from_json smart ref_sym deduction") {
        nlohmann::json j_ref = {
            {"name", "header"},
            {"type", "struct"},
            {"ref_include", "mpkg_msgpack_header.hpp"}
        };

        Field f_ref{};
        Field::from_json(j_ref, f_ref);

        REQUIRE(f_ref.ref_include == "mpkg_msgpack_header.hpp");
        REQUIRE(f_ref.ref_sym == "msgpack_header");
    }
}

TEST_CASE("Field::to_yaml emits correct YAML", "[job_field]") {
    Field f{};
    f.key = 1;
    f.name = "yaml_test";
    f.type = "str";
    f.required = true;
    f.comment = "YAML comment";

    YAML::Emitter emitter;
    Field::to_yaml(emitter, f);

    REQUIRE(emitter.good());
    std::string output = emitter.c_str();

    // Check for the presence of keys and values
    // NOTE: This is a simple string check, not a full YAML parse,
    // as YAML-CPP doesn't have an easy "get key"
    REQUIRE(output.find("key: 1") != std::string::npos);
    REQUIRE(output.find("name: yaml_test") != std::string::npos);
    REQUIRE(output.find("type: str") != std::string::npos);
    REQUIRE(output.find("required: true") != std::string::npos);
    REQUIRE(output.find("comment: YAML comment") != std::string::npos);

    // Check that optional-but-empty fields are not present
    REQUIRE(output.find("size:") == std::string::npos);
    REQUIRE(output.find("ctype:") == std::string::npos);
}

