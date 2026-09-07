#include <catch2/catch_all.hpp>

#include <cstdint>
#include <string>

#include <job_serializer_utils.h>
#include <runtime_object.h>

using namespace job::serializer;

TEST_CASE("FieldValue type checkers (isNull, isScalar, etc.)", "[runtime_object][field_value]")
{
    FieldValue fvNull;
    fvNull.value = std::monostate{};

    REQUIRE(fvNull.isNull());
    REQUIRE_FALSE(fvNull.isScalar());

    FieldValue fvScalar;
    fvScalar.value = FieldValue::Scalar{static_cast<uint32_t>(123)};

    REQUIRE(fvScalar.isScalar());
    REQUIRE_FALSE(fvScalar.isNull());

    FieldValue fvBin;
    fvBin.value = FieldValue::Binary{0xDE, 0xAD};

    REQUIRE(fvBin.isBinary());
    REQUIRE_FALSE(fvBin.isScalar());

    FieldValue fvList;
    fvList.value = FieldValue::List{};

    REQUIRE(fvList.isList());
    REQUIRE_FALSE(fvList.isBinary());

    FieldValue fvStruct;
    fvStruct.value = FieldValue::Struct{};

    REQUIRE(fvStruct.isStruct());
    REQUIRE_FALSE(fvStruct.isList());
}

TEST_CASE("RuntimeObject API (set, get, has, remove)", "[runtime_object]")
{
    RuntimeObject obj;

    FieldValue valScalar;
    valScalar.value = FieldValue::Scalar{std::string("hello")};

    FieldValue valBin;
    valBin.value = FieldValue::Binary{0x01, 0x02, 0x03};

    FieldValue valI32;
    valI32.value = FieldValue::Scalar{static_cast<int32_t>(-42)};

    SECTION("setField and hasField")
    {
        REQUIRE_FALSE(obj.hasField("scalar_field"));
        REQUIRE(obj.setField("scalar_field", valScalar));
        REQUIRE(obj.hasField("scalar_field"));

        REQUIRE(obj.setField("bin_field", valBin));
        REQUIRE(obj.hasField("bin_field"));

        REQUIRE_FALSE(obj.setField("", valScalar));
    }

    SECTION("getField")
    {
        obj.setField("scalar_field", valScalar);
        obj.setField("i32_field", valI32);

        auto optVal = obj.getField("scalar_field");

        REQUIRE(optVal.has_value());
        REQUIRE(optVal->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(optVal->value) == FieldValue::Scalar{std::string("hello")});

        auto optI32 = obj.getField("i32_field");

        REQUIRE(optI32.has_value());
        REQUIRE(optI32->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(optI32->value) == FieldValue::Scalar{static_cast<int32_t>(-42)});

        auto optNone = obj.getField("no_such_field");

        REQUIRE_FALSE(optNone.has_value());

        auto optEmpty = obj.getField("");

        REQUIRE_FALSE(optEmpty.has_value());
    }

    SECTION("removeField")
    {
        obj.setField("scalar_field", valScalar);

        REQUIRE(obj.hasField("scalar_field"));
        REQUIRE(obj.removeField("scalar_field"));
        REQUIRE_FALSE(obj.hasField("scalar_field"));
        REQUIRE_FALSE(obj.removeField("scalar_field"));
        REQUIRE_FALSE(obj.removeField(""));
    }

    SECTION("clear")
    {
        obj.setField("scalar_field", valScalar);
        obj.setField("bin_field", valBin);

        REQUIRE(obj.fields().size() == 2);

        obj.clear();

        REQUIRE(obj.fields().empty());
        REQUIRE_FALSE(obj.hasField("scalar_field"));
    }

    SECTION("fields (const and non-const)")
    {
        obj.setField("field1", valScalar);

        obj.fields()["field2"] = valI32;

        REQUIRE(obj.hasField("field2"));

        const RuntimeObject &constObj = obj;

        REQUIRE(constObj.fields().size() == 2);
        REQUIRE(constObj.fields().at("field1").isScalar());
    }

    SECTION("Recursive Structs and Lists (Complex Test)")
    {
        RuntimeObject root;

        FieldValue nestedStructVal;
        FieldValue::Struct nestedStruct;

        FieldValue nestedFieldVal;
        nestedFieldVal.value = FieldValue::Scalar{static_cast<uint64_t>(999)};
        nestedStruct["nested_id"] = nestedFieldVal;

        nestedStructVal.value = nestedStruct;

        REQUIRE(nestedStructVal.isStruct());

        root.setField("my_struct", nestedStructVal);

        FieldValue listVal;
        FieldValue::List scalarList;

        FieldValue listItem1;
        listItem1.value = FieldValue::Scalar{std::string("item1")};
        scalarList.push_back(listItem1);

        FieldValue listItem2;
        listItem2.value = FieldValue::Scalar{std::string("item2")};
        scalarList.push_back(listItem2);

        listVal.value = scalarList;

        REQUIRE(listVal.isList());

        root.setField("my_list", listVal);

        REQUIRE(root.hasField("my_struct"));
        REQUIRE(root.hasField("my_list"));

        auto optStruct = root.getField("my_struct");

        REQUIRE(optStruct.has_value());
        REQUIRE(optStruct->isStruct());

        auto &retrievedStructMap = std::get<FieldValue::Struct>(optStruct->value);

        REQUIRE(retrievedStructMap.count("nested_id") == 1);
        REQUIRE(retrievedStructMap.at("nested_id").isScalar());

        auto optList = root.getField("my_list");

        REQUIRE(optList.has_value());
        REQUIRE(optList->isList());

        auto &retrievedListVec = std::get<FieldValue::List>(optList->value);

        REQUIRE(retrievedListVec.size() == 2);
        REQUIRE(retrievedListVec[0].isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(retrievedListVec[1].value) ==
                FieldValue::Scalar{std::string("item2")});
    }
}