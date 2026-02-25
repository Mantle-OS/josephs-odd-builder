#include <catch2/catch_all.hpp>

#include <string>

#include <schema.h>
#include <runtime_object.h>

#include <job_emitter_msgpack.h>

#include "../job_serializer/test_emitter.h"
using namespace job::serializer;
using namespace job::serializer::msg_pack;

TEST_CASE("JobEmitterMsgPack generates pack/unpack functions", "[job_emitter_msgpack]")
{
    JobEmitterMsgPack emitter{};
    Schema s = TestEmitter::getEmitterTestSchema();
    REQUIRE(s.isValid());

    auto [header, source] = emitter.render(s);

    REQUIRE_FALSE(header.empty());
    REQUIRE_FALSE(source.empty());

    SECTION("Header content verification")
    {
        REQUIRE(stringContains(header, "struct EmitterTest_t {"));

        REQUIRE(stringContains(header, "uint32_t count;"));

        REQUIRE(stringContains(header, "#include <msgpack.hpp>"));
        REQUIRE(stringContains(header, "void pack_msgpack(msgpack::packer<msgpack::sbuffer> &pk) const;"));
        REQUIRE(stringContains(header, "void unpack_msgpack(const msgpack::object &obj);"));

        REQUIRE_FALSE(stringContains(header, "pk.pack_map(6);"));
    }


    SECTION("Source content verification")
    {
        REQUIRE(stringContains(source, "#include \"emitter_test_msg.hpp\""));
        REQUIRE(stringContains(source, "namespace job::serializer::generated {"));

        REQUIRE(stringContains(source, "void EmitterTest_t::pack_msgpack(msgpack::packer<msgpack::sbuffer> &pk) const {"));
        REQUIRE(stringContains(source, "pk.pack_map(6);"));
        REQUIRE(stringContains(source, "pk.pack_str_body(\"count\", 5);"));
        REQUIRE(stringContains(source, "pk.pack(count);"));
        REQUIRE(stringContains(source, "header.pack_msgpack(pk);"));

        REQUIRE(stringContains(source, "void EmitterTest_t::unpack_msgpack(const msgpack::object &obj) {"));
        REQUIRE(stringContains(source, "if (key == \"count\") {"));
        REQUIRE(stringContains(source, "val_obj.convert(count);"));
        REQUIRE(stringContains(source, "header.unpack_msgpack(val_obj);"));
    }
}
