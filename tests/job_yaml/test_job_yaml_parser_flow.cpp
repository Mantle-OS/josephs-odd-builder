#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

#include "job_yaml_node.h"
#include "job_yaml_object_parser_destination.h"
#include "job_yaml_parser.h"
#include "job_yaml_parser_destination.h"

#include "test_job_yaml_flow_fixtures_generated.h"

namespace job::yaml::tests {

struct FlowSequenceFixture
{
    std::vector<std::string> items;
};

struct FlowIntegerSequenceFixture
{
    std::vector<int> values;
};

struct FlowNestedSequenceFixture
{
    std::vector<std::vector<int>> values;
};

struct FlowItemFixture
{
    std::string name;
    int count{};
};

struct FlowObjectSequenceFixture
{
    std::vector<FlowItemFixture> items;
};

// ========================================
// Root flow sequences
// ========================================

TEST_CASE("YamlParser parses empty root flow sequence",
          "[job_yaml][parser][flow][sequence]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("[]", destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlParser parses root flow sequence",
          "[job_yaml][parser][flow][sequence]")
{
    constexpr std::string_view source = "[alpha, beta, gamma]";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().size() == 3);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].scalar() == "beta");
    REQUIRE(result.sequence()[2].scalar() == "gamma");
}

TEST_CASE("YamlParser accepts trailing comma in root flow sequence",
          "[job_yaml][parser][flow][sequence][trailing_comma]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("[alpha, beta,]", destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.sequence().size() == 2);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].scalar() == "beta");
}

// ========================================
// Root flow mappings
// ========================================

TEST_CASE("YamlParser parses empty root flow mapping",
          "[job_yaml][parser][flow][mapping]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("{}", destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isMapping());
    REQUIRE(result.mapping().empty());
}

TEST_CASE("YamlParser parses root flow mapping",
          "[job_yaml][parser][flow][mapping]")
{
    constexpr std::string_view source = "{name: alpha, count: 42}";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *name = result.member("name");
    const YamlNode *count = result.member("count");

    REQUIRE(destination.complete());
    REQUIRE(result.isMapping());
    REQUIRE(name != nullptr);
    REQUIRE(count != nullptr);
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(count->scalar() == "42");
}

TEST_CASE("YamlParser parses quoted keys and values in flow mapping",
          "[job_yaml][parser][flow][mapping][quoted]")
{
    constexpr std::string_view source =
        "{'display name': 'alpha', \"message\": \"hello\\nworld\"}";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *name = result.member("display name");
    const YamlNode *message = result.member("message");

    REQUIRE(destination.complete());
    REQUIRE(name != nullptr);
    REQUIRE(message != nullptr);
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(message->scalar() == "hello\nworld");
    REQUIRE(message->ownsScalar());
}

TEST_CASE("YamlParser accepts trailing comma in root flow mapping",
          "[job_yaml][parser][flow][mapping][trailing_comma]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("{name: alpha, count: 42,}", destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.member("name") != nullptr);
    REQUIRE(result.member("count") != nullptr);
}

// ========================================
// Nested flow structures
// ========================================

TEST_CASE("YamlParser parses nested flow sequence",
          "[job_yaml][parser][flow][sequence][nested]")
{
    constexpr std::string_view source = "[alpha, [beta, gamma]]";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(result.sequence().size() == 2);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].isSequence());
    REQUIRE(result.sequence()[1].sequence().size() == 2);
    REQUIRE(result.sequence()[1].sequence()[0].scalar() == "beta");
    REQUIRE(result.sequence()[1].sequence()[1].scalar() == "gamma");
}

TEST_CASE("YamlParser parses nested flow mapping",
          "[job_yaml][parser][flow][mapping][nested]")
{
    constexpr std::string_view source = "{outer: {inner: alpha}}";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *outer = result.member("outer");

    REQUIRE(outer != nullptr);
    REQUIRE(outer->isMapping());

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->scalar() == "alpha");
}

TEST_CASE("YamlParser parses sequence of flow mappings",
          "[job_yaml][parser][flow][sequence][mapping][nested]")
{
    constexpr std::string_view source =
        "[{name: alpha}, {name: beta}]";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(result.sequence().size() == 2);
    REQUIRE(result.sequence()[0].isMapping());
    REQUIRE(result.sequence()[1].isMapping());

    const YamlNode *first = result.sequence()[0].member("name");
    const YamlNode *second = result.sequence()[1].member("name");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->scalar() == "alpha");
    REQUIRE(second->scalar() == "beta");
}

TEST_CASE("YamlParser parses mapping containing nested flow sequence",
          "[job_yaml][parser][flow][mapping][sequence][nested]")
{
    constexpr std::string_view source =
        "{items: [alpha, beta], enabled: true}";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *items = result.member("items");
    const YamlNode *enabled = result.member("enabled");

    REQUIRE(items != nullptr);
    REQUIRE(enabled != nullptr);
    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 2);
    REQUIRE(items->sequence()[0].scalar() == "alpha");
    REQUIRE(items->sequence()[1].scalar() == "beta");
    REQUIRE(enabled->scalar() == "true");
}

// ========================================
// Block / flow combinations
// ========================================

TEST_CASE("YamlParser parses block mapping containing flow sequence",
          "[job_yaml][parser][flow][block][mapping][sequence]")
{
    constexpr std::string_view source =
        "items: [alpha, beta, gamma]\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *items = result.member("items");

    REQUIRE(destination.complete());
    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 3);
}

TEST_CASE("YamlParser parses block mapping containing flow mapping",
          "[job_yaml][parser][flow][block][mapping]")
{
    constexpr std::string_view source =
        "config: {name: alpha, count: 42}\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *config = result.member("config");

    REQUIRE(config != nullptr);
    REQUIRE(config->isMapping());
    REQUIRE(config->member("name") != nullptr);
    REQUIRE(config->member("count") != nullptr);
}

TEST_CASE("YamlParser parses block sequence containing flow collections",
          "[job_yaml][parser][flow][block][sequence]")
{
    constexpr std::string_view source =
        "- [alpha, beta]\n"
        "- {name: gamma}\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().size() == 2);
    REQUIRE(result.sequence()[0].isSequence());
    REQUIRE(result.sequence()[1].isMapping());
}

// ========================================
// Flow whitespace and comments
// ========================================

TEST_CASE("YamlParser parses multiline flow collection",
          "[job_yaml][parser][flow][multiline]")
{
    constexpr std::string_view source =
        "[\n"
        "  alpha,\n"
        "  beta,\n"
        "  gamma\n"
        "]\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(result.sequence().size() == 3);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].scalar() == "beta");
    REQUIRE(result.sequence()[2].scalar() == "gamma");
}

TEST_CASE("YamlParser parses comments inside flow collection",
          "[job_yaml][parser][flow][comment]")
{
    constexpr std::string_view source =
        "[alpha, # first\n"
        " beta, # second\n"
        " gamma]\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(result.sequence().size() == 3);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].scalar() == "beta");
    REQUIRE(result.sequence()[2].scalar() == "gamma");
}

// ========================================
// Anchors and aliases
// ========================================

TEST_CASE("YamlParser replays anchored flow sequence",
          "[job_yaml][parser][flow][anchor][alias][sequence]")
{
    constexpr std::string_view source =
        "template: &items [alpha, beta]\n"
        "copy: *items\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *copy = result.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isSequence());
    REQUIRE(copy->sequence().size() == 2);
    REQUIRE(copy->sequence()[0].scalar() == "alpha");
    REQUIRE(copy->sequence()[1].scalar() == "beta");
}

TEST_CASE("YamlParser replays anchored flow mapping",
          "[job_yaml][parser][flow][anchor][alias][mapping]")
{
    constexpr std::string_view source =
        "template: &entry {name: alpha, count: 42}\n"
        "copy: *entry\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *copy = result.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isMapping());
    REQUIRE(copy->member("name") != nullptr);
    REQUIRE(copy->member("count") != nullptr);
    REQUIRE(copy->member("name")->scalar() == "alpha");
    REQUIRE(copy->member("count")->scalar() == "42");
}

TEST_CASE("YamlParser replays alias inside flow sequence",
          "[job_yaml][parser][flow][anchor][alias][nested]")
{
    constexpr std::string_view source =
        "template: &entry {name: alpha}\n"
        "items: [*entry, {name: beta}]\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *items = result.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 2);
    REQUIRE(items->sequence()[0].isMapping());
    REQUIRE(items->sequence()[1].isMapping());
    REQUIRE(items->sequence()[0].member("name")->scalar() == "alpha");
    REQUIRE(items->sequence()[1].member("name")->scalar() == "beta");
}

// ========================================
// Reflected sequence destinations
// ========================================

TEST_CASE("YamlParser assigns flow sequence to reflected string sequence member",
          "[job_yaml][parser][flow][object][sequence]")
{
    FlowSequenceFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("items: [alpha, beta, gamma]\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.items.size() == 3);
    REQUIRE(object.items[0] == "alpha");
    REQUIRE(object.items[1] == "beta");
    REQUIRE(object.items[2] == "gamma");
}

TEST_CASE("YamlParser converts flow sequence scalars into reflected integer sequence",
          "[job_yaml][parser][flow][object][sequence][conversion]")
{
    FlowIntegerSequenceFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("values: [1, 2, 3]\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.values == std::vector<int>{1, 2, 3});
}

TEST_CASE("YamlParser assigns nested reflected sequence",
          "[job_yaml][parser][flow][object][sequence][nested]")
{
    FlowNestedSequenceFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("values: [[1, 2], [3, 4]]\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.values.size() == 2);
    REQUIRE(object.values[0] == std::vector<int>{1, 2});
    REQUIRE(object.values[1] == std::vector<int>{3, 4});
}

TEST_CASE("YamlParser assigns flow sequence of reflected objects",
          "[job_yaml][parser][flow][object][sequence][mapping]")
{
    FlowObjectSequenceFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(
        "items: [{name: alpha, count: 1}, {name: beta, count: 2}]\n",
        destination));

    REQUIRE(destination.complete());
    REQUIRE(object.items.size() == 2);
    REQUIRE(object.items[0].name == "alpha");
    REQUIRE(object.items[0].count == 1);
    REQUIRE(object.items[1].name == "beta");
    REQUIRE(object.items[1].count == 2);
}

TEST_CASE("YamlParser parses root reflected integer sequence",
          "[job_yaml][parser][flow][object][sequence][root]")
{
    std::vector<int> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(YamlParser::parse("[1, 2, 3]", destination));

    REQUIRE(destination.complete());
    REQUIRE(values == std::vector<int>{1, 2, 3});
}

// ========================================
// Invalid flow grammar
// ========================================

TEST_CASE("YamlParser rejects unterminated flow sequence",
          "[job_yaml][parser][flow][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("[alpha, beta", destination));
}

TEST_CASE("YamlParser rejects unterminated flow mapping",
          "[job_yaml][parser][flow][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("{name: alpha", destination));
}

TEST_CASE("YamlParser rejects mismatched flow delimiters",
          "[job_yaml][parser][flow][invalid]")
{
    SECTION("sequence closed by mapping delimiter")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse("[alpha, beta}", destination));
    }

    SECTION("mapping closed by sequence delimiter")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse("{name: alpha]", destination));
    }
}

TEST_CASE("YamlParser rejects missing flow mapping value",
          "[job_yaml][parser][flow][mapping][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("{name:}", destination));
}

TEST_CASE("YamlParser rejects missing flow mapping separator",
          "[job_yaml][parser][flow][mapping][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("{name alpha}", destination));
}

TEST_CASE("YamlParser rejects duplicate flow collection separators",
          "[job_yaml][parser][flow][invalid]")
{
    SECTION("sequence")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse("[alpha,, beta]", destination));
    }

    SECTION("mapping")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        REQUIRE_FALSE(YamlParser::parse("{name: alpha,, count: 42}", destination));
    }
}


TEST_CASE("YamlParser parses generated flow fixtures", "[job_yaml][parser][flow][generated]")
{
    for (const YamlFlowFixture &fixture : FlowFixtures) {
        // WARN("fixture: " << fixture.name);
        // WARN("production: " << fixture.production);
        // WARN("source: " << fixture.source);

        YamlNode node;
        YamlNodeParserDestination destination{node};

        const bool parsed = YamlParser::parse(fixture.source, destination);

        REQUIRE(parsed == fixture.valid);

        if (!fixture.valid)
            continue;

        REQUIRE(destination.complete());

        if (fixture.kind == YamlFlowFixtureKind::Sequence)
            REQUIRE(node.isSequence());
        else
            REQUIRE(node.isMapping());
    }
}




} // namespace job::yaml::tests
