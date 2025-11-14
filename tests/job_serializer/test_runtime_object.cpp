#include <catch2/catch_all.hpp>

#include <string>

#include <job_serializer_utils.h>
#include <runtime_object.h>

using namespace job::serializer;

TEST_CASE("FieldValue type checkers (isNull, isScalar, etc.)", "[runtime_object][field_value]")
{
    FieldValue fv_null;
    fv_null.value = std::monostate{};
    REQUIRE(fv_null.isNull());
    REQUIRE_FALSE(fv_null.isScalar());

    FieldValue fv_scalar;
    fv_scalar.value = FieldValue::Scalar{ (uint32_t)123 };
    REQUIRE(fv_scalar.isScalar());
    REQUIRE_FALSE(fv_scalar.isNull());

    FieldValue fv_bin;
    fv_bin.value = FieldValue::Binary{ 0xDE, 0xAD };
    REQUIRE(fv_bin.isBinary());
    REQUIRE_FALSE(fv_bin.isScalar());

    FieldValue fv_list;
    fv_list.value = FieldValue::List{};
    REQUIRE(fv_list.isList());
    REQUIRE_FALSE(fv_list.isBinary());

    FieldValue fv_struct;
    fv_struct.value = FieldValue::Struct{};
    REQUIRE(fv_struct.isStruct());
    REQUIRE_FALSE(fv_struct.isList());
}

TEST_CASE("RuntimeObject API (set, get, has, remove)", "[runtime_object]")
{
    RuntimeObject obj;

    FieldValue val_scalar;
    val_scalar.value = FieldValue::Scalar{ std::string("hello") };

    FieldValue val_bin;
    val_bin.value = FieldValue::Binary{ 0x01, 0x02, 0x03 };

    FieldValue val_i32;
    val_i32.value = FieldValue::Scalar{ (int32_t)-42 };


    SECTION("setField and hasField")
    {
        REQUIRE_FALSE(obj.hasField("scalar_field"));
        REQUIRE(obj.setField("scalar_field", val_scalar));
        REQUIRE(obj.hasField("scalar_field"));

        REQUIRE(obj.setField("bin_field", val_bin));
        REQUIRE(obj.hasField("bin_field"));

        REQUIRE_FALSE(obj.setField("", val_scalar));
    }

    SECTION("getField")
    {
        obj.setField("scalar_field", val_scalar);
        obj.setField("i32_field", val_i32);

        auto opt_val = obj.getField("scalar_field");
        REQUIRE(opt_val.has_value());
        REQUIRE(opt_val->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(opt_val->value) == FieldValue::Scalar{ std::string("hello") });

        auto opt_i32 = obj.getField("i32_field");
        REQUIRE(opt_i32.has_value());
        REQUIRE(opt_i32->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(opt_i32->value) == FieldValue::Scalar{ (int32_t)-42 });

        auto opt_none = obj.getField("no_such_field");
        REQUIRE_FALSE(opt_none.has_value());

        auto opt_empty = obj.getField("");
        REQUIRE_FALSE(opt_empty.has_value());
    }

    SECTION("removeField")
    {
        obj.setField("scalar_field", val_scalar);
        REQUIRE(obj.hasField("scalar_field"));

        REQUIRE(obj.removeField("scalar_field"));
        REQUIRE_FALSE(obj.hasField("scalar_field"));

        REQUIRE_FALSE(obj.removeField("scalar_field"));

        REQUIRE_FALSE(obj.removeField(""));
    }

    SECTION("clear")
    {
        obj.setField("scalar_field", val_scalar);
        obj.setField("bin_field", val_bin);
        REQUIRE(obj.fields().size() == 2);

        obj.clear();
        REQUIRE(obj.fields().empty());
        REQUIRE_FALSE(obj.hasField("scalar_field"));
    }

    SECTION("fields (const and non-const)")
    {
        obj.setField("field1", val_scalar);

        // Test non-const getter
        obj.fields()["field2"] = val_i32;
        REQUIRE(obj.hasField("field2"));

        // Test const getter
        const RuntimeObject& const_obj = obj;
        REQUIRE(const_obj.fields().size() == 2);
        REQUIRE(const_obj.fields().at("field1").isScalar());
    }

    SECTION("Recursive Structs and Lists (Complex Test)")
    {
        RuntimeObject root;

        FieldValue nested_struct_val;
        FieldValue::Struct nested_struct;

        FieldValue nested_field_val;
        nested_field_val.value = FieldValue::Scalar{ (uint64_t)999 };
        nested_struct["nested_id"] = nested_field_val;

        nested_struct_val.value = nested_struct; // Assign map to variant
        REQUIRE(nested_struct_val.isStruct());

        root.setField("my_struct", nested_struct_val);

        FieldValue list_val;
        FieldValue::List scalar_list;

        FieldValue list_item_1;
        list_item_1.value = FieldValue::Scalar{ std::string("item1") };
        scalar_list.push_back(list_item_1);

        FieldValue list_item_2;
        list_item_2.value = FieldValue::Scalar{ std::string("item2") };
        scalar_list.push_back(list_item_2);

        list_val.value = scalar_list; // Assign vector to variant
        REQUIRE(list_val.isList());

        root.setField("my_list", list_val);

        REQUIRE(root.hasField("my_struct"));
        REQUIRE(root.hasField("my_list"));

        auto opt_struct = root.getField("my_struct");
        REQUIRE(opt_struct.has_value());
        REQUIRE(opt_struct->isStruct());

        auto& retrieved_struct_map = std::get<FieldValue::Struct>(opt_struct->value);
        REQUIRE(retrieved_struct_map.count("nested_id") == 1);
        REQUIRE(retrieved_struct_map.at("nested_id").isScalar());

        auto opt_list = root.getField("my_list");
        REQUIRE(opt_list.has_value());
        REQUIRE(opt_list->isList());

        auto& retrieved_list_vec = std::get<FieldValue::List>(opt_list->value);
        REQUIRE(retrieved_list_vec.size() == 2);
        REQUIRE(retrieved_list_vec[0].isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(retrieved_list_vec[1].value) == FieldValue::Scalar{ std::string("item2") });
    }
}
// CHECKPOINT: v1
