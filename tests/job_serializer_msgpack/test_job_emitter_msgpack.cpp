#include <catch2/catch_all.hpp>

#include <string>

#include <schema.h>
#include <runtime_object.h>

#include <job_emitter_msgpack.h>
#include <job_serializer_msgpack.h>

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

    SECTION("Bin field generates pack_bin, not the generic pk.pack() path")
    {
        REQUIRE(stringContains(source, "pk.pack_bin(static_cast<uint32_t>(data.size()));"));
        REQUIRE(stringContains(source, "pk.pack_bin_body(reinterpret_cast<const char*>(data.data()), static_cast<uint32_t>(data.size()));"));
        REQUIRE_FALSE(stringContains(source, "pk.pack(data);"));
    }

    SECTION("Bin field unpack enforces exact size, throws on mismatch")
    {
        REQUIRE(stringContains(source, "if (val_obj.via.bin.size != 32)"));
        REQUIRE(stringContains(source, "throw std::runtime_error(\"size mismatch unpacking bin field 'data', expected 32 bytes"));
        REQUIRE(stringContains(source, "std::memcpy(data.data(), val_obj.via.bin.ptr, 32);"));
        REQUIRE_FALSE(stringContains(source, "std::min<size_t>(data.size()"));
    }

    SECTION("ListBin field packs each element as bin, not as a nested array of ints")
    {
        REQUIRE(stringContains(source, "pk.pack_array(static_cast<uint32_t>(packets.size()));"));
        REQUIRE(stringContains(source, "pk.pack_bin(static_cast<uint32_t>(item.size()));"));
        REQUIRE(stringContains(source, "pk.pack_bin_body(reinterpret_cast<const char*>(item.data()), static_cast<uint32_t>(item.size()));"));
    }

    SECTION("ListBin unpack enforces exact per-element size, throws on mismatch")
    {
        REQUIRE(stringContains(source, "if (elem.via.bin.size != 128)"));
        REQUIRE(stringContains(source, "throw std::runtime_error(\"size mismatch unpacking list<bin> element in 'packets', expected 128 bytes"));
        REQUIRE(stringContains(source, "std::array<uint8_t, 128> item{};"));
        REQUIRE(stringContains(source, "std::memcpy(item.data(), elem.via.bin.ptr, 128);"));
    }
}

TEST_CASE("JobSerializerMsgPack (Runtime) round-trips bool/float/double scalars", "[job_serializer_msgpack][bugfix]")
{
    JobMsgPackSerializer ser{};
    Schema s = TestEmitter::getEmitterTestSchema();

    s.fields.push_back({
        .key = 20, .name = "flag", .type = "bool", .kind = FieldKind::Scalar,
        .size{}, .ctype{}, .ref_include{}, .ref_sym{}, .required = false, .comment{}
    });
    s.fields.push_back({
        .key = 21, .name = "ratio", .type = "float", .kind = FieldKind::Scalar,
        .size{}, .ctype{}, .ref_include{}, .ref_sym{}, .required = false, .comment{}
    });
    s.fields.push_back({
        .key = 22, .name = "precise", .type = "double", .kind = FieldKind::Scalar,
        .size{}, .ctype{}, .ref_include{}, .ref_sym{}, .required = false, .comment{}
    });
    REQUIRE(s.isValid());

    RuntimeObject obj_in{};
    obj_in.setField("flag", FieldValue{ .value = FieldValue::Scalar{ true } });
    obj_in.setField("ratio", FieldValue{ .value = FieldValue::Scalar{ 3.5f } });
    obj_in.setField("precise", FieldValue{ .value = FieldValue::Scalar{ 2.71828 } });

    std::vector<uint8_t> buffer;
    REQUIRE(ser.encode(s, obj_in, buffer, SerializeFormat::Binary));

    RuntimeObject obj_out{};
    REQUIRE(ser.decode(s, obj_out, buffer, SerializeFormat::Binary));

    auto flag_val = obj_out.getField("flag");
    REQUIRE(flag_val.has_value());
    REQUIRE(flag_val->isScalar());
    REQUIRE(std::get<FieldValue::Scalar>(flag_val->value) == FieldValue::Scalar{ true });

    auto ratio_val = obj_out.getField("ratio");
    REQUIRE(ratio_val.has_value());
    REQUIRE(std::get<float>(std::get<FieldValue::Scalar>(ratio_val->value)) == Catch::Approx(3.5f));

    auto precise_val = obj_out.getField("precise");
    REQUIRE(precise_val.has_value());
    REQUIRE(std::get<double>(std::get<FieldValue::Scalar>(precise_val->value)) == Catch::Approx(2.71828));
}