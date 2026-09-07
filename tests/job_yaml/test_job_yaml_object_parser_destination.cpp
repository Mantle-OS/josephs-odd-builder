#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include "test_job_yaml_utils.h"
#endif

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "test_job_yaml_fixtures.h"

#include <job_yaml_concepts.h>
#include <job_yaml_object_parser_destination.h>
#include <job_yaml_object_reader.h>

namespace job::yaml::tests {
struct NullableObjectDestinationFixture
{
    int scalarValue{7};
    std::optional<int> optionalValue{42};
    std::shared_ptr<int> sharedValue{std::make_shared<int>(43)};
    std::unique_ptr<int> uniqueValue{std::make_unique<int>(44)};
};

static_assert(YamlParseDestination<YamlObjectParserDestination<ObjectDestinationFixture>>);
static_assert(YamlParseDestination<YamlObjectParserDestination<NullableObjectDestinationFixture>>);
static_assert(YamlParseDestination<YamlObjectParserDestination<std::vector<std::optional<int>>>>);
static_assert(YamlParseDestination<YamlObjectParserDestination<std::vector<std::shared_ptr<int>>>>);
static_assert(YamlParseDestination<YamlObjectParserDestination<std::vector<std::unique_ptr<int>>>>);

// ========================================
// Initial state
// ========================================

TEST_CASE("YamlObjectParserDestination starts incomplete at root depth",
          "[job_yaml][parser_destination][object]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 0);
}

// ========================================
// Reflected object destination
// ========================================

TEST_CASE("YamlObjectParserDestination builds root mapping with reflected scalar members",
          "[job_yaml][parser_destination][object][mapping]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar("alpha"));

    REQUIRE(destination.key("count"));
    REQUIRE(destination.scalar("42"));

    REQUIRE(destination.key("enabled"));
    REQUIRE(destination.scalar("true"));

    REQUIRE(destination.key("ratio"));
    REQUIRE(destination.scalar("1.5"));

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
    REQUIRE(object.enabled);
    REQUIRE(object.ratio == 1.5);
}

TEST_CASE("YamlObjectParserDestination builds nested reflected mapping",
          "[job_yaml][parser_destination][object][nested]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 2);

    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalar("Delta"));

    REQUIRE(destination.key("zip"));
    REQUIRE(destination.scalar("12345"));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.zip == 12345);
}

TEST_CASE("YamlObjectParserDestination builds deeply nested reflected mappings",
          "[job_yaml][parser_destination][object][nested]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("profile"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 2);

    REQUIRE(destination.key("displayName"));
    REQUIRE(destination.scalar("beta"));

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.depth() == 3);

    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalar("Delta"));

    REQUIRE(destination.key("zip"));
    REQUIRE(destination.scalar("54321"));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.depth() == 2);

    REQUIRE(destination.endMapping());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());

    REQUIRE(object.profile.displayName == "beta");
    REQUIRE(object.profile.address.city == "Delta");
    REQUIRE(object.profile.address.zip == 54321);
}

TEST_CASE("YamlObjectParserDestination resumes parent after nested mapping",
          "[job_yaml][parser_destination][object][nested]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalar("Delta"));

    REQUIRE(destination.endMapping());

    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar("alpha"));

    REQUIRE(destination.key("count"));
    REQUIRE(destination.scalar("42"));

    REQUIRE(destination.endMapping());

    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.name == "alpha");
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination accepts empty nested reflected mapping",
          "[job_yaml][parser_destination][object][nested]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(object.address.city.empty());
    REQUIRE(object.address.zip == 0);
}

TEST_CASE("YamlObjectParserDestination accepts empty root mapping",
          "[job_yaml][parser_destination][object][mapping]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
}

// ========================================
// Source-view routing
// ========================================

TEST_CASE("YamlObjectParserDestination preserves string_view length",
          "[job_yaml][parser_destination][object][scalar][source_view]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
    REQUIRE(object.name.size() == 5);
}

TEST_CASE("YamlObjectParserDestination reflected string intentionally owns source slice",
          "[job_yaml][parser_destination][object][scalar][source_view]")
{
    char source[] = "alpha";
    const std::string_view value{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
    REQUIRE(object.name.data() != value.data());

    source[0] = 'X';

    REQUIRE(value == "Xlpha");
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination preserves nested string_view length",
          "[job_yaml][parser_destination][object][nested][scalar][source_view]")
{
    constexpr char source[] = "Delta-extra";
    const std::string_view value{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalar(value));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.address.city == "Delta");
    REQUIRE(object.address.city.size() == 5);
}

TEST_CASE("YamlObjectParserDestination reflected integer consumes exact source slice",
          "[job_yaml][parser_destination][object][scalar][source_view]")
{
    constexpr std::string_view source = "prefix42suffix";
    const std::string_view value = source.substr(6, 2);

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(value == "42");
    REQUIRE(value.data() == source.data() + 6);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("count"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination reflected boolean consumes exact source slice",
          "[job_yaml][parser_destination][object][scalar][source_view]")
{
    constexpr std::string_view source = "prefixtruesuffix";
    const std::string_view value = source.substr(6, 4);

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(value == "true");
    REQUIRE(value.data() == source.data() + 6);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("enabled"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectParserDestination reflected floating point consumes exact source slice",
          "[job_yaml][parser_destination][object][scalar][source_view]")
{
    constexpr std::string_view source = "prefix12.5suffix";
    const std::string_view value = source.substr(6, 4);

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(value == "12.5");
    REQUIRE(value.data() == source.data() + 6);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("ratio"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.ratio == 12.5);
}

TEST_CASE("YamlObjectParserDestination nested integer consumes exact source slice",
          "[job_yaml][parser_destination][object][nested][scalar][source_view]")
{
    constexpr std::string_view source = "prefix12345suffix";
    const std::string_view value = source.substr(6, 5);

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(value == "12345");
    REQUIRE(value.data() == source.data() + 6);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("zip"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.address.zip == 12345);
}

TEST_CASE("YamlObjectParserDestination key consumes bounded string_view",
          "[job_yaml][parser_destination][object][source_view]")
{
    constexpr char source[] = "count-extra";
    const std::string_view key{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key(key));
    REQUIRE(destination.scalar("42"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination pending key remains borrowed until scalar routing",
          "[job_yaml][parser_destination][object][source_view]")
{
    char source[] = "count";
    const std::string_view key{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key(key));

    REQUIRE(destination.scalar("42"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination source slice boundaries exclude surrounding numeric bytes",
          "[job_yaml][parser_destination][object][source_view]")
{
    constexpr char source[] = {
        'x',
        '4', '2',
        'x'
    };

    const std::string_view value{source + 1, 2};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("count"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination preserves embedded null in reflected string",
          "[job_yaml][parser_destination][object][source_view]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    const std::string_view value{source, sizeof(source)};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name.size() == sizeof(source));
    REQUIRE(object.name[0] == 'A');
    REQUIRE(object.name[1] == '\0');
    REQUIRE(object.name[2] == 'B');
}

// ========================================
// Owned scalar routing
// ========================================

TEST_CASE("YamlObjectParserDestination routes owned reflected string scalar",
          "[job_yaml][parser_destination][object][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalarOwned(std::string{"alpha"}));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination routes owned reflected integer scalar",
          "[job_yaml][parser_destination][object][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("count"));
    REQUIRE(destination.scalarOwned(std::string{"42"}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination routes owned reflected boolean scalar",
          "[job_yaml][parser_destination][object][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("enabled"));
    REQUIRE(destination.scalarOwned(std::string{"true"}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectParserDestination routes owned reflected floating point scalar",
          "[job_yaml][parser_destination][object][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("ratio"));
    REQUIRE(destination.scalarOwned(std::string{"12.5"}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.ratio == 12.5);
}

TEST_CASE("YamlObjectParserDestination routes owned scalar through nested reflected mapping",
          "[job_yaml][parser_destination][object][nested][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalarOwned(std::string{"Delta"}));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.address.city == "Delta");
}

TEST_CASE("YamlObjectParserDestination preserves embedded null in owned reflected string",
          "[job_yaml][parser_destination][object][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalarOwned(std::string{"A\0B", 3}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name.size() == 3);
    REQUIRE(object.name[0] == 'A');
    REQUIRE(object.name[1] == '\0');
    REQUIRE(object.name[2] == 'B');
}

// ========================================
// Owned key routing
// ========================================

TEST_CASE("YamlObjectParserDestination routes owned key to reflected scalar member",
          "[job_yaml][parser_destination][object][key][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"count"}));
    REQUIRE(destination.scalar("42"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination routes owned key with owned scalar",
          "[job_yaml][parser_destination][object][key][owned][scalar][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"name"}));
    REQUIRE(destination.scalarOwned(std::string{"alpha"}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination owned key survives until nested mapping resolution",
          "[job_yaml][parser_destination][object][nested][key][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"address"}));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.keyOwned(std::string{"city"}));
    REQUIRE(destination.scalarOwned(std::string{"Delta"}));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.address.city == "Delta");
}

TEST_CASE("YamlObjectParserDestination owned key survives frame growth",
          "[job_yaml][parser_destination][object][nested][key][owned]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"profile"}));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.keyOwned(std::string{"address"}));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.keyOwned(std::string{"zip"}));
    REQUIRE(destination.scalarOwned(std::string{"54321"}));

    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.profile.address.zip == 54321);
}

TEST_CASE("YamlObjectParserDestination preserves owned pending key after failed nested mapping",
          "[job_yaml][parser_destination][object][nested][key][owned][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"name"}));

    REQUIRE_FALSE(destination.beginMapping());
    REQUIRE_FALSE(destination.key("count"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"count"}));

    REQUIRE(destination.scalarOwned(std::string{"alpha"}));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

// ========================================
// Invalid root state
// ========================================

TEST_CASE("YamlObjectParserDestination rejects scalar before root mapping",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("alpha"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"alpha"}));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 0);
}
TEST_CASE("YamlObjectParserDestination rejects key before root mapping",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"name"}));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 0);
}

// ========================================
// Invalid mapping state
// ========================================

TEST_CASE("YamlObjectParserDestination rejects scalar without pending key",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("alpha"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"alpha"}));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);
}

TEST_CASE("YamlObjectParserDestination rejects second key while one is pending",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("name"));
    REQUIRE_FALSE(destination.key("count"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"count"}));

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination rejects mapping close with pending key",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));

    REQUIRE_FALSE(destination.endMapping());

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.complete());
}

TEST_CASE("YamlObjectParserDestination rejects nested mapping close with pending key",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("city"));
    REQUIRE_FALSE(destination.endMapping());

    REQUIRE(destination.depth() == 2);

    REQUIRE(destination.scalar("Delta"));
    REQUIRE(destination.endMapping());
    REQUIRE(destination.endMapping());

    REQUIRE(object.address.city == "Delta");
}

// ========================================
// Reflected dispatch failures
// ========================================

TEST_CASE("YamlObjectParserDestination rejects unknown reflected member",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("missing"));

    REQUIRE_FALSE(destination.scalar("value"));

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.endMapping());
}

TEST_CASE("YamlObjectParserDestination rejects unknown nested reflected member",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("missing"));
    REQUIRE_FALSE(destination.scalar("value"));

    REQUIRE(destination.depth() == 2);
    REQUIRE_FALSE(destination.key("city"));
    REQUIRE_FALSE(destination.endMapping());
}

TEST_CASE("YamlObjectParserDestination rejects invalid reflected scalar conversion",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("count"));

    REQUIRE_FALSE(destination.scalar("potato"));

    REQUIRE(object.count == 0);
    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    REQUIRE_FALSE(destination.key("name"));
    REQUIRE_FALSE(destination.endMapping());
}

TEST_CASE("YamlObjectParserDestination rejects invalid bounded reflected scalar conversion",
          "[job_yaml][parser_destination][object][invalid][source_view]")
{
    constexpr std::string_view source = "prefixbad suffix";
    const std::string_view value = source.substr(6, 3);

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("count"));

    REQUIRE_FALSE(destination.scalar(value));

    REQUIRE(object.count == 0);
    REQUIRE(destination.depth() == 1);
    REQUIRE_FALSE(destination.complete());
}

TEST_CASE("YamlObjectParserDestination rejects invalid nested reflected scalar conversion",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("zip"));
    REQUIRE_FALSE(destination.scalar("potato"));

    REQUIRE(object.address.zip == 0);
    REQUIRE(destination.depth() == 2);

    REQUIRE_FALSE(destination.key("city"));
    REQUIRE_FALSE(destination.endMapping());
}

// ========================================
// Nested mapping validation
// ========================================

TEST_CASE("YamlObjectParserDestination rejects nested mapping for scalar member",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));

    REQUIRE_FALSE(destination.beginMapping());

    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination preserves pending key after failed nested mapping",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));

    REQUIRE_FALSE(destination.beginMapping());

    REQUIRE_FALSE(destination.key("count"));

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination preserves bounded pending key after failed nested mapping",
          "[job_yaml][parser_destination][object][nested][invalid][source_view]")
{
    constexpr char source[] = "name-extra";
    const std::string_view key{source, 4};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key(key));

    REQUIRE_FALSE(destination.beginMapping());

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination rejects nested mapping without pending key",
          "[job_yaml][parser_destination][object][nested][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE_FALSE(destination.beginMapping());

    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.endMapping());
    REQUIRE(destination.complete());
}

// ========================================
// Unsupported sequence destination
// ========================================

TEST_CASE("YamlObjectParserDestination rejects sequences",
          "[job_yaml][parser_destination][object][sequence][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(destination.beginSequence());
    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));

    REQUIRE_FALSE(destination.beginSequence());
    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());
}

// ========================================
// Completion state
// ========================================

TEST_CASE("YamlObjectParserDestination completes only after root mapping closes",
          "[job_yaml][parser_destination][object][nested]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("address"));
    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("city"));
    REQUIRE(destination.scalar("Delta"));

    REQUIRE(destination.endMapping());

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
}

TEST_CASE("YamlObjectParserDestination rejects events after completion",
          "[job_yaml][parser_destination][object][invalid]")
{
    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar("alpha"));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());

    REQUIRE_FALSE(destination.null());
    REQUIRE_FALSE(destination.scalar("again"));
    REQUIRE_FALSE(destination.scalarOwned(std::string{"again"}));
    REQUIRE_FALSE(destination.key("count"));
    REQUIRE_FALSE(destination.keyOwned(std::string{"count"}));
    REQUIRE_FALSE(destination.beginMapping());
    REQUIRE_FALSE(destination.endMapping());
    REQUIRE_FALSE(destination.beginSequence());
    REQUIRE_FALSE(destination.endSequence());

    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectParserDestination completed destination preserves assigned source slice",
          "[job_yaml][parser_destination][object][source_view]")
{
    char source[] = "alpha";
    const std::string_view value{source, 5};

    ObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("name"));
    REQUIRE(destination.scalar(value));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());

    source[0] = 'X';

    REQUIRE(object.name == "alpha");
}

// ========================================
// Factory helpers
// ========================================

TEST_CASE("YamlObjectParserDestination createShared constructs destination",
          "[job_yaml][parser_destination][object][factory]")
{
    ObjectDestinationFixture object;

    auto destination = YamlObjectParserDestination<ObjectDestinationFixture>::createShared(object);

    REQUIRE(destination != nullptr);
    REQUIRE_FALSE(destination->complete());
    REQUIRE(destination->depth() == 0);

    REQUIRE(destination->beginMapping());
    REQUIRE(destination->key("count"));
    REQUIRE(destination->scalar("42"));
    REQUIRE(destination->endMapping());

    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectParserDestination createUniq constructs destination",
          "[job_yaml][parser_destination][object][factory]")
{
    ObjectDestinationFixture object;

    auto destination = YamlObjectParserDestination<ObjectDestinationFixture>::createUniq(object);

    REQUIRE(destination != nullptr);
    REQUIRE_FALSE(destination->complete());
    REQUIRE(destination->depth() == 0);

    REQUIRE(destination->beginMapping());
    REQUIRE(destination->key("enabled"));
    REQUIRE(destination->scalar("true"));
    REQUIRE(destination->endMapping());

    REQUIRE(object.enabled);
}



// ========================================
// Null routing
// ========================================

TEST_CASE("YamlObjectParserDestination rejects null before root mapping",
          "[job_yaml][parser_destination][object][null][invalid]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE_FALSE(destination.null());

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(object.scalarValue == 7);
    REQUIRE(object.optionalValue == 42);
    REQUIRE(object.sharedValue != nullptr);
    REQUIRE(object.uniqueValue != nullptr);
}

TEST_CASE("YamlObjectParserDestination resets optional member from null",
          "[job_yaml][parser_destination][object][null][optional]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(object.optionalValue.has_value());

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("optionalValue"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE_FALSE(object.optionalValue.has_value());
}

TEST_CASE("YamlObjectParserDestination resets shared pointer member from null",
          "[job_yaml][parser_destination][object][null][pointer][shared]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(object.sharedValue != nullptr);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("sharedValue"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(object.sharedValue == nullptr);
}

TEST_CASE("YamlObjectParserDestination resets unique pointer member from null",
          "[job_yaml][parser_destination][object][null][pointer][unique]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(object.uniqueValue != nullptr);

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("uniqueValue"));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);
    REQUIRE(object.uniqueValue == nullptr);
}

TEST_CASE("YamlObjectParserDestination resets nullable members in one mapping",
          "[job_yaml][parser_destination][object][null]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());

    REQUIRE(destination.key("optionalValue"));
    REQUIRE(destination.null());

    REQUIRE(destination.key("sharedValue"));
    REQUIRE(destination.null());

    REQUIRE(destination.key("uniqueValue"));
    REQUIRE(destination.null());

    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE_FALSE(object.optionalValue.has_value());
    REQUIRE(object.sharedValue == nullptr);
    REQUIRE(object.uniqueValue == nullptr);
}

TEST_CASE("YamlObjectParserDestination routes owned key to nullable member",
          "[job_yaml][parser_destination][object][null][key][owned]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.keyOwned(std::string{"optionalValue"}));
    REQUIRE(destination.null());
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE_FALSE(object.optionalValue.has_value());
}

TEST_CASE("YamlObjectParserDestination rejects null for ordinary scalar member",
          "[job_yaml][parser_destination][object][null][invalid]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("scalarValue"));

    REQUIRE_FALSE(destination.null());

    REQUIRE(object.scalarValue == 7);
    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    // A rejected null must not consume the pending key.
    REQUIRE(destination.scalar("9"));
    REQUIRE(destination.endMapping());

    REQUIRE(destination.complete());
    REQUIRE(object.scalarValue == 9);
}

TEST_CASE("YamlObjectParserDestination rejects null for unknown member",
          "[job_yaml][parser_destination][object][null][invalid]")
{
    NullableObjectDestinationFixture object;
    YamlObjectParserDestination destination{object};

    REQUIRE(destination.beginMapping());
    REQUIRE(destination.key("missing"));

    REQUIRE_FALSE(destination.null());

    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    // Failed routing must leave the unresolved key pending.
    REQUIRE_FALSE(destination.key("optionalValue"));
    REQUIRE_FALSE(destination.endMapping());
}

// ========================================
// Sequence null routing
// ========================================

TEST_CASE("YamlObjectParserDestination appends null optional sequence element",
          "[job_yaml][parser_destination][object][sequence][null][optional]")
{
    std::vector<std::optional<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.null());
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(values.size() == 1);
    REQUIRE_FALSE(values[0].has_value());
}

TEST_CASE("YamlObjectParserDestination appends null shared pointer sequence element",
          "[job_yaml][parser_destination][object][sequence][null][pointer][shared]")
{
    std::vector<std::shared_ptr<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.null());
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == nullptr);
}

TEST_CASE("YamlObjectParserDestination appends null unique pointer sequence element",
          "[job_yaml][parser_destination][object][sequence][null][pointer][unique]")
{
    std::vector<std::unique_ptr<int>> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(destination.beginSequence());
    REQUIRE(destination.null());
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(destination.depth() == 0);

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == nullptr);
}

TEST_CASE("YamlObjectParserDestination rejects null ordinary scalar sequence element",
          "[job_yaml][parser_destination][object][sequence][null][invalid]")
{
    std::vector<int> values;
    YamlObjectParserDestination destination{values};

    REQUIRE(destination.beginSequence());

    REQUIRE_FALSE(destination.null());

    REQUIRE(values.empty());
    REQUIRE_FALSE(destination.complete());
    REQUIRE(destination.depth() == 1);

    REQUIRE(destination.scalar("42"));
    REQUIRE(destination.endSequence());

    REQUIRE(destination.complete());
    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == 42);
}


// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlObjectParserDestination root scalar benchmark",
          "[job_yaml][parser_destination][object][benchmark]")
{
    constexpr std::string_view key = "count";
    constexpr std::string_view value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectParserDestination root scalar")
    {
        ObjectDestinationFixture object;
        YamlObjectParserDestination destination{object};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        bool result = destination.beginMapping();
        result = destination.key(key) && result;
        result = destination.scalar(value) && result;
        result = destination.endMapping() && result;

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.count;
    };

    BENCHMARK("YamlObjectReader direct root scalar")
    {
        ObjectDestinationFixture object;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlObjectParserDestination owned scalar benchmark",
          "[job_yaml][parser_destination][object][benchmark]")
{
    const std::string key = "count";
    const std::string value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectParserDestination owned scalar")
    {
        ObjectDestinationFixture object;
        YamlObjectParserDestination destination{object};

        std::string keyCopy = key;
        std::string valueCopy = value;

        benchmarkDoNotOptimize(keyCopy);
        benchmarkDoNotOptimize(valueCopy);

        bool result = destination.beginMapping();
        result = destination.keyOwned(std::move(keyCopy)) && result;
        result = destination.scalarOwned(std::move(valueCopy)) && result;
        result = destination.endMapping() && result;

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlObjectParserDestination nested scalar benchmark",
          "[job_yaml][parser_destination][object][nested][benchmark]")
{
    constexpr std::string_view memberKey = "address";
    constexpr std::string_view scalarKey = "zip";
    constexpr std::string_view value = "12345";

    benchmarkDoNotOptimize(memberKey);
    benchmarkDoNotOptimize(scalarKey);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectParserDestination nested scalar")
    {
        ObjectDestinationFixture object;
        YamlObjectParserDestination destination{object};

        benchmarkDoNotOptimize(memberKey);
        benchmarkDoNotOptimize(scalarKey);
        benchmarkDoNotOptimize(value);

        bool result = destination.beginMapping();
        result = destination.key(memberKey) && result;
        result = destination.beginMapping() && result;
        result = destination.key(scalarKey) && result;
        result = destination.scalar(value) && result;
        result = destination.endMapping() && result;
        result = destination.endMapping() && result;

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.address.zip;
    };

    BENCHMARK("YamlObjectReader direct nested scalar")
    {
        ObjectDestinationFixture object;

        benchmarkDoNotOptimize(scalarKey);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object.address, scalarKey, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.address.zip;
    };
}

TEST_CASE("YamlObjectParserDestination full object benchmark",
          "[job_yaml][parser_destination][object][benchmark]")
{
    BENCHMARK("YamlObjectParserDestination reflected object")
    {
        ObjectDestinationFixture object;
        YamlObjectParserDestination destination{object};

        bool result = destination.beginMapping();

        result = destination.key("name") && result;
        result = destination.scalar("alpha") && result;

        result = destination.key("count") && result;
        result = destination.scalar("42") && result;

        result = destination.key("enabled") && result;
        result = destination.scalar("true") && result;

        result = destination.key("ratio") && result;
        result = destination.scalar("1.5") && result;

        result = destination.key("address") && result;
        result = destination.beginMapping() && result;

        result = destination.key("city") && result;
        result = destination.scalar("Delta") && result;

        result = destination.key("zip") && result;
        result = destination.scalar("12345") && result;

        result = destination.endMapping() && result;

        result = destination.key("profile") && result;
        result = destination.beginMapping() && result;

        result = destination.key("displayName") && result;
        result = destination.scalar("beta") && result;

        result = destination.key("address") && result;
        result = destination.beginMapping() && result;

        result = destination.key("city") && result;
        result = destination.scalar("Delta") && result;

        result = destination.key("zip") && result;
        result = destination.scalar("54321") && result;

        result = destination.endMapping() && result;
        result = destination.endMapping() && result;
        result = destination.endMapping() && result;

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.count +
               object.address.zip +
               object.profile.address.zip +
               static_cast<int>(object.name.size()) +
               static_cast<int>(object.address.city.size()) +
               static_cast<int>(object.profile.displayName.size());
    };
}

TEST_CASE("YamlObjectParserDestination nested traversal benchmark",
          "[job_yaml][parser_destination][object][nested][benchmark]")
{
    BENCHMARK("YamlObjectParserDestination three mapping levels")
    {
        ObjectDestinationFixture object;
        YamlObjectParserDestination destination{object};

        bool result = destination.beginMapping();
        result = destination.key("profile") && result;
        result = destination.beginMapping() && result;
        result = destination.key("address") && result;
        result = destination.beginMapping() && result;
        result = destination.key("zip") && result;
        result = destination.scalar("54321") && result;
        result = destination.endMapping() && result;
        result = destination.endMapping() && result;
        result = destination.endMapping() && result;

        benchmarkClobber(object);
        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return object.profile.address.zip;
    };
}

#endif

} // namespace job::yaml::tests