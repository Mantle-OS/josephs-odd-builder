#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <string_view>

#include <job_yaml_concepts.h>
#include <job_yaml_indent_stack.h>
#include <job_yaml_node.h>
#include <job_yaml_parser_destination.h>

#include "test_job_yaml_fixtures.h"

namespace job::yaml::tests {

static_assert(YamlParseDestination<YamlNodeParserDestination>);

// ========================================
// Initial state
// ========================================

TEST_CASE("YamlNodeParserDestination starts incomplete at root depth",
          "[job_yaml][parser_destination][node]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 0);
}

// ========================================
// Root null
// ========================================

TEST_CASE("YamlNodeParserDestination routes root null directly to destination",
          "[job_yaml][parser_destination][node][null]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.null());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(node.isNull());
}

TEST_CASE("YamlNodeParserDestination rejects events after root null completion",
          "[job_yaml][parser_destination][node][null][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.null());
    REQUIRE(destination.complete());

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("again"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"again"}));
    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"name"}));
    REQUIRE_FALSE(destination.beginMapping());
    REQUIRE_FALSE(destination.endMapping());
    REQUIRE_FALSE(destination.beginSequence());
    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(node.isNull());
}

// ========================================
// Root scalar
// ========================================

TEST_CASE("YamlNodeParserDestination routes root scalar directly to destination",
          "[job_yaml][parser_destination][node][scalar]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(source));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination routes owned root scalar directly to destination",
          "[job_yaml][parser_destination][node][scalar][owned]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalarOwned(std::string{"alpha"}));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
}

TEST_CASE("YamlNodeParserDestination preserves root scalar string_view length",
          "[job_yaml][parser_destination][node][scalar][source_view]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(value));

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source);
    REQUIRE(result.scalar().size() == 5);
}

TEST_CASE("YamlNodeParserDestination preserves root scalar source offset",
          "[job_yaml][parser_destination][node][scalar][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(value));

    const YamlNode &result = node;

    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data() + 6);
    REQUIRE(result.scalar().data() == value.data());
}

TEST_CASE("YamlNodeParserDestination accepts empty borrowed root scalar",
          "[job_yaml][parser_destination][node][scalar][source_view]")
{
    const std::string_view value{};

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(value));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNodeParserDestination preserves embedded null in root scalar",
          "[job_yaml][parser_destination][node][scalar][source_view]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    const std::string_view value{source, sizeof(source)};

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(value));

    const YamlNode &result = node;
    const std::string_view scalar = result.scalar();

    REQUIRE(result.borrowsScalar());
    REQUIRE(scalar.data() == source);
    REQUIRE(scalar.size() == sizeof(source));
    REQUIRE(scalar[0] == 'A');
    REQUIRE(scalar[1] == '\0');
    REQUIRE(scalar[2] == 'B');
}

TEST_CASE("YamlNodeParserDestination rejects events after root scalar completion",
          "[job_yaml][parser_destination][node][scalar][invalid]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(source));
    REQUIRE(destination.complete());

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("again"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"again"}));
    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"name"}));
    REQUIRE_FALSE(destination.beginMapping());
    REQUIRE_FALSE(destination.endMapping());
    REQUIRE_FALSE(destination.beginSequence());
    REQUIRE_FALSE(destination.endSequence());

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().data() == source.data());
}

// ========================================
// Root mapping
// ========================================

TEST_CASE("YamlNodeParserDestination builds root mapping with scalar member",
          "[job_yaml][parser_destination][node][mapping]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(source));

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(node.mapping()[0].key == "name");
    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination builds root mapping with null member",
          "[job_yaml][parser_destination][node][mapping][null]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("value"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "value");
    REQUIRE(node.mapping()[0].value.isNull());
}

TEST_CASE("YamlNodeParserDestination builds root mapping with owned key and null member",
          "[job_yaml][parser_destination][node][mapping][null][key][owned]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"value"}));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "value");
    REQUIRE(node.mapping()[0].value.isNull());
}

TEST_CASE("YamlNodeParserDestination builds root mapping with owned scalar member",
          "[job_yaml][parser_destination][node][mapping][owned]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalarOwned(std::string{"alpha"}));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(node.mapping()[0].key == "name");
    REQUIRE(value.isScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.ownsScalar());
    REQUIRE_FALSE(value.borrowsScalar());
}

TEST_CASE("YamlNodeParserDestination builds root mapping with owned key",
          "[job_yaml][parser_destination][node][mapping][key][owned]")
{
    constexpr std::string_view value = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"name"}));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == value.data());
}

TEST_CASE("YamlNodeParserDestination owned key survives until nested container is committed",
          "[job_yaml][parser_destination][node][mapping][key][owned][nested]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"entry"}));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalarOwned(std::string{"alpha"}));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());

    const YamlNode *entry = node.member("entry");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->isMapping());

    const YamlNode *name = entry->member("name");

    REQUIRE(name != nullptr);
    REQUIRE(name->ownsScalar());
    REQUIRE(name->scalar() == "alpha");
}

TEST_CASE("YamlNodeParserDestination mapping scalar preserves exact source slice",
          "[job_yaml][parser_destination][node][mapping][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

TEST_CASE("YamlNodeParserDestination mapping stores key independently from source",
          "[job_yaml][parser_destination][node][mapping][source_view]")
{
    char keySource[] = "name";
    constexpr std::string_view value = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key(std::string_view{keySource, 4}));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    keySource[0] = 'X';

    REQUIRE(node.mapping()[0].key == "name");

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == value.data());
}

TEST_CASE("YamlNodeParserDestination accepts empty root mapping",
          "[job_yaml][parser_destination][node][mapping]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
}

TEST_CASE("YamlNodeParserDestination accepts empty mapping key",
          "[job_yaml][parser_destination][node][mapping][key]")
{
    constexpr std::string_view value = "value";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key(""));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key.empty());

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == value.data());
}

TEST_CASE("YamlNodeParserDestination accepts empty owned mapping key",
          "[job_yaml][parser_destination][node][mapping][key][owned]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{}));
    REQUIRE(destination.scalarOwned(std::string{"value"}));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key.empty());
    REQUIRE(node.mapping()[0].value.ownsScalar());
    REQUIRE(node.mapping()[0].value.scalar() == "value");
}

TEST_CASE("YamlNodeParserDestination rejects scalar in mapping without key",
          "[job_yaml][parser_destination][node][mapping][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.scalar("value"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"value"}));

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlNodeParserDestination rejects null in mapping without key",
          "[job_yaml][parser_destination][node][mapping][null][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.null());

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());

    REQUIRE(destination.key("value"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].value.isNull());
}

TEST_CASE("YamlNodeParserDestination rejects second mapping key while one is pending",
          "[job_yaml][parser_destination][node][mapping][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("first"));

    REQUIRE_FALSE(destination.key("second"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"second"}));

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlNodeParserDestination rejects mapping close with pending key",
          "[job_yaml][parser_destination][node][mapping][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));

    REQUIRE_FALSE(destination.endMapping());

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Root sequence
// ========================================

TEST_CASE("YamlNodeParserDestination builds root scalar sequence",
          "[job_yaml][parser_destination][node][sequence]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view second = "two";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.scalar(first));
    REQUIRE(destination.scalar(second));

    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);

    const YamlNode &firstValue = node.sequence()[0];
    const YamlNode &secondValue = node.sequence()[1];

    REQUIRE(firstValue.isScalar());
    REQUIRE(firstValue.borrowsScalar());
    REQUIRE(firstValue.scalar() == "one");
    REQUIRE(firstValue.scalar().data() == first.data());

    REQUIRE(secondValue.isScalar());
    REQUIRE(secondValue.borrowsScalar());
    REQUIRE(secondValue.scalar() == "two");
    REQUIRE(secondValue.scalar().data() == second.data());
}

TEST_CASE("YamlNodeParserDestination builds root sequence with null item",
          "[job_yaml][parser_destination][node][sequence][null]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.null());
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);
    REQUIRE(node.sequence()[0].isNull());
}

TEST_CASE("YamlNodeParserDestination builds root sequence with mixed scalar and null items",
          "[job_yaml][parser_destination][node][sequence][null]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view third = "three";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalar(first));
    REQUIRE(destination.null());
    REQUIRE(destination.scalar(third));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 3);

    REQUIRE(node.sequence()[0].isScalar());
    REQUIRE(node.sequence()[0].scalar() == "one");

    REQUIRE(node.sequence()[1].isNull());

    REQUIRE(node.sequence()[2].isScalar());
    REQUIRE(node.sequence()[2].scalar() == "three");
}

TEST_CASE("YamlNodeParserDestination builds root sequence with owned scalars",
          "[job_yaml][parser_destination][node][sequence][owned]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalarOwned(std::string{"one"}));
    REQUIRE(destination.scalarOwned(std::string{"two"}));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);

    REQUIRE(node.sequence()[0].ownsScalar());
    REQUIRE(node.sequence()[0].scalar() == "one");

    REQUIRE(node.sequence()[1].ownsScalar());
    REQUIRE(node.sequence()[1].scalar() == "two");
}

TEST_CASE("YamlNodeParserDestination sequence scalar preserves exact source slice",
          "[job_yaml][parser_destination][node][sequence][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endSequence());

    const YamlNode &scalar = node.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

TEST_CASE("YamlNodeParserDestination accepts empty root sequence",
          "[job_yaml][parser_destination][node][sequence]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().empty());
}

TEST_CASE("YamlNodeParserDestination rejects key inside sequence",
          "[job_yaml][parser_destination][node][sequence][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());

    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"name"}));

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Nested structures
// ========================================

TEST_CASE("YamlNodeParserDestination builds mapping containing mapping",
          "[job_yaml][parser_destination][node][nested]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.key("entry"));

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 2);

    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(source));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(node.isMapping());

    const YamlNode *entry = node.member("entry");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->isMapping());

    const YamlNode *name = entry->member("name");

    REQUIRE(name != nullptr);
    REQUIRE(name->borrowsScalar());
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(name->scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination nested mapping preserves borrowed source after frame moves",
          "[job_yaml][parser_destination][node][nested][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("outer"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("inner"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    const YamlNode *outer = node.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->borrowsScalar());
    REQUIRE(inner->scalar() == "alpha");
    REQUIRE(inner->scalar().data() == source.data() + 6);
}

TEST_CASE("YamlNodeParserDestination builds mapping containing sequence",
          "[job_yaml][parser_destination][node][nested]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view second = "two";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("items"));

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalar(first));
    REQUIRE(destination.scalar(second));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());

    const YamlNode *items = node.member("items");

    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());
    REQUIRE(items->sequence().size() == 2);

    REQUIRE(items->sequence()[0].borrowsScalar());
    REQUIRE(items->sequence()[0].scalar().data() == first.data());

    REQUIRE(items->sequence()[1].borrowsScalar());
    REQUIRE(items->sequence()[1].scalar().data() == second.data());
}

TEST_CASE("YamlNodeParserDestination builds mapping containing null",
          "[job_yaml][parser_destination][node][nested][null]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("entry"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());

    const YamlNode *entry = node.member("entry");

    REQUIRE(entry != nullptr);
    REQUIRE(entry->isNull());
}

TEST_CASE("YamlNodeParserDestination builds sequence containing mapping",
          "[job_yaml][parser_destination][node][nested]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(source));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);
    REQUIRE(node.sequence()[0].isMapping());

    const YamlNode *name = node.sequence()[0].member("name");

    REQUIRE(name != nullptr);
    REQUIRE(name->borrowsScalar());
    REQUIRE(name->scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination builds sequence containing sequence",
          "[job_yaml][parser_destination][node][nested]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view second = "two";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalar(first));
    REQUIRE(destination.scalar(second));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);

    const YamlNode &nested = node.sequence()[0];

    REQUIRE(nested.isSequence());
    REQUIRE(nested.sequence().size() == 2);

    REQUIRE(nested.sequence()[0].borrowsScalar());
    REQUIRE(nested.sequence()[0].scalar() == "one");
    REQUIRE(nested.sequence()[0].scalar().data() == first.data());

    REQUIRE(nested.sequence()[1].borrowsScalar());
    REQUIRE(nested.sequence()[1].scalar() == "two");
    REQUIRE(nested.sequence()[1].scalar().data() == second.data());
}

TEST_CASE("YamlNodeParserDestination builds mixed nested structure",
          "[job_yaml][parser_destination][node][nested]")
{
    constexpr std::string_view nameValue = "alpha";
    constexpr std::string_view firstThing = "one";
    constexpr std::string_view secondThing = "two";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("entries"));

    REQUIRE(destination.beginSequence());

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(nameValue));

    REQUIRE(destination.key("things"));
    REQUIRE(destination.beginSequence());
    REQUIRE(destination.scalar(firstThing));
    REQUIRE(destination.scalar(secondThing));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.endMapping());

    REQUIRE(destination.endSequence());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(node.isMapping());

    const YamlNode *entries = node.member("entries");

    REQUIRE(entries != nullptr);
    REQUIRE(entries->isSequence());
    REQUIRE(entries->sequence().size() == 1);

    const YamlNode &entry = entries->sequence()[0];

    REQUIRE(entry.isMapping());

    const YamlNode *name = entry.member("name");
    const YamlNode *things = entry.member("things");

    REQUIRE(name != nullptr);
    REQUIRE(things != nullptr);

    REQUIRE(name->borrowsScalar());
    REQUIRE(name->scalar().data() == nameValue.data());

    REQUIRE(things->isSequence());
    REQUIRE(things->sequence().size() == 2);

    REQUIRE(things->sequence()[0].borrowsScalar());
    REQUIRE(things->sequence()[0].scalar().data() == firstThing.data());

    REQUIRE(things->sequence()[1].borrowsScalar());
    REQUIRE(things->sequence()[1].scalar().data() == secondThing.data());
}

// ========================================
// Structural mismatch rejection
// ========================================

TEST_CASE("YamlNodeParserDestination rejects mismatched mapping close",
          "[job_yaml][parser_destination][node][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginSequence());

    REQUIRE_FALSE(destination.endMapping());

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());

    REQUIRE(destination.endSequence());
    REQUIRE(destination.complete());
}

TEST_CASE("YamlNodeParserDestination rejects mismatched sequence close",
          "[job_yaml][parser_destination][node][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());

    REQUIRE(destination.endMapping());
    REQUIRE(destination.complete());
}

TEST_CASE("YamlNodeParserDestination rejects container close at root",
          "[job_yaml][parser_destination][node][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE_FALSE(destination.endMapping());
    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(destination.depth() == 0);
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlNodeParserDestination rejects nested container in mapping without pending key",
          "[job_yaml][parser_destination][node][invalid]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.beginMapping());
    REQUIRE_FALSE(destination.beginSequence());

    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());

    REQUIRE(destination.endMapping());
    REQUIRE(destination.complete());
}

// ========================================
// Borrowed state survives rejected operations
// ========================================

TEST_CASE("YamlNodeParserDestination rejected event does not disturb existing borrowed scalar",
          "[job_yaml][parser_destination][node][invalid][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.scalar(source));

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("beta"));

    const YamlNode &result = node;

    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination rejected mapping scalar leaves pending key intact",
          "[job_yaml][parser_destination][node][mapping][invalid]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("invalid"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"invalid"}));

    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(source));
    REQUIRE(destination.endMapping());

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar().data() == source.data());
}

// ========================================
// Depth limit
// ========================================

TEST_CASE("YamlNodeParserDestination accepts maximum structural depth",
          "[job_yaml][parser_destination][node][depth]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i) {
        INFO("depth " << i);
        REQUIRE(destination.beginSequence());
    }

    REQUIRE(destination.depth() == YamlIndentStack::MaxDepth);

    REQUIRE_FALSE(destination.beginSequence());

    for (std::size_t i = 0; i < YamlIndentStack::MaxDepth; ++i) {
        INFO("remaining depth " << destination.depth());
        REQUIRE(destination.endSequence());
    }

    REQUIRE(destination.depth() == 0);
    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
}

// ========================================
// Factory helpers
// ========================================

TEST_CASE("YamlNodeParserDestination createShared constructs destination",
          "[job_yaml][parser_destination][node][factory]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    auto destination = YamlNodeParserDestination::createShared(node);

    REQUIRE(destination != nullptr);
    REQUIRE_FALSE(destination->complete());
    REQUIRE(destination->depth() == 0);

    REQUIRE(destination->scalar(source));

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNodeParserDestination createUniq constructs destination",
          "[job_yaml][parser_destination][node][factory]")
{
    constexpr std::string_view source = "one";

    YamlNode node;

    auto destination = YamlNodeParserDestination::createUniq(node);

    REQUIRE(destination != nullptr);
    REQUIRE_FALSE(destination->complete());
    REQUIRE(destination->depth() == 0);

    REQUIRE(destination->beginSequence());
    REQUIRE(destination->scalar(source));
    REQUIRE(destination->endSequence());

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);

    const YamlNode &value = node.sequence()[0];

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar() == "one");
    REQUIRE(value.scalar().data() == source.data());
}

} // namespace job::yaml::tests