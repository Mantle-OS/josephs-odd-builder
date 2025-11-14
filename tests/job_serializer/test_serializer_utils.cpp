#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

#include <job_serializer_utils.h>

using namespace job::serializer;

TEST_CASE("deduceFieldKind correctly identifies all FieldKinds", "[serializer_utils]") {

    SECTION("Scalar types") {
        REQUIRE(deduceFieldKind("str") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("i32") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("u32") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("i64") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("u64") == FieldKind::Scalar);
    }

    SECTION("Binary types") {
        REQUIRE(deduceFieldKind("bin") == FieldKind::Bin);
        REQUIRE(deduceFieldKind("bin[32]") == FieldKind::Bin);
        REQUIRE(deduceFieldKind("bin[1024]") == FieldKind::Bin);
    }

    SECTION("Struct type") {
        REQUIRE(deduceFieldKind("struct") == FieldKind::Struct);
    }

    SECTION("List types") {
        REQUIRE(deduceFieldKind("list<str>") == FieldKind::ListScalar);
        REQUIRE(deduceFieldKind("list<bin>") == FieldKind::ListBin);
        REQUIRE(deduceFieldKind("list<bin[64]>") == FieldKind::ListBin);
        REQUIRE(deduceFieldKind("list<struct>") == FieldKind::ListStruct);
    }

    SECTION("Unknown types default to Scalar") {
        // IMPORANT This should return the default (Scalar) and log a warning
        // as per your implementation.
        REQUIRE(deduceFieldKind("f32") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("double") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("") == FieldKind::Scalar);
        REQUIRE(deduceFieldKind("list<f32>") == FieldKind::Scalar);
    }
}

TEST_CASE("jsonToScalar converts FieldValue::Scalar to nlohmann::json", "[serializer_utils]") {

    SECTION("int32_t") {
        FieldValue::Scalar scalar = (int32_t)-123;
        nlohmann::json j = jsonToScalar(scalar);
        REQUIRE(j.is_number_integer());
        REQUIRE(j.get<int32_t>() == -123);
    }

    SECTION("uint32_t") {
        FieldValue::Scalar scalar = (uint32_t)456;
        nlohmann::json j = jsonToScalar(scalar);
        REQUIRE(j.is_number_unsigned());
        REQUIRE(j.get<uint32_t>() == 456);
    }

    SECTION("int64_t") {
        FieldValue::Scalar scalar = (int64_t)-7890123456;
        nlohmann::json j = jsonToScalar(scalar);
        REQUIRE(j.is_number_integer());
        REQUIRE(j.get<int64_t>() == -7890123456);
    }

    SECTION("uint64_t") {
        FieldValue::Scalar scalar = (uint64_t)9876543210;
        nlohmann::json j = jsonToScalar(scalar);
        REQUIRE( (j.is_number_unsigned() || j.is_number_integer()) );
        REQUIRE(j.get<uint64_t>() == 9876543210);
    }

    SECTION("std::string") {
        FieldValue::Scalar scalar = std::string("Hello, JOB!");
        nlohmann::json j = jsonToScalar(scalar);
        REQUIRE(j.is_string());
        REQUIRE(j.get<std::string>() == "Hello, JOB!");
    }
}
// CHECKPOINT: v1
