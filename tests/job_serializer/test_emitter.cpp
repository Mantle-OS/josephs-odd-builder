#include <catch2/catch_all.hpp>

#include <string>


#include <schema.h>

#include "test_emitter.h"
#include <emitters/cpp_emitter.h>

using namespace job::serializer;

TEST_CASE("Emitter::render generates valid header and source content", "[emitter]")
{
    TestEmitter emitter{};
    Schema s = TestEmitter::getEmitterTestSchema();
    REQUIRE(s.isValid());

    auto [header, source] = emitter.render(s);

    SECTION("Emission validation")
    {
        REQUIRE_FALSE(header.empty());
        REQUIRE_FALSE(source.empty());
    }

    SECTION("Header content verification")
    {
        REQUIRE(stringContains(header, "#pragma once"));
        REQUIRE(stringContains(header, "namespace job::serializer::generated"));
        REQUIRE(stringContains(header, "struct EmitterTest_t {"));

        REQUIRE(stringContains(header, "#include \"my_header.hpp\""));
        REQUIRE(stringContains(header, "#include \"my_item.hpp\""));

        REQUIRE(stringContains(header, "uint32_t count;"));
        REQUIRE(stringContains(header, "std::array<uint8_t, 32> data;"));
        REQUIRE(stringContains(header, "std::vector<std::string> names;"));
        REQUIRE(stringContains(header, "std::vector<std::array<uint8_t, 128>> packets;"));
        REQUIRE(stringContains(header, "MyHeader header;"));
        REQUIRE(stringContains(header, "std::vector<MyTestItem> items;"));

        REQUIRE(stringContains(header, "void print_emitter_test_msg(const EmitterTest_t &obj);"));
        REQUIRE(stringContains(header, "--- Appended Header (TestEmitter) ---"));

        REQUIRE(stringContains(header, "\n\n};\n\n"));
    }

    SECTION("Source content verification")
    {
        REQUIRE(stringContains(source, "#include \"emitter_test_msg.hpp\""));
        REQUIRE(stringContains(source, "namespace job::serializer::generated"));
        REQUIRE(stringContains(source, "void print_emitter_test_msg(const EmitterTest_t &obj)"));
        REQUIRE(stringContains(source, "std::cout << \"  count: \""));
        REQUIRE(stringContains(source, "std::cout << \"  data: \""));
        REQUIRE(stringContains(source, "std::cout << \"  items: \""));
        REQUIRE(stringContains(source, "--- Appended Source (TestEmitter) ---"));
    }

    SECTION("Invalid schema returns empty strings")
    {
        Schema s_invalid{};
        REQUIRE_FALSE(s_invalid.isValid());

        auto [h, src] = emitter.render(s_invalid);
        REQUIRE(h.empty());
        REQUIRE(src.empty());
    }
}

TEST_CASE("Emitter correctly generates types for variable-size 'bin' (bug fix)", "[emitter][bugfix]")
{
    TestEmitter emitter{};
    Schema s = TestEmitter::getEmitterTestSchema();
    s.fields.push_back({
        .key = 10,
        .name = "var_bin_data",
        .type = "bin",
        .kind = FieldKind::Bin,
        .size{}, .ctype{},
        .ref_include{},
        .ref_sym{},
        .required = false,
        .comment{}
    });

    s.fields.push_back({
        .key = 11,
        .name = "var_bin_list",
        .type = "list<bin>",
        .kind = FieldKind::ListBin,
        .size{},
        .ctype{},
        .ref_include{},
        .ref_sym{},
        .required = false,
        .comment{}
    });
    REQUIRE(s.isValid());
    auto [header, source] = emitter.render(s);
    SECTION("Header generates std::vector<uint8_t> for 'bin'")
    {
        REQUIRE(stringContains(header, "std::vector<uint8_t> var_bin_data;"));
        REQUIRE_FALSE(stringContains(header, "std::array<uint8_t, 0> var_bin_data;"));
    }

    SECTION("Header generates std::vector<std::vector<uint8_t>> for 'list<bin>'")
    {
        REQUIRE(stringContains(header, "std::vector<std::vector<uint8_t>> var_bin_list;"));
        REQUIRE_FALSE(stringContains(header, "std::vector<std::array<uint8_t, 0> > var_bin_list;"));
    }
}
// CHECKPOINT: v1
