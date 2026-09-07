#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

namespace job::yaml::tests {

struct ParserNullObject
{
    int scalarValue{7};
    std::string stringValue{"unchanged"};
    std::optional<int> optionalValue{42};
    std::shared_ptr<int> sharedValue{std::make_shared<int>(43)};
    std::unique_ptr<int> uniqueValue{std::make_unique<int>(44)};
};

// ========================================
// Root null
// ========================================

TEST_CASE("YamlParser parses YAML null spellings as root null nodes",
          "[job_yaml][parser][null][root]")
{
    SECTION("tilde")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("~", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isNull());
    }

    SECTION("lowercase")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("null", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isNull());
    }

    SECTION("title case")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("Null", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isNull());
    }

    SECTION("uppercase")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("NULL", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isNull());
    }
}

TEST_CASE("YamlParser keeps quoted null spellings as scalar strings",
          "[job_yaml][parser][null][quoted]")
{
    SECTION("double quoted null")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("\"null\"", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isScalar());
        REQUIRE(node.scalar() == "null");
    }

    SECTION("single quoted null")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("'null'", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isScalar());
        REQUIRE(node.scalar() == "null");
    }

    SECTION("double quoted tilde")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("\"~\"", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isScalar());
        REQUIRE(node.scalar() == "~");
    }

    SECTION("single quoted tilde")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE(YamlParser::parse("'~'", destination));
        REQUIRE(destination.complete());
        REQUIRE(node.isScalar());
        REQUIRE(node.scalar() == "~");
    }
}

// ========================================
// Block mapping null
// ========================================

TEST_CASE("YamlParser parses block mapping null values",
          "[job_yaml][parser][null][mapping]")
{
    constexpr std::string_view source =
        "first: null\n"
        "second: ~\n"
        "third: NULL\n"
        "text: \"null\"\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");
    const YamlNode *third = node.member("third");
    const YamlNode *text = node.member("text");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);
    REQUIRE(text != nullptr);

    REQUIRE(first->isNull());
    REQUIRE(second->isNull());
    REQUIRE(third->isNull());

    REQUIRE(text->isScalar());
    REQUIRE(text->scalar() == "null");
}

TEST_CASE("YamlParser keeps plain null spelling as textual mapping key",
          "[job_yaml][parser][null][mapping][key]")
{
    constexpr std::string_view source =
        "null: value\n"
        "~: other\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *nullKey = node.member("null");
    const YamlNode *tildeKey = node.member("~");

    REQUIRE(nullKey != nullptr);
    REQUIRE(tildeKey != nullptr);

    REQUIRE(nullKey->isScalar());
    REQUIRE(nullKey->scalar() == "value");

    REQUIRE(tildeKey->isScalar());
    REQUIRE(tildeKey->scalar() == "other");
}

// ========================================
// Block sequence null
// ========================================

TEST_CASE("YamlParser parses block sequence null entries",
          "[job_yaml][parser][null][sequence]")
{
    constexpr std::string_view source =
        "- alpha\n"
        "- null\n"
        "- ~\n"
        "- \"null\"\n"
        "- omega\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());

    const auto &sequence = node.sequence();

    REQUIRE(sequence.size() == 5);

    REQUIRE(sequence[0].isScalar());
    REQUIRE(sequence[0].scalar() == "alpha");

    REQUIRE(sequence[1].isNull());
    REQUIRE(sequence[2].isNull());

    REQUIRE(sequence[3].isScalar());
    REQUIRE(sequence[3].scalar() == "null");

    REQUIRE(sequence[4].isScalar());
    REQUIRE(sequence[4].scalar() == "omega");
}

TEST_CASE("YamlParser parses nested block null values",
          "[job_yaml][parser][null][nested]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  value: null\n"
        "  items:\n"
        "    - first\n"
        "    - ~\n"
        "    - last\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);
    REQUIRE(outer->isMapping());

    const YamlNode *value = outer->member("value");
    const YamlNode *items = outer->member("items");

    REQUIRE(value != nullptr);
    REQUIRE(items != nullptr);

    REQUIRE(value->isNull());

    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 3);
    REQUIRE(items->sequence()[0].scalar() == "first");
    REQUIRE(items->sequence()[1].isNull());
    REQUIRE(items->sequence()[2].scalar() == "last");
}

// ========================================
// Flow collection null
// ========================================

TEST_CASE("YamlParser parses flow sequence null entries",
          "[job_yaml][parser][null][flow][sequence]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("[alpha, null, ~, \"null\", omega]", destination));
    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());

    const auto &sequence = node.sequence();

    REQUIRE(sequence.size() == 5);
    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[1].isNull());
    REQUIRE(sequence[2].isNull());
    REQUIRE(sequence[3].isScalar());
    REQUIRE(sequence[3].scalar() == "null");
    REQUIRE(sequence[4].scalar() == "omega");
}

TEST_CASE("YamlParser parses flow mapping null values",
          "[job_yaml][parser][null][flow][mapping]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("{first: null, second: ~, text: \"null\"}", destination));
    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");
    const YamlNode *text = node.member("text");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(text != nullptr);

    REQUIRE(first->isNull());
    REQUIRE(second->isNull());
    REQUIRE(text->isScalar());
    REQUIRE(text->scalar() == "null");
}

// ========================================
// Reflected nullable members
// ========================================

TEST_CASE("YamlParser resets reflected nullable members from null",
          "[job_yaml][parser][null][object]")
{
    constexpr std::string_view source =
        "optionalValue: null\n"
        "sharedValue: ~\n"
        "uniqueValue: NULL\n";

    ParserNullObject object;
    YamlObjectParserDestination destination{object};

    REQUIRE(object.optionalValue.has_value());
    REQUIRE(object.sharedValue != nullptr);
    REQUIRE(object.uniqueValue != nullptr);

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());

    REQUIRE_FALSE(object.optionalValue.has_value());
    REQUIRE(object.sharedValue == nullptr);
    REQUIRE(object.uniqueValue == nullptr);

    REQUIRE(object.scalarValue == 7);
    REQUIRE(object.stringValue == "unchanged");
}

TEST_CASE("YamlParser rejects semantic null for ordinary reflected scalar member",
          "[job_yaml][parser][null][object][invalid]")
{
    ParserNullObject object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("scalarValue: null\n", destination));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(object.scalarValue == 7);
}

TEST_CASE("YamlParser rejects semantic null for ordinary reflected string member",
          "[job_yaml][parser][null][object][invalid]")
{
    ParserNullObject object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("stringValue: null\n", destination));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(object.stringValue == "unchanged");
}

TEST_CASE("YamlParser keeps quoted null as reflected string value",
          "[job_yaml][parser][null][object][quoted]")
{
    ParserNullObject object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("stringValue: \"null\"\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.stringValue == "null");
}

// ========================================
// Nullable sequence destinations
// ========================================

TEST_CASE("YamlParser parses null into optional sequence destination",
          "[job_yaml][parser][null][object][sequence][optional]")
{
    std::vector<std::optional<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(YamlParser::parse("- null\n- ~\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(values.size() == 2);
    REQUIRE_FALSE(values[0].has_value());
    REQUIRE_FALSE(values[1].has_value());
}

TEST_CASE("YamlParser parses null into shared pointer sequence destination",
          "[job_yaml][parser][null][object][sequence][shared_pointer]")
{
    std::vector<std::shared_ptr<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(YamlParser::parse("- null\n- ~\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == nullptr);
    REQUIRE(values[1] == nullptr);
}

TEST_CASE("YamlParser parses null into unique pointer sequence destination",
          "[job_yaml][parser][null][object][sequence][unique_pointer]")
{
    std::vector<std::unique_ptr<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(YamlParser::parse("- null\n- ~\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == nullptr);
    REQUIRE(values[1] == nullptr);
}

TEST_CASE("YamlParser rejects null in ordinary scalar sequence destination",
          "[job_yaml][parser][null][object][sequence][invalid]")
{
    std::vector<int> values;
    YamlObjectParserDestination destination{values};

    REQUIRE_FALSE(YamlParser::parse("- 1\n- null\n", destination));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 1);
}

// ========================================
// Anchors and aliases
// ========================================

TEST_CASE("YamlParser replays anchored null semantics through alias",
          "[job_yaml][parser][null][anchor][alias]")
{
    constexpr std::string_view source =
        "first: &nothing null\n"
        "second: *nothing\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));
    REQUIRE(destination.complete());

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(first->isNull());
    REQUIRE(second->isNull());
}

} // namespace job::yaml::tests
