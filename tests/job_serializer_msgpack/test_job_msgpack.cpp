#include <catch2/catch_template_test_macros.hpp>

#include <string>

// base serializer lib
#include <schema.h>
#include <job_serializer_utils.h>
#include <job_field.h>
#include <runtime_object.h>

// message pack plugin lib
#include <job_emitter_msgpack.h>
#include <job_serializer_msgpack.h>
#include <job_util_msgpack.h>

#include "../job_serializer/test_emitter.h"

using namespace job::serializer;
using namespace job::serializer::msg_pack;

TEST_CASE("JobSerializerMsgPack (Runtime) encode/decode round-trip", "[job_serializer_msgpack]")
{
    JobMsgPackSerializer ser{};
    Schema s = TestEmitter::getEmitterTestSchema();
    REQUIRE(s.isValid());

    RuntimeObject obj_in{};
    obj_in.setField("count", FieldValue{ .value = FieldValue::Scalar{ (uint32_t)123 } });

    FieldValue::Binary bin_data = { 0x01, 0x02, 0x03, 0x04 };
    obj_in.setField("data", FieldValue{ .value = bin_data });

    FieldValue::Struct sub_item;
    sub_item["id"] = FieldValue{ .value = FieldValue::Scalar{ (int64_t)42 } };
    sub_item["name"] = FieldValue{ .value = FieldValue::Scalar{ std::string("test") } };

    FieldValue::List item_list;
    item_list.push_back(FieldValue{ .value = sub_item });

    obj_in.setField("items", FieldValue{ .value = item_list });

    std::vector<uint8_t> buffer;
    REQUIRE(ser.encode(s, obj_in, buffer, SerializeFormat::Binary));
    REQUIRE_FALSE(buffer.empty());

    RuntimeObject obj_out{};
    REQUIRE(ser.decode(s, obj_out, buffer, SerializeFormat::Binary));

    SECTION("Verify decoded data"){
        REQUIRE(obj_out.hasField("count"));
        auto count_val = obj_out.getField("count");
        REQUIRE(count_val->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(count_val->value) == FieldValue::Scalar{(uint64_t)123});

        REQUIRE(obj_out.hasField("data"));
        auto data_val = obj_out.getField("data");
        REQUIRE(data_val->isBinary());
        REQUIRE(std::get<FieldValue::Binary>(data_val->value) == bin_data);

        REQUIRE(obj_out.hasField("names"));
        REQUIRE(obj_out.getField("names")->isNull());

        REQUIRE(obj_out.hasField("items"));
        auto items_val = obj_out.getField("items");
        REQUIRE(items_val->isList());

        const auto &list = std::get<FieldValue::List>(items_val->value);
        REQUIRE(list.size() == 1);
        REQUIRE(list[0].isStruct());

        const auto &map = std::get<FieldValue::Struct>(list[0].value);
        REQUIRE(map.at("id").isScalar());
        REQUIRE(map.at("name").isScalar());

        REQUIRE(std::get<FieldValue::Scalar>(map.at("id").value) == FieldValue::Scalar{(uint64_t)42});
        REQUIRE(std::get<FieldValue::Scalar>(map.at("name").value) == FieldValue::Scalar{std::string("test")});
    }
}
