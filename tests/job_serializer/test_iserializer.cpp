#include <catch2/catch_all.hpp>

#include <string>

#include <job_serializer_utils.h>
#include <runtime_object.h>
#include <schema.h>

#include <iserializer.h>

#include "test_emitter.h"

using namespace job::serializer;

TEST_CASE("ISerializer (base class) default JSON encode/decode", "[iserializer]")
{
    // We are now testing the BASE class, which *is* the BaseSerializer
    ISerializer ser{};
    Schema s = TestEmitter::getEmitterTestSchema();
    s.fields.push_back({
        .key = 100,
        .name = "items",
        .type = "list<struct>",
        .kind = FieldKind::ListStruct,
        .size{},
        .ctype{},
        .ref_include{},
        .ref_sym = "MyTestItem",
        .required = false,
        .comment{}
    });

    RuntimeObject obj_in;

    FieldValue::Struct sub_item;
    sub_item["id"] = FieldValue{ .value = FieldValue::Scalar{ (uint32_t)1 } };

    FieldValue::List list_of_items;
    list_of_items.push_back(FieldValue{ .value = sub_item });

    obj_in.setField("items", FieldValue{.value = list_of_items });
    obj_in.setField("count", FieldValue{ .value = FieldValue::Scalar{ (uint32_t)42 } });
    obj_in.setField("bogus_field", FieldValue{ .value = FieldValue::Scalar{ (int32_t)-1 } });

    std::vector<uint8_t> buffer;

    SECTION("Encode: Full Recursive Object"){
        // Use the base class's router, which will call encodeJson
        REQUIRE(ser.encode(s, obj_in, buffer, SerializeFormat::Json));
        REQUIRE_FALSE(buffer.empty());

        std::string json_str(buffer.begin(), buffer.end());

        REQUIRE(stringContains(json_str, "\"count\": 42"));
        REQUIRE_FALSE(stringContains(json_str, "bogus_field"));

        // Check for our recursive list/struct
        REQUIRE(stringContains(json_str, "\"items\": ["));
        REQUIRE(stringContains(json_str, "\"id\": 1"));
    }

    SECTION("Decode: Full Recursive Object"){
        std::string json_str = R"( {
            "count": 123,
            "items": [
                { "id": 10, "name": "first" },
                { "id": 20, "name": "second" }
            ],
            "bogus_field": "this-should-be-ignored-by-decoder"
        })";
        std::vector<uint8_t> in_buf(json_str.begin(), json_str.end());
        RuntimeObject obj_out;

        REQUIRE(ser.decode(s, obj_out, in_buf, SerializeFormat::Json));

        REQUIRE(obj_out.hasField("count"));
        REQUIRE(obj_out.hasField("items"));
        REQUIRE_FALSE(obj_out.hasField("bogus_field"));

        auto count_val = obj_out.getField("count");
        REQUIRE(count_val->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(count_val->value) == FieldValue::Scalar{ (int64_t)123 });

        auto items_val = obj_out.getField("items");
        REQUIRE(items_val->isList());

        auto& list_vec = std::get<FieldValue::List>(items_val->value);
        REQUIRE(list_vec.size() == 2);

        auto item_1 = list_vec[0];
        REQUIRE(item_1.isStruct());
        auto& item_1_map = std::get<FieldValue::Struct>(item_1.value);
        REQUIRE(item_1_map.count("id"));
        REQUIRE(item_1_map.count("name"));
        REQUIRE(std::get<FieldValue::Scalar>(item_1_map["id"].value) == FieldValue::Scalar{ (int64_t)10 });
    }
}

TEST_CASE("ISerializer (base class) default YAML encode/decode", "[iserializer]"){
    ISerializer ser{};
    Schema s = TestEmitter::getEmitterTestSchema();
    s.fields.push_back({
        .key = 100,
        .name = "items",
        .type = "list<struct>",
        .kind = FieldKind::ListStruct,
        .size{},
        .ctype{},
        .ref_include{},
        .ref_sym = "MyTestItem",
        .required = false,
        .comment{}
    });

    RuntimeObject obj_in;
    FieldValue::Struct sub_item;
    sub_item["id"] = FieldValue{ .value = FieldValue::Scalar{ (uint32_t)1 } };
    sub_item["name"] = FieldValue{ .value = FieldValue::Scalar{ std::string("test_item") } };

    FieldValue::List list_of_items;
    list_of_items.push_back(FieldValue{ .value = sub_item });

    obj_in.setField("items", FieldValue{ .value = list_of_items });

    obj_in.setField("count", FieldValue{ .value = FieldValue::Scalar{ (uint32_t)42 } });
    obj_in.setField("bogus_field", FieldValue{ .value = FieldValue::Scalar{ (int32_t)-1 } });

    std::vector<uint8_t> buffer;

    SECTION("Encode: Full Recursive Object (YAML)") {
        REQUIRE(ser.encode(s, obj_in, buffer, SerializeFormat::Yaml));
        REQUIRE_FALSE(buffer.empty());

        std::string yaml_str(buffer.begin(), buffer.end());
        REQUIRE(stringContains(yaml_str, "count: 42"));
        REQUIRE_FALSE(stringContains(yaml_str, "bogus_field"));

        REQUIRE(stringContains(yaml_str, "items:"));
        REQUIRE(stringContains(yaml_str, "id: 1"));
        REQUIRE(stringContains(yaml_str, "name: test_item"));
    }

    SECTION("Decode: Full Recursive Object (YAML)") {
        std::string yaml_str = R"(
count: 123
items:
  - id: 10
    name: first
  - id: 20
    name: second
bogus_field: this-should-be-ignored-by-decoder)";
        std::vector<uint8_t> in_buf(yaml_str.begin(), yaml_str.end());
        RuntimeObject obj_out;

        REQUIRE(ser.decode(s, obj_out, in_buf, SerializeFormat::Yaml));

        REQUIRE(obj_out.hasField("count"));
        REQUIRE(obj_out.hasField("items"));
        REQUIRE_FALSE(obj_out.hasField("bogus_field"));

        auto count_val = obj_out.getField("count");
        REQUIRE(count_val->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(count_val->value) == FieldValue::Scalar{ std::string("123") });

        auto items_val = obj_out.getField("items");
        REQUIRE(items_val->isList());

        auto& list_vec = std::get<FieldValue::List>(items_val->value);
        REQUIRE(list_vec.size() == 2);

        auto item_1 = list_vec[0];
        REQUIRE(item_1.isStruct());
        auto& item_1_map = std::get<FieldValue::Struct>(item_1.value);
        REQUIRE(item_1_map.count("id"));
        REQUIRE(item_1_map.count("name"));
        REQUIRE(std::get<FieldValue::Scalar>(item_1_map["id"].value) == FieldValue::Scalar{ std::string("10") });
        REQUIRE(std::get<FieldValue::Scalar>(item_1_map["name"].value) == FieldValue::Scalar{ std::string("first") });
    }
}
