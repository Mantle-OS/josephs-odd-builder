#include <catch2/catch_all.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <iserializer.h>
#include <job_serializer_utils.h>
#include <runtime_object.h>
#include <schema.h>

#include "test_emitter.h"

using namespace job::serializer;

TEST_CASE("ISerializer (base class) default JSON encode/decode", "[iserializer]")
{
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

    RuntimeObject objIn;

    FieldValue::Struct subItem;
    subItem["id"] = FieldValue{
        .value = FieldValue::Scalar{static_cast<uint32_t>(1)}
    };

    FieldValue::List listOfItems;
    listOfItems.push_back(FieldValue{.value = subItem});

    objIn.setField("items", FieldValue{.value = listOfItems});
    objIn.setField("count", FieldValue{
                                .value = FieldValue::Scalar{static_cast<uint32_t>(42)}
                            });
    objIn.setField("bogus_field", FieldValue{
                                      .value = FieldValue::Scalar{static_cast<int32_t>(-1)}
                                  });

    std::vector<uint8_t> buffer;

    SECTION("Encode: Full Recursive Object")
    {
        REQUIRE(ser.encode(s, objIn, buffer, SerializeFormat::Json));
        REQUIRE_FALSE(buffer.empty());

        std::string jsonStr(buffer.begin(), buffer.end());

        REQUIRE(stringContains(jsonStr, "\"count\": 42"));
        REQUIRE_FALSE(stringContains(jsonStr, "bogus_field"));

        REQUIRE(stringContains(jsonStr, "\"items\": ["));
        REQUIRE(stringContains(jsonStr, "\"id\": 1"));
    }

    SECTION("Decode: Full Recursive Object")
    {
        const std::string jsonStr = R"({
            "count": 123,
            "items": [
                { "id": 10, "name": "first" },
                { "id": 20, "name": "second" }
            ],
            "bogus_field": "this-should-be-ignored-by-decoder"
        })";

        std::vector<uint8_t> inBuf(jsonStr.begin(), jsonStr.end());
        RuntimeObject objOut;

        REQUIRE(ser.decode(s, objOut, inBuf, SerializeFormat::Json));

        REQUIRE(objOut.hasField("count"));
        REQUIRE(objOut.hasField("items"));
        REQUIRE_FALSE(objOut.hasField("bogus_field"));

        auto countVal = objOut.getField("count");

        REQUIRE(countVal.has_value());
        REQUIRE(countVal->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(countVal->value) ==
                FieldValue::Scalar{static_cast<int64_t>(123)});

        auto itemsVal = objOut.getField("items");

        REQUIRE(itemsVal.has_value());
        REQUIRE(itemsVal->isList());

        auto &listVec = std::get<FieldValue::List>(itemsVal->value);

        REQUIRE(listVec.size() == 2);

        auto item1 = listVec[0];

        REQUIRE(item1.isStruct());

        auto &item1Map = std::get<FieldValue::Struct>(item1.value);

        REQUIRE(item1Map.count("id"));
        REQUIRE(item1Map.count("name"));
        REQUIRE(std::get<FieldValue::Scalar>(item1Map["id"].value) ==
                FieldValue::Scalar{static_cast<int64_t>(10)});
    }
}

TEST_CASE("ISerializer (base class) default YAML encode/decode", "[iserializer]")
{
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

    RuntimeObject objIn;

    FieldValue::Struct subItem;
    subItem["id"] = FieldValue{
        .value = FieldValue::Scalar{static_cast<uint32_t>(1)}
    };
    subItem["name"] = FieldValue{
        .value = FieldValue::Scalar{std::string("test_item")}
    };

    FieldValue::List listOfItems;
    listOfItems.push_back(FieldValue{.value = subItem});

    objIn.setField("items", FieldValue{.value = listOfItems});
    objIn.setField("count", FieldValue{
                                .value = FieldValue::Scalar{static_cast<uint32_t>(42)}
                            });
    objIn.setField("bogus_field", FieldValue{
                                      .value = FieldValue::Scalar{static_cast<int32_t>(-1)}
                                  });

    std::vector<uint8_t> buffer;

    SECTION("Encode: Full Recursive Object (YAML)")
    {
        REQUIRE(ser.encode(s, objIn, buffer, SerializeFormat::Yaml));
        REQUIRE_FALSE(buffer.empty());

        std::string yamlStr(buffer.begin(), buffer.end());

        REQUIRE(stringContains(yamlStr, "count: 42"));
        REQUIRE_FALSE(stringContains(yamlStr, "bogus_field"));

        REQUIRE(stringContains(yamlStr, "items:"));
        REQUIRE(stringContains(yamlStr, "id: 1"));
        REQUIRE(stringContains(yamlStr, "name: test_item"));
    }

    SECTION("Decode: Full Recursive Object (YAML)")
    {
        const std::string yamlStr = R"(
count: 123
items:
  - id: 10
    name: first
  - id: 20
    name: second
bogus_field: this-should-be-ignored-by-decoder)";

        std::vector<uint8_t> inBuf(yamlStr.begin(), yamlStr.end());
        RuntimeObject objOut;

        REQUIRE(ser.decode(s, objOut, inBuf, SerializeFormat::Yaml));

        REQUIRE(objOut.hasField("count"));
        REQUIRE(objOut.hasField("items"));
        REQUIRE_FALSE(objOut.hasField("bogus_field"));

        auto countVal = objOut.getField("count");

        REQUIRE(countVal.has_value());
        REQUIRE(countVal->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(countVal->value) ==
                FieldValue::Scalar{std::string("123")});

        auto itemsVal = objOut.getField("items");

        REQUIRE(itemsVal.has_value());
        REQUIRE(itemsVal->isList());

        auto &listVec = std::get<FieldValue::List>(itemsVal->value);

        REQUIRE(listVec.size() == 2);

        auto item1 = listVec[0];

        REQUIRE(item1.isStruct());

        auto &item1Map = std::get<FieldValue::Struct>(item1.value);

        REQUIRE(item1Map.count("id"));
        REQUIRE(item1Map.count("name"));
        REQUIRE(std::get<FieldValue::Scalar>(item1Map["id"].value) ==
                FieldValue::Scalar{std::string("10")});
        REQUIRE(std::get<FieldValue::Scalar>(item1Map["name"].value) ==
                FieldValue::Scalar{std::string("first")});
    }
}