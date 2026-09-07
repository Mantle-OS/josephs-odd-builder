#include <catch2/catch_test_macros.hpp>

#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

#include "test_job_yaml_fixtures.h"

#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Anchored scalars
// ========================================

TEST_CASE("YamlParser parses anchored root scalar", "[job_yaml][parser][node_property][anchor][scalar]")
{
    constexpr std::string_view source = "&alpha value";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "value");
}

TEST_CASE("YamlParser anchored root scalar borrows scalar source view", "[job_yaml][parser][node_property][anchor][scalar][source_view]")
{
    constexpr std::string_view source = "&alpha value";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const std::size_t valueOffset = source.find("value");

    REQUIRE(destination.complete());
    REQUIRE(valueOffset != std::string_view::npos);
    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "value");
    REQUIRE(result.scalar().data() == source.data() + valueOffset);
}

// ========================================
// Anchored mappings
// ========================================

TEST_CASE("YamlParser parses anchored root mapping",
          "[job_yaml][parser][node_property][anchor][mapping]")
{
    constexpr std::string_view source =
        "&config\n"
        "name: alpha\n"
        "count: 42\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *name = node.member("name");
    const YamlNode *count = node.member("count");

    REQUIRE(name != nullptr);
    REQUIRE(count != nullptr);
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(count->scalar() == "42");
}

TEST_CASE("YamlParser parses anchored nested mapping",
          "[job_yaml][parser][node_property][anchor][mapping][nested]")
{
    constexpr std::string_view source =
        "config: &config\n"
        "  name: alpha\n"
        "  count: 42\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *config = node.member("config");

    REQUIRE(config != nullptr);
    REQUIRE(config->isMapping());

    const YamlNode *name = config->member("name");
    const YamlNode *count = config->member("count");

    REQUIRE(name != nullptr);
    REQUIRE(count != nullptr);
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(count->scalar() == "42");
}

// ========================================
// Anchored sequences
// ========================================

TEST_CASE("YamlParser parses anchored root sequence",
          "[job_yaml][parser][node_property][anchor][sequence]")
{
    constexpr std::string_view source =
        "&items\n"
        "- alpha\n"
        "- beta\n"
        "- gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());

    const auto &sequence = node.sequence();

    REQUIRE(sequence.size() == 3);
    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[1].scalar() == "beta");
    REQUIRE(sequence[2].scalar() == "gamma");
}

TEST_CASE("YamlParser parses anchored nested sequence",
          "[job_yaml][parser][node_property][anchor][sequence][nested]")
{
    constexpr std::string_view source =
        "items: &items\n"
        "  - alpha\n"
        "  - beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *items = node.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());

    const auto &sequence = items->sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[1].scalar() == "beta");
}

// ========================================
// Scalar aliases
// ========================================

TEST_CASE("YamlParser replays scalar alias",
          "[job_yaml][parser][node_property][alias][scalar]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "copy: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *original = node.member("original");
    const YamlNode *copy = node.member("copy");

    REQUIRE(original != nullptr);
    REQUIRE(copy != nullptr);

    REQUIRE(original->isScalar());
    REQUIRE(copy->isScalar());

    REQUIRE(original->scalar() == "value");
    REQUIRE(copy->scalar() == "value");
}

TEST_CASE("YamlParser scalar alias retains original borrowed source view",
          "[job_yaml][parser][node_property][alias][scalar][source_view]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "copy: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *original = node.member("original");
    const YamlNode *copy = node.member("copy");

    REQUIRE(original != nullptr);
    REQUIRE(copy != nullptr);

    const std::size_t valueOffset = source.find("value");

    REQUIRE(valueOffset != std::string_view::npos);

    REQUIRE(original->borrowsScalar());
    REQUIRE(copy->borrowsScalar());

    REQUIRE(original->scalar().data() == source.data() + valueOffset);
    REQUIRE(copy->scalar().data() == source.data() + valueOffset);
}

// ========================================
// Mapping aliases
// ========================================

TEST_CASE("YamlParser replays mapping alias",
          "[job_yaml][parser][node_property][alias][mapping]")
{
    constexpr std::string_view source =
        "original: &config\n"
        "  name: alpha\n"
        "  count: 42\n"
        "copy: *config\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *original = node.member("original");
    const YamlNode *copy = node.member("copy");

    REQUIRE(original != nullptr);
    REQUIRE(copy != nullptr);

    REQUIRE(original->isMapping());
    REQUIRE(copy->isMapping());

    const YamlNode *originalName = original->member("name");
    const YamlNode *originalCount = original->member("count");
    const YamlNode *copyName = copy->member("name");
    const YamlNode *copyCount = copy->member("count");

    REQUIRE(originalName != nullptr);
    REQUIRE(originalCount != nullptr);
    REQUIRE(copyName != nullptr);
    REQUIRE(copyCount != nullptr);

    REQUIRE(originalName->scalar() == "alpha");
    REQUIRE(originalCount->scalar() == "42");
    REQUIRE(copyName->scalar() == "alpha");
    REQUIRE(copyCount->scalar() == "42");
}

TEST_CASE("YamlParser mapping alias scalar members retain original source views",
          "[job_yaml][parser][node_property][alias][mapping][source_view]")
{
    constexpr std::string_view source =
        "original: &config\n"
        "  name: alpha\n"
        "  count: 42\n"
        "copy: *config\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *original = node.member("original");
    const YamlNode *copy = node.member("copy");

    REQUIRE(original != nullptr);
    REQUIRE(copy != nullptr);

    const YamlNode *originalName = original->member("name");
    const YamlNode *copyName = copy->member("name");

    REQUIRE(originalName != nullptr);
    REQUIRE(copyName != nullptr);

    const std::size_t alphaOffset = source.find("alpha");

    REQUIRE(alphaOffset != std::string_view::npos);

    REQUIRE(originalName->borrowsScalar());
    REQUIRE(copyName->borrowsScalar());

    REQUIRE(originalName->scalar().data() == source.data() + alphaOffset);
    REQUIRE(copyName->scalar().data() == source.data() + alphaOffset);
}

// ========================================
// Sequence aliases
// ========================================

TEST_CASE("YamlParser replays sequence alias",
          "[job_yaml][parser][node_property][alias][sequence]")
{
    constexpr std::string_view source =
        "original: &items\n"
        "  - alpha\n"
        "  - beta\n"
        "copy: *items\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    const YamlNode *original = node.member("original");
    const YamlNode *copy = node.member("copy");

    REQUIRE(original != nullptr);
    REQUIRE(copy != nullptr);

    REQUIRE(original->isSequence());
    REQUIRE(copy->isSequence());

    REQUIRE(original->sequence().size() == 2);
    REQUIRE(copy->sequence().size() == 2);

    REQUIRE(original->sequence()[0].scalar() == "alpha");
    REQUIRE(original->sequence()[1].scalar() == "beta");

    REQUIRE(copy->sequence()[0].scalar() == "alpha");
    REQUIRE(copy->sequence()[1].scalar() == "beta");
}

TEST_CASE("YamlParser sequence alias scalars retain original source views",
          "[job_yaml][parser][node_property][alias][sequence][source_view]")
{
    constexpr std::string_view source =
        "original: &items\n"
        "  - alpha\n"
        "  - beta\n"
        "copy: *items\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isSequence());

    const auto &sequence = copy->sequence();

    REQUIRE(sequence.size() == 2);

    const std::size_t alphaOffset = source.find("alpha");
    const std::size_t betaOffset = source.find("beta");

    REQUIRE(alphaOffset != std::string_view::npos);
    REQUIRE(betaOffset != std::string_view::npos);

    REQUIRE(sequence[0].borrowsScalar());
    REQUIRE(sequence[1].borrowsScalar());

    REQUIRE(sequence[0].scalar().data() == source.data() + alphaOffset);
    REQUIRE(sequence[1].scalar().data() == source.data() + betaOffset);
}

// ========================================
// Alias replay inside sequences
// ========================================

TEST_CASE("YamlParser replays scalar alias inside sequence",
          "[job_yaml][parser][node_property][alias][sequence][scalar]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "items:\n"
        "  - *alpha\n"
        "  - *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *items = node.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());

    const auto &sequence = items->sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].scalar() == "value");
    REQUIRE(sequence[1].scalar() == "value");
}

TEST_CASE("YamlParser replays mapping alias inside sequence",
          "[job_yaml][parser][node_property][alias][sequence][mapping]")
{
    constexpr std::string_view source =
        "template: &entry\n"
        "  name: alpha\n"
        "  count: 1\n"
        "items:\n"
        "  - *entry\n"
        "  - *entry\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *items = node.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());

    const auto &sequence = items->sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].isMapping());
    REQUIRE(sequence[1].isMapping());

    const YamlNode *firstName = sequence[0].member("name");
    const YamlNode *secondName = sequence[1].member("name");

    REQUIRE(firstName != nullptr);
    REQUIRE(secondName != nullptr);

    REQUIRE(firstName->scalar() == "alpha");
    REQUIRE(secondName->scalar() == "alpha");
}

// ========================================
// Nested alias replay
// ========================================

TEST_CASE("YamlParser replays alias from previously replayed mapping content",
          "[job_yaml][parser][node_property][alias][nested]")
{
    constexpr std::string_view source =
        "value: &alpha beta\n"
        "template: &config\n"
        "  inner: *alpha\n"
        "copy: *config\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *config = node.member("copy");

    REQUIRE(config != nullptr);
    REQUIRE(config->isMapping());

    const YamlNode *inner = config->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->scalar() == "beta");
}

// ========================================
// Replay indentation
// ========================================

TEST_CASE("YamlParser normalizes indentation when replaying nested mapping",
          "[job_yaml][parser][node_property][alias][indent]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  template: &config\n"
        "    name: alpha\n"
        "    count: 42\n"
        "copy: *config\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isMapping());

    const YamlNode *name = copy->member("name");
    const YamlNode *count = copy->member("count");

    REQUIRE(name != nullptr);
    REQUIRE(count != nullptr);

    REQUIRE(name->scalar() == "alpha");
    REQUIRE(count->scalar() == "42");
}

TEST_CASE("YamlParser normalizes indentation when replaying nested sequence",
          "[job_yaml][parser][node_property][alias][indent][sequence]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  template: &items\n"
        "    - alpha\n"
        "    - beta\n"
        "copy: *items\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isSequence());

    REQUIRE(copy->sequence().size() == 2);
    REQUIRE(copy->sequence()[0].scalar() == "alpha");
    REQUIRE(copy->sequence()[1].scalar() == "beta");
}

// ========================================
// Tags
// ========================================

TEST_CASE("YamlParser accepts tag on scalar",
          "[job_yaml][parser][node_property][tag][scalar]")
{
    constexpr std::string_view source = "!example alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser accepts secondary tag on scalar",
          "[job_yaml][parser][node_property][tag][scalar]")
{
    constexpr std::string_view source = "!!str alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser accepts verbatim tag on scalar",
          "[job_yaml][parser][node_property][tag][scalar]")
{
    constexpr std::string_view source = "!<tag:example.com,2026:type> alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlParser accepts tag on nested mapping",
          "[job_yaml][parser][node_property][tag][mapping]")
{
    constexpr std::string_view source =
        "config: !example\n"
        "  name: alpha\n"
        "  count: 42\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *config = node.member("config");

    REQUIRE(config != nullptr);
    REQUIRE(config->isMapping());
}

TEST_CASE("YamlParser accepts tag on nested sequence",
          "[job_yaml][parser][node_property][tag][sequence]")
{
    constexpr std::string_view source =
        "items: !example\n"
        "  - alpha\n"
        "  - beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *items = node.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 2);
}

// ========================================
// Anchor and tag ordering
// ========================================

TEST_CASE("YamlParser accepts anchor before tag",
          "[job_yaml][parser][node_property][anchor][tag]")
{
    constexpr std::string_view source =
        "original: &alpha !example value\n"
        "copy: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->scalar() == "value");
}

TEST_CASE("YamlParser accepts tag before anchor",
          "[job_yaml][parser][node_property][anchor][tag]")
{
    constexpr std::string_view source =
        "original: !example &alpha value\n"
        "copy: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->scalar() == "value");
}

// ========================================
// Reflected destination replay
// ========================================

TEST_CASE("YamlParser replays anchored scalar into reflected destination",
          "[job_yaml][parser][object][node_property][alias]")
{
    constexpr std::string_view source =
        "name: &alpha beta\n"
        "profile:\n"
        "  displayName: *alpha\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "beta");
    REQUIRE(object.profile.displayName == "beta");
}

TEST_CASE("YamlParser replays anchored mapping into reflected destination",
          "[job_yaml][parser][object][node_property][alias][mapping]")
{
    constexpr std::string_view source =
        "address: &address\n"
        "  city: Delta\n"
        "  zip: 12345\n"
        "profile:\n"
        "  address: *address\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());

    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);

    REQUIRE(object.profile.address.city == "Delta");
    REQUIRE(object.profile.address.zip == 12345);
}

// ========================================
// Alias reuse
// ========================================

TEST_CASE("YamlParser may replay same alias multiple times",
          "[job_yaml][parser][node_property][alias][reuse]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "first: *alpha\n"
        "second: *alpha\n"
        "third: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");
    const YamlNode *third = node.member("third");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);

    REQUIRE(first->scalar() == "value");
    REQUIRE(second->scalar() == "value");
    REQUIRE(third->scalar() == "value");
}

// ========================================
// Invalid anchors
// ========================================

TEST_CASE("YamlParser rejects bare anchor",
          "[job_yaml][parser][node_property][anchor][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("&", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects duplicate anchor property on node",
          "[job_yaml][parser][node_property][anchor][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("&alpha &beta value", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects duplicate anchor name in document",
          "[job_yaml][parser][node_property][anchor][invalid]")
{
    constexpr std::string_view source =
        "first: &alpha one\n"
        "second: &alpha two\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse(source, destination));
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Invalid aliases
// ========================================

TEST_CASE("YamlParser rejects bare alias",
          "[job_yaml][parser][node_property][alias][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("*", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unknown alias",
          "[job_yaml][parser][node_property][alias][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("*missing", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects forward alias",
          "[job_yaml][parser][node_property][alias][invalid]")
{
    constexpr std::string_view source =
        "copy: *alpha\n"
        "original: &alpha value\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse(source, destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects alias with anchor property",
          "[job_yaml][parser][node_property][alias][invalid]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "copy: &beta *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse(source, destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects alias with tag property",
          "[job_yaml][parser][node_property][alias][invalid]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "copy: !example *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse(source, destination));
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Invalid tags
// ========================================

TEST_CASE("YamlParser rejects bare tag",
          "[job_yaml][parser][node_property][tag][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("!", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects duplicate tag property",
          "[job_yaml][parser][node_property][tag][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse("!first !second value", destination));
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unterminated verbatim tag",
          "[job_yaml][parser][node_property][tag][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(YamlParser::parse(
        "!<tag:example.com,2026:type value",
        destination));

    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Anchor range replay
// ========================================

TEST_CASE("YamlParser alias replay does not redeclare original anchor",
          "[job_yaml][parser][node_property][anchor][alias][range]")
{
    constexpr std::string_view source =
        "original: &alpha value\n"
        "first: *alpha\n"
        "second: *alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *first = node.member("first");
    const YamlNode *second = node.member("second");

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first->scalar() == "value");
    REQUIRE(second->scalar() == "value");
}

TEST_CASE("YamlParser anchor range excludes trailing sibling",
          "[job_yaml][parser][node_property][anchor][alias][range]")
{
    constexpr std::string_view source =
        "original: &config\n"
        "  name: alpha\n"
        "  count: 42\n"
        "sibling: beta\n"
        "copy: *config\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode *copy = node.member("copy");

    REQUIRE(copy != nullptr);
    REQUIRE(copy->isMapping());

    REQUIRE(copy->member("name") != nullptr);
    REQUIRE(copy->member("count") != nullptr);
    REQUIRE(copy->member("sibling") == nullptr);

    const YamlNode *sibling = node.member("sibling");

    REQUIRE(sibling != nullptr);
    REQUIRE(sibling->scalar() == "beta");
}

} // namespace job::yaml::tests