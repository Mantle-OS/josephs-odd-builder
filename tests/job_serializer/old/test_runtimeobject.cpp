#include <catch2/catch_test_macros.hpp>
#include <string>
#include <variant>
#include <vector>

#include <unordered_map>

#include <runtime_object.h>
#include <job_logger.h>

using namespace job::serializer;
TEST_CASE("RuntimeObject field management", "[runtime][fields]")
{
    // Set up a RuntimeObject instance
    RuntimeObject obj;

    // Test setting and getting fields
    FieldValue scalarValue;
    scalarValue.value = FieldValue::Scalar{int32_t{42}};

    FieldValue binaryValue;
    binaryValue.value = FieldValue::Binary{std::vector<uint8_t>{1, 2, 3, 4}};

    FieldValue listValue;
    listValue.value = FieldValue::List{
        FieldValue{FieldValue::Scalar{int32_t{10}}},
        FieldValue{FieldValue::Scalar{int32_t{20}}}
    };

    FieldValue structValue;
    structValue.value = FieldValue::Struct{
        {"field1", FieldValue{FieldValue::Scalar{std::string{"hello"}}}},
        {"field2", FieldValue{FieldValue::Scalar{std::string{"world"}}}}
    };

    // Set fields
    REQUIRE(obj.setField("scalar_field", scalarValue));
    REQUIRE(obj.setField("binary_field", binaryValue));
    REQUIRE(obj.setField("list_field", listValue));
    REQUIRE(obj.setField("struct_field", structValue));

    // Test getting fields
    REQUIRE(obj.hasField("scalar_field"));
    REQUIRE(obj.hasField("binary_field"));
    REQUIRE(obj.hasField("list_field"));
    REQUIRE(obj.hasField("struct_field"));

    // Retrieve the field values and validate
    auto scalar = obj.getField("scalar_field");
    REQUIRE(scalar.has_value());

    // Ensure it holds the expected type
    auto* scalarInt = std::get_if<FieldValue::Scalar>(&scalar->value);  // Get the Scalar variant
    REQUIRE(scalarInt != nullptr);  // Ensure the Scalar variant is not empty

    // Now access the specific type (int32_t in this case) inside the Scalar variant

    // WHAT dont you get about that there is no "value" in scalarInt as it is a "std::variant"
    auto* valueInt = std::get_if<int32_t>(scalarInt); // Extract int32_t directly from Scalar
    REQUIRE(valueInt != nullptr);  // Ensure it holds an int32_t
    REQUIRE(*valueInt == 42);  // Compare with 42


    auto binary = obj.getField("binary_field");
    REQUIRE(binary.has_value());

    // Extract and compare the Binary value properly
    auto* binaryData = std::get_if<FieldValue::Binary>(&binary->value);
    REQUIRE(binaryData != nullptr);
    REQUIRE(*binaryData == std::vector<uint8_t>{1, 2, 3, 4});

    // Test removing fields
    REQUIRE(obj.removeField("scalar_field"));
    REQUIRE_FALSE(obj.hasField("scalar_field"));

    // Ensure removing a non-existing field does not cause an issue
    REQUIRE_FALSE(obj.removeField("non_existent_field"));

    // Test clearing all fields
    obj.clear();
    REQUIRE(obj.fields().empty());



    SECTION("Supports all scalar types")
    {
        std::vector<FieldValue> scalars = {
            FieldValue{FieldValue::Scalar{int32_t{-42}}},
            FieldValue{FieldValue::Scalar{uint32_t{42u}}},
            FieldValue{FieldValue::Scalar{int64_t{-42000}}},
            FieldValue{FieldValue::Scalar{uint64_t{42000u}}},
            FieldValue{FieldValue::Scalar{std::string{"hello"}}}
        };

        for (size_t i = 0; i < scalars.size(); ++i) {
            std::string name = "scalar_" + std::to_string(i);
            REQUIRE(obj.setField(name, scalars[i]));
            auto val = obj.getField(name);
            REQUIRE(val.has_value());
            REQUIRE(val->isScalar());
        }
    }

    SECTION("Handles deep nesting correctly")
    {
        FieldValue deep;
        deep.value = FieldValue::Struct{
            {"lvl1", FieldValue{FieldValue::Struct{
                         {"lvl2", FieldValue{FieldValue::List{
                                      FieldValue{FieldValue::Scalar{std::string{"deep_value"}}}
                                  }}}
                     }}}
        };
        REQUIRE(obj.setField("deep", deep));

        auto val = obj.getField("deep");
        REQUIRE(val.has_value());
        REQUIRE(val->isStruct());
    }

    SECTION("Supports overwrite and mutation")
    {
        FieldValue f1{FieldValue::Scalar{std::string{"one"}}};
        FieldValue f2{FieldValue::Binary{std::vector<uint8_t>{7, 8, 9}}};

        REQUIRE(obj.setField("mutate", f1));
        auto v1 = obj.getField("mutate");
        REQUIRE(v1->isScalar());

        REQUIRE(obj.setField("mutate", f2));
        auto v2 = obj.getField("mutate");
        REQUIRE(v2->isBinary());
    }

    SECTION("Handles clearing and invalid operations safely")
    {
        REQUIRE_FALSE(obj.setField("", FieldValue{}));  // empty name
        REQUIRE_FALSE(obj.removeField("missing_field"));
        obj.clear();
        REQUIRE(obj.fields().empty());
    }
}
// CHECKPOINT: v1
