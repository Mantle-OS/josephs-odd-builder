#include <catch2/catch_all.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <job_emitter_msgpack.h>
#include <job_field.h>
#include <job_serializer_msgpack.h>
#include <job_serializer_utils.h>
#include <job_util_msgpack.h>
#include <runtime_object.h>
#include <schema.h>

#include "../job_serializer/test_emitter.h"

using namespace job::serializer;
using namespace job::serializer::msg_pack;

TEST_CASE("JobSerializerMsgPack (Runtime) encode/decode round-trip", "[job_serializer_msgpack]")
{
    JobMsgPackSerializer ser{};
    Schema s = TestEmitter::getEmitterTestSchema();

    REQUIRE(s.isValid());

    RuntimeObject objIn{};

    objIn.setField("count", FieldValue{
                                .value = FieldValue::Scalar{static_cast<uint32_t>(123)}
                            });

    FieldValue::Binary binData = {0x01, 0x02, 0x03, 0x04};

    objIn.setField("data", FieldValue{
                               .value = binData
                           });

    FieldValue::Struct subItem;

    subItem["id"] = FieldValue{
        .value = FieldValue::Scalar{static_cast<int64_t>(42)}
    };

    subItem["name"] = FieldValue{
        .value = FieldValue::Scalar{std::string("test")}
    };

    FieldValue::List itemList;
    itemList.push_back(FieldValue{.value = subItem});

    objIn.setField("items", FieldValue{
                                .value = itemList
                            });

    std::vector<uint8_t> buffer;

    REQUIRE(ser.encode(s, objIn, buffer, SerializeFormat::Binary));
    REQUIRE_FALSE(buffer.empty());

    RuntimeObject objOut{};

    REQUIRE(ser.decode(s, objOut, buffer, SerializeFormat::Binary));

    SECTION("Verify decoded data")
    {
        REQUIRE(objOut.hasField("count"));

        auto countVal = objOut.getField("count");

        REQUIRE(countVal.has_value());
        REQUIRE(countVal->isScalar());
        REQUIRE(std::get<FieldValue::Scalar>(countVal->value) ==
                FieldValue::Scalar{static_cast<uint64_t>(123)});

        REQUIRE(objOut.hasField("data"));

        auto dataVal = objOut.getField("data");

        REQUIRE(dataVal.has_value());
        REQUIRE(dataVal->isBinary());
        REQUIRE(std::get<FieldValue::Binary>(dataVal->value) == binData);

        REQUIRE(objOut.hasField("names"));

        auto namesVal = objOut.getField("names");

        REQUIRE(namesVal.has_value());
        REQUIRE(namesVal->isNull());

        REQUIRE(objOut.hasField("items"));

        auto itemsVal = objOut.getField("items");

        REQUIRE(itemsVal.has_value());
        REQUIRE(itemsVal->isList());

        const auto &list = std::get<FieldValue::List>(itemsVal->value);

        REQUIRE(list.size() == 1);
        REQUIRE(list[0].isStruct());

        const auto &map = std::get<FieldValue::Struct>(list[0].value);

        REQUIRE(map.at("id").isScalar());
        REQUIRE(map.at("name").isScalar());

        REQUIRE(std::get<FieldValue::Scalar>(map.at("id").value) ==
                FieldValue::Scalar{static_cast<uint64_t>(42)});

        REQUIRE(std::get<FieldValue::Scalar>(map.at("name").value) ==
                FieldValue::Scalar{std::string("test")});
    }
}