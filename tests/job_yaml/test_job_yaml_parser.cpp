#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include "test_job_yaml_utils.h"
#endif

#include <string_view>

#include "test_job_yaml_fixtures.h"

#include <job_yaml_node.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_parser.h>
#include <job_yaml_parser_destination.h>

namespace job::yaml::tests {

// ========================================
// End-to-end reflected object parsing
// ========================================

TEST_CASE("YamlParser parses reflected scalar mapping end to end",
          "[job_yaml][parser][object][mapping]")
{
    constexpr std::string_view source =
        "name: alpha\n"
        "count: 42\n"
        "enabled: true\n"
        "ratio: 1.5\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
    REQUIRE(object.enabled);
    REQUIRE(object.ratio == 1.5);
}

TEST_CASE("YamlParser parses single reflected mapping member",
          "[job_yaml][parser][object][mapping]")
{
    constexpr std::string_view source = "name: alpha\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlParser parses reflected mapping without trailing line break",
          "[job_yaml][parser][object][mapping]")
{
    constexpr std::string_view source =
        "name: alpha\n"
        "count: 42";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses reflected mapping with blank lines",
          "[job_yaml][parser][object][mapping]")
{
    constexpr std::string_view source =
        "\n"
        "\n"
        "name: alpha\n"
        "\n"
        "count: 42\n"
        "\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses reflected mapping with comments",
          "[job_yaml][parser][object][mapping][comment]")
{
    constexpr std::string_view source =
        "# configuration\n"
        "name: alpha\n"
        "# count follows\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses reflected mapping with trailing comments",
          "[job_yaml][parser][object][mapping][comment]")
{
    constexpr std::string_view source =
        "name: alpha # label\n"
        "count: 42 # value\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

// ========================================
// Nested reflected mappings
// ========================================

TEST_CASE("YamlParser parses nested reflected mapping",
          "[job_yaml][parser][object][mapping][nested]")
{
    constexpr std::string_view source =
        "address:\n"
        "  city: Delta\n"
        "  zip: 12345\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
}

TEST_CASE("YamlParser parses nested reflected mapping with parent members",
          "[job_yaml][parser][object][mapping][nested]")
{
    constexpr std::string_view source =
        "name: alpha\n"
        "address:\n"
        "  city: Delta\n"
        "  zip: 12345\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser resumes parent mapping after nested mapping",
          "[job_yaml][parser][object][mapping][nested][dedent]")
{
    constexpr std::string_view source =
        "address:\n"
        "  city: Delta\n"
        "  zip: 12345\n"
        "enabled: true\n"
        "ratio: 2.5\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
    REQUIRE(object.enabled);
    REQUIRE(object.ratio == 2.5);
}

TEST_CASE("YamlParser parses deeply nested reflected mapping",
          "[job_yaml][parser][object][mapping][nested]")
{
    constexpr std::string_view source =
        "profile:\n"
        "  displayName: beta\n"
        "  address:\n"
        "    city: Delta\n"
        "    zip: 54321\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.profile.displayName == "beta");
    REQUIRE(object.profile.address.city == "Delta");
    REQUIRE(object.profile.address.zip == 54321);
}

TEST_CASE("YamlParser resumes each parent after deep nested mapping",
          "[job_yaml][parser][object][mapping][nested][dedent]")
{
    constexpr std::string_view source =
        "profile:\n"
        "  address:\n"
        "    city: Delta\n"
        "    zip: 54321\n"
        "  displayName: beta\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.profile.address.city == "Delta");
    REQUIRE(object.profile.address.zip == 54321);
    REQUIRE(object.profile.displayName == "beta");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses blank lines inside nested mapping",
          "[job_yaml][parser][object][mapping][nested]")
{
    constexpr std::string_view source =
        "address:\n"
        "\n"
        "  city: Delta\n"
        "\n"
        "  zip: 12345\n"
        "\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser parses comments inside nested mapping",
          "[job_yaml][parser][object][mapping][nested][comment]")
{
    constexpr std::string_view source =
        "address:\n"
        "  # city follows\n"
        "  city: Delta\n"
        "  # postal value follows\n"
        "  zip: 12345\n"
        "count: 42\n";

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
    REQUIRE(object.count == 42);
}

// ========================================
// Root scalar parsing
// ========================================

TEST_CASE("YamlParser forwards root scalar to destination",
          "[job_yaml][parser][scalar]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.scalar() == "alpha");
}

TEST_CASE("YamlParser root scalar borrows exact source view",
          "[job_yaml][parser][scalar][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const std::string_view scalar = result.scalar();

    REQUIRE(destination.complete());
    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(scalar == "alpha");
    REQUIRE(scalar.data() == source.data());
    REQUIRE(scalar.size() == source.size());
}

TEST_CASE("YamlParser preserves root scalar string_view boundaries",
          "[job_yaml][parser][scalar][source_view]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(value, destination));

    const YamlNode &result = node;
    const std::string_view scalar = result.scalar();

    REQUIRE(destination.complete());
    REQUIRE(result.borrowsScalar());
    REQUIRE(scalar == "alpha");
    REQUIRE(scalar.data() == source);
    REQUIRE(scalar.size() == 5);
}

TEST_CASE("YamlParser trims root scalar range without copying",
          "[job_yaml][parser][scalar][source_view]")
{
    constexpr std::string_view source = "alpha   \t";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const std::string_view scalar = result.scalar();

    REQUIRE(destination.complete());
    REQUIRE(result.borrowsScalar());
    REQUIRE(scalar == "alpha");
    REQUIRE(scalar.data() == source.data());
    REQUIRE(scalar.size() == 5);
}

TEST_CASE("YamlParser accepts root scalar followed by line break",
          "[job_yaml][parser][scalar]")
{
    constexpr std::string_view source = "alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlParser accepts root scalar followed by comment",
          "[job_yaml][parser][scalar][comment]")
{
    constexpr std::string_view source = "alpha # comment\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().data() == source.data());
    REQUIRE(result.scalar().size() == 5);
}

// ========================================
// Block sequences
// ========================================

TEST_CASE("YamlParser parses root block sequence",
          "[job_yaml][parser][sequence]")
{
    constexpr std::string_view source =
        "- alpha\n"
        "- beta\n"
        "- gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
}

TEST_CASE("YamlParser root block sequence scalars borrow source views",
          "[job_yaml][parser][sequence][source_view]")
{
    constexpr std::string_view source =
        "- alpha\n"
        "- beta\n"
        "- gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const auto &sequence = result.sequence();

    REQUIRE(destination.complete());
    REQUIRE(sequence.size() == 3);

    REQUIRE(sequence[0].borrowsScalar());
    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[0].scalar().data() == source.data() + 2);

    REQUIRE(sequence[1].borrowsScalar());
    REQUIRE(sequence[1].scalar() == "beta");
    REQUIRE(sequence[1].scalar().data() == source.data() + 10);

    REQUIRE(sequence[2].borrowsScalar());
    REQUIRE(sequence[2].scalar() == "gamma");
    REQUIRE(sequence[2].scalar().data() == source.data() + 17);
}

TEST_CASE("YamlParser parses mapping containing block sequence",
          "[job_yaml][parser][mapping][sequence]")
{
    constexpr std::string_view source =
        "items:\n"
        "  - alpha\n"
        "  - beta\n"
        "  - gamma\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
}

TEST_CASE("YamlParser mapping sequence values retain source views",
          "[job_yaml][parser][mapping][sequence][source_view]")
{
    constexpr std::string_view source =
        "items:\n"
        "  - alpha\n"
        "  - beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *items = result.member("items");

    REQUIRE(destination.complete());
    REQUIRE(items != nullptr);
    REQUIRE(items->isSequence());

    const auto &sequence = items->sequence();

    REQUIRE(sequence.size() == 2);

    REQUIRE(sequence[0].borrowsScalar());
    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[0].scalar().data() == source.data() + 11);

    REQUIRE(sequence[1].borrowsScalar());
    REQUIRE(sequence[1].scalar() == "beta");
    REQUIRE(sequence[1].scalar().data() == source.data() + 21);
}

TEST_CASE("YamlParser parses nested block sequence",
          "[job_yaml][parser][mapping][sequence][nested]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  inner:\n"
        "    - alpha\n"
        "    - beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
}

TEST_CASE("YamlParser nested sequence scalars remain borrowed after container moves",
          "[job_yaml][parser][mapping][sequence][nested][source_view]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  inner:\n"
        "    - alpha\n"
        "    - beta\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *outer = result.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->isSequence());

    const auto &sequence = inner->sequence();

    REQUIRE(sequence.size() == 2);
    REQUIRE(sequence[0].borrowsScalar());
    REQUIRE(sequence[1].borrowsScalar());

    REQUIRE(sequence[0].scalar() == "alpha");
    REQUIRE(sequence[1].scalar() == "beta");

    const std::size_t alphaOffset = source.find("alpha");
    const std::size_t betaOffset = source.find("beta");

    REQUIRE(alphaOffset != std::string_view::npos);
    REQUIRE(betaOffset != std::string_view::npos);

    REQUIRE(sequence[0].scalar().data() == source.data() + alphaOffset);
    REQUIRE(sequence[1].scalar().data() == source.data() + betaOffset);
}

TEST_CASE("YamlParser parses mapping entries inside block sequence",
          "[job_yaml][parser][sequence][mapping]")
{
    constexpr std::string_view source =
        "- name: alpha\n"
        "  count: 1\n"
        "- name: beta\n"
        "  count: 2\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isSequence());
}

TEST_CASE("YamlParser sequence mapping scalar values retain source views",
          "[job_yaml][parser][sequence][mapping][source_view]")
{
    constexpr std::string_view source =
        "- name: alpha\n"
        "  count: 1\n"
        "- name: beta\n"
        "  count: 2\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const auto &sequence = result.sequence();

    REQUIRE(sequence.size() == 2);

    const YamlNode *firstName = sequence[0].member("name");
    const YamlNode *firstCount = sequence[0].member("count");
    const YamlNode *secondName = sequence[1].member("name");
    const YamlNode *secondCount = sequence[1].member("count");

    REQUIRE(firstName != nullptr);
    REQUIRE(firstCount != nullptr);
    REQUIRE(secondName != nullptr);
    REQUIRE(secondCount != nullptr);

    REQUIRE(firstName->borrowsScalar());
    REQUIRE(firstName->scalar() == "alpha");
    REQUIRE(firstName->scalar().data() == source.data() + 8);

    REQUIRE(firstCount->borrowsScalar());
    REQUIRE(firstCount->scalar() == "1");

    REQUIRE(secondName->borrowsScalar());
    REQUIRE(secondName->scalar() == "beta");

    REQUIRE(secondCount->borrowsScalar());
    REQUIRE(secondCount->scalar() == "2");
}

TEST_CASE("YamlParser parses mapping containing sequence of mappings",
          "[job_yaml][parser][mapping][sequence][nested]")
{
    constexpr std::string_view source =
        "items:\n"
        "  - name: alpha\n"
        "    count: 1\n"
        "  - name: beta\n"
        "    count: 2\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    REQUIRE(destination.complete());
    REQUIRE(node.isMapping());
}

// ========================================
// Mapping scalar source views
// ========================================

TEST_CASE("YamlParser mapping scalar values borrow exact source slices",
          "[job_yaml][parser][mapping][source_view]")
{
    constexpr std::string_view source =
        "name: alpha\n"
        "count: 42\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;

    const YamlNode *name = result.member("name");
    const YamlNode *count = result.member("count");

    REQUIRE(name != nullptr);
    REQUIRE(count != nullptr);

    REQUIRE(name->borrowsScalar());
    REQUIRE(name->scalar() == "alpha");
    REQUIRE(name->scalar().data() == source.data() + 6);
    REQUIRE(name->scalar().size() == 5);

    REQUIRE(count->borrowsScalar());
    REQUIRE(count->scalar() == "42");
    REQUIRE(count->scalar().data() == source.data() + 19);
    REQUIRE(count->scalar().size() == 2);
}

TEST_CASE("YamlParser nested mapping scalar retains exact source slice",
          "[job_yaml][parser][mapping][nested][source_view]")
{
    constexpr std::string_view source =
        "outer:\n"
        "  inner: alpha\n";

    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse(source, destination));

    const YamlNode &result = node;
    const YamlNode *outer = result.member("outer");

    REQUIRE(outer != nullptr);

    const YamlNode *inner = outer->member("inner");

    REQUIRE(inner != nullptr);
    REQUIRE(inner->borrowsScalar());
    REQUIRE(inner->scalar() == "alpha");
    REQUIRE(inner->scalar().data() == source.data() + 16);
    REQUIRE(inner->scalar().size() == 5);
}

// ========================================
// Scalar conversion
// ========================================

TEST_CASE("YamlParser assigns reflected integer member",
          "[job_yaml][parser][object][scalar]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("count: 42\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser assigns reflected boolean member",
          "[job_yaml][parser][object][scalar]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("enabled: true\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.enabled);
}

TEST_CASE("YamlParser assigns reflected floating point member",
          "[job_yaml][parser][object][scalar]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("ratio: 1.5\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.ratio == 1.5);
}

TEST_CASE("YamlParser preserves reflected string member exactly",
          "[job_yaml][parser][object][scalar]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: alpha beta\n", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha beta");
}

TEST_CASE("YamlParser assigns reflected scalar inside nested mapping",
          "[job_yaml][parser][object][scalar][nested]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(
        "address:\n"
        "  zip: 12345\n",
        destination));

    REQUIRE(destination.complete());
    REQUIRE(object.address.zip == 12345);
}

TEST_CASE("YamlParser reflected integer consumes bounded source slice",
          "[job_yaml][parser][object][scalar][source_view]")
{
    constexpr char source[] = "count: 42\n-extra";
    const std::string_view value{source, 10};

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(value, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlParser reflected boolean consumes bounded source slice",
          "[job_yaml][parser][object][scalar][source_view]")
{
    constexpr char source[] = "enabled: true\n-extra";
    const std::string_view value{source, 14};

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(value, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.enabled);
}

TEST_CASE("YamlParser reflected floating point consumes bounded source slice",
          "[job_yaml][parser][object][scalar][source_view]")
{
    constexpr char source[] = "ratio: 12.5\n-extra";
    const std::string_view value{source, 12};

    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse(value, destination));

    REQUIRE(destination.complete());
    REQUIRE(object.ratio == 12.5);
}

// ========================================
// Invalid source
// ========================================

TEST_CASE("YamlParser rejects empty source",
          "[job_yaml][parser][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("", destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects comment-only source",
          "[job_yaml][parser][invalid][comment]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("# nothing here\n", destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects indented root mapping",
          "[job_yaml][parser][invalid][indent]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "  name: alpha\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unknown reflected member",
          "[job_yaml][parser][object][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("missing: value\n", destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unknown nested reflected member",
          "[job_yaml][parser][object][nested][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "address:\n"
        "  missing: value\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects invalid reflected integer",
          "[job_yaml][parser][object][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("count: potato\n", destination));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(object.count == 0);
}

TEST_CASE("YamlParser rejects invalid reflected integer inside nested mapping",
          "[job_yaml][parser][object][nested][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "address:\n"
        "  zip: potato\n",
        destination));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(object.address.zip == 0);
}

TEST_CASE("YamlParser rejects missing mapping value",
          "[job_yaml][parser][mapping][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse("name:\n", destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects empty nested mapping value",
          "[job_yaml][parser][mapping][nested][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "address:\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects second scalar where mapping key is expected",
          "[job_yaml][parser][mapping][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "name: alpha\n"
        "orphan\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects unexpected deeper indentation",
          "[job_yaml][parser][mapping][invalid][indent]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "name: alpha\n"
        "  count: 42\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects invalid dedent between established levels",
          "[job_yaml][parser][mapping][nested][invalid][indent]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "profile:\n"
        "  address:\n"
        "    city: Delta\n"
        "   displayName: beta\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects sequence for reflected object destination",
          "[job_yaml][parser][object][sequence][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "- alpha\n"
        "- beta\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser rejects nested sequence for reflected object destination",
          "[job_yaml][parser][object][sequence][nested][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "address:\n"
        "  - alpha\n"
        "  - beta\n",
        destination));

    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser parses flow mapping syntax",
          "[job_yaml][parser][flow][mapping]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("{name: alpha}", destination));

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlParser parses flow sequence syntax",
          "[job_yaml][parser][flow][sequence]")
{
    YamlNode node;
    YamlNodeParserDestination destination{node};

    REQUIRE(YamlParser::parse("[alpha, beta]", destination));

    const YamlNode &result = node;

    REQUIRE(destination.complete());
    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().size() == 2);
    REQUIRE(result.sequence()[0].scalar() == "alpha");
    REQUIRE(result.sequence()[1].scalar() == "beta");
}

// ========================================
// Destination state
// ========================================

TEST_CASE("YamlParser rejects parsing into completed destination",
          "[job_yaml][parser][destination][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(YamlParser::parse("name: alpha\n", destination));
    REQUIRE(destination.complete());

    REQUIRE_FALSE(YamlParser::parse("count: 42\n", destination));

    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 0);
}

TEST_CASE("YamlParser leaves successfully assigned members before later failure",
          "[job_yaml][parser][object][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "name: alpha\n"
        "count: potato\n",
        destination));

    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 0);
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlParser leaves successfully assigned nested members before later failure",
          "[job_yaml][parser][object][nested][invalid]")
{
    ParserDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(YamlParser::parse(
        "address:\n"
        "  city: Delta\n"
        "  zip: potato\n",
        destination));

    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 0);
    REQUIRE_FALSE(destination.complete());
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlParser reflected mapping benchmark",
          "[job_yaml][parser][object][benchmark]")
{
    constexpr std::string_view source =
        "name: alpha\n"
        "count: 42\n"
        "enabled: true\n"
        "ratio: 1.5\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("YamlParser reflected mapping")
    {
        ParserDestinationFixture object;
        YamlObjectParserDestination destination{object};

        benchmarkDoNotOptimize(source);

        const bool result = YamlParser::parse(source, destination);

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.count +
               static_cast<int>(object.name.size()) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.ratio);
    };
}

TEST_CASE("YamlParser nested reflected mapping benchmark",
          "[job_yaml][parser][object][nested][benchmark]")
{
    constexpr std::string_view source =
        "profile:\n"
        "  displayName: beta\n"
        "  address:\n"
        "    city: Delta\n"
        "    zip: 54321\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("YamlParser nested reflected mapping")
    {
        ParserDestinationFixture object;
        YamlObjectParserDestination destination{object};

        benchmarkDoNotOptimize(source);

        const bool result = YamlParser::parse(source, destination);

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.profile.address.zip +
               static_cast<int>(object.profile.displayName.size()) +
               static_cast<int>(object.profile.address.city.size());
    };
}

TEST_CASE("YamlParser single reflected scalar benchmark",
          "[job_yaml][parser][object][benchmark]")
{
    constexpr std::string_view source = "count: 42\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("YamlParser single reflected scalar")
    {
        ParserDestinationFixture object;
        YamlObjectParserDestination destination{object};

        benchmarkDoNotOptimize(source);

        const bool result = YamlParser::parse(source, destination);

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlParser root scalar benchmark",
          "[job_yaml][parser][scalar][benchmark]")
{
    constexpr std::string_view source = "alpha";

    benchmarkDoNotOptimize(source);

    BENCHMARK("YamlParser root scalar")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        benchmarkDoNotOptimize(source);

        const bool result = YamlParser::parse(source, destination);

        const YamlNode &parsed = node;

        benchmarkClobber(node);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return parsed.scalar().size();
    };
}

TEST_CASE("YamlParser root sequence benchmark",
          "[job_yaml][parser][sequence][benchmark]")
{
    constexpr std::string_view source =
        "- alpha\n"
        "- beta\n"
        "- gamma\n";

    benchmarkDoNotOptimize(source);

    BENCHMARK("YamlParser root sequence")
    {
        YamlNode node;
        YamlNodeParserDestination destination{node};

        benchmarkDoNotOptimize(source);

        const bool result = YamlParser::parse(source, destination);

        benchmarkClobber(node);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return result;
    };
}

#endif

} // namespace job::yaml::tests