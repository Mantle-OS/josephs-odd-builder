#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_node.h>
#include <job_yaml_node_sink.h>

#include "test_job_yaml_utils.h"

#include <cstddef>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Concept
// ========================================

static_assert(YamlNodeDestination<YamlNode>);

// ========================================
// Default state
// ========================================

TEST_CASE("YamlNode default constructs as null", "[job_yaml][node]")
{
    const YamlNode node;

    REQUIRE(node.type() == YamlNode::Type::Null);
    REQUIRE(node.isNull());
    REQUIRE_FALSE(node.isScalar());
    REQUIRE_FALSE(node.isMapping());
    REQUIRE_FALSE(node.isSequence());
    REQUIRE(node.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(node.ownsScalar());
    REQUIRE_FALSE(node.borrowsScalar());
}

TEST_CASE("YamlNode default containers are empty", "[job_yaml][node]")
{
    const YamlNode node;

    REQUIRE(node.scalar().empty());
    REQUIRE(node.mapping().empty());
    REQUIRE(node.sequence().empty());
}

// ========================================
// Owning scalar construction
// ========================================

TEST_CASE("YamlNode constructs owning scalar", "[job_yaml][node][owned]")
{
    const YamlNode node{"alpha"};

    REQUIRE(node.type() == YamlNode::Type::Scalar);
    REQUIRE(node.isScalar());
    REQUIRE_FALSE(node.isNull());
    REQUIRE_FALSE(node.isMapping());
    REQUIRE_FALSE(node.isSequence());
    REQUIRE(node.scalarStorage() == YamlNode::ScalarStorage::Owned);
    REQUIRE(node.ownsScalar());
    REQUIRE_FALSE(node.borrowsScalar());
    REQUIRE(node.scalar() == "alpha");
}

TEST_CASE("YamlNode constructs owning empty scalar", "[job_yaml][node][owned]")
{
    const YamlNode node{""};

    REQUIRE(node.isScalar());
    REQUIRE(node.ownsScalar());
    REQUIRE(node.scalar().empty());
}

TEST_CASE("YamlNode scalar constructor respects string_view length",
          "[job_yaml][node][owned]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    const YamlNode node{value};

    REQUIRE(node.isScalar());
    REQUIRE(node.ownsScalar());
    REQUIRE(node.scalar() == "alpha");
    REQUIRE(node.scalar().size() == 5);
}

TEST_CASE("YamlNode scalar constructor copies source view",
          "[job_yaml][node][owned]")
{
    constexpr std::string_view source = "alpha";

    const YamlNode node{source};

    REQUIRE(node.ownsScalar());
    REQUIRE(node.scalar() == source);
    REQUIRE(node.scalar().data() != source.data());
}

TEST_CASE("YamlNode scalar constructor preserves embedded null",
          "[job_yaml][node][owned]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const YamlNode node{
        std::string_view{source, sizeof(source)}
    };

    REQUIRE(node.isScalar());
    REQUIRE(node.ownsScalar());
    REQUIRE(node.scalar().size() == sizeof(source));
    REQUIRE(node.scalar()[0] == 'J');
    REQUIRE(node.scalar()[2] == 'B');
    REQUIRE(node.scalar()[3] == '\0');
    REQUIRE(node.scalar()[4] == 'Y');
    REQUIRE(node.scalar()[7] == 'L');
}

// ========================================
// setScalar owning storage
// ========================================

TEST_CASE("YamlNode setScalar changes null node to owned scalar",
          "[job_yaml][node][owned]")
{
    YamlNode node;

    node.setScalar("alpha");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
}

TEST_CASE("YamlNode setScalar overwrites scalar value",
          "[job_yaml][node][owned]")
{
    YamlNode node{"old"};

    node.setScalar("new");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "new");
}

TEST_CASE("YamlNode setScalar accepts empty value", "[job_yaml][node][owned]")
{
    YamlNode node{"old"};

    node.setScalar("");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode setScalar preserves exact scalar syntax",
          "[job_yaml][node][owned]")
{
    YamlNode node;

    node.setScalar("\"alpha\\n\"");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "\"alpha\\n\"");
}

TEST_CASE("YamlNode setScalar copies source view",
          "[job_yaml][node][owned]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    node.setScalar(value);

    const YamlNode &result = node;

    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() != value.data());
    REQUIRE(result.ownsScalar());
}

// ========================================
// setScalarView borrowed storage
// ========================================

TEST_CASE("YamlNode setScalarView changes null node to borrowed scalar",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    node.setScalarView(source);

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::Borrowed);
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNode setScalarView borrows exact source slice",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    node.setScalarView(value);

    const YamlNode &result = node;

    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data() + 6);
    REQUIRE(result.scalar().data() == value.data());
    REQUIRE(result.scalar().size() == 5);
}

TEST_CASE("YamlNode setScalarView accepts empty view",
          "[job_yaml][node][borrowed]")
{
    YamlNode node;

    node.setScalarView({});

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode setScalarView preserves embedded null",
          "[job_yaml][node][borrowed]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    const std::string_view value{source, sizeof(source)};

    YamlNode node;

    node.setScalarView(value);

    const YamlNode &result = node;

    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().data() == source);
    REQUIRE(result.scalar().size() == sizeof(source));
    REQUIRE(result.scalar()[0] == 'A');
    REQUIRE(result.scalar()[1] == '\0');
    REQUIRE(result.scalar()[2] == 'B');
}

TEST_CASE("YamlNode borrowed scalar reflects source mutation",
          "[job_yaml][node][borrowed]")
{
    char source[] = "alpha";

    YamlNode node;
    node.setScalarView(std::string_view{source, 5});

    const YamlNode &result = node;

    REQUIRE(result.scalar() == "alpha");

    source[0] = 'X';

    REQUIRE(result.scalar() == "Xlpha");
    REQUIRE(result.scalar().data() == source);
}

// ========================================
// Borrowed scalar materialization
// ========================================

TEST_CASE("YamlNode mutable scalar access materializes borrowed scalar",
          "[job_yaml][node][borrowed][materialize]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    const YamlNode &before = node;

    REQUIRE(before.borrowsScalar());
    REQUIRE(before.scalar().data() == source.data());

    std::string &mutableScalar = node.scalar();

    const YamlNode &after = node;

    REQUIRE(after.ownsScalar());
    REQUIRE_FALSE(after.borrowsScalar());
    REQUIRE(after.scalar() == "alpha");
    REQUIRE(after.scalar().data() != source.data());
    REQUIRE(mutableScalar == "alpha");
}

TEST_CASE("YamlNode materialized scalar becomes independent of source",
          "[job_yaml][node][borrowed][materialize]")
{
    char source[] = "alpha";

    YamlNode node;
    node.setScalarView(std::string_view{source, 5});

    node.scalar();

    source[0] = 'X';

    const YamlNode &result = node;

    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "alpha");
}

TEST_CASE("YamlNode mutable scalar can modify materialized borrowed value",
          "[job_yaml][node][borrowed][materialize]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    node.scalar() = "beta";

    const YamlNode &result = node;

    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "beta");
    REQUIRE(result.scalar().data() != source.data());
}

TEST_CASE("YamlNode mutable scalar access on null creates owned scalar",
          "[job_yaml][node][materialize]")
{
    YamlNode node;

    std::string &value = node.scalar();
    value = "alpha";

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "alpha");
}

// ========================================
// Mapping
// ========================================

TEST_CASE("YamlNode setMemberScalar changes null node to mapping",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");
    REQUIRE(node.mapping()[0].value.isScalar());

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
}

TEST_CASE("YamlNode preserves mapping insertion order",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("first", "1");
    node.setMemberScalar("second", "2");
    node.setMemberScalar("third", "3");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 3);

    REQUIRE(node.mapping()[0].key == "first");
    REQUIRE(node.mapping()[0].value.scalar() == "1");

    REQUIRE(node.mapping()[1].key == "second");
    REQUIRE(node.mapping()[1].value.scalar() == "2");

    REQUIRE(node.mapping()[2].key == "third");
    REQUIRE(node.mapping()[2].value.scalar() == "3");
}

TEST_CASE("YamlNode preserves duplicate mapping keys",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("name", "first");
    node.setMemberScalar("name", "second");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 2);

    REQUIRE(node.mapping()[0].key == "name");
    REQUIRE(node.mapping()[0].value.scalar() == "first");

    REQUIRE(node.mapping()[1].key == "name");
    REQUIRE(node.mapping()[1].value.scalar() == "second");
}

TEST_CASE("YamlNode accepts empty mapping key", "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("", "value");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key.empty());
    REQUIRE(node.mapping()[0].value.scalar() == "value");
}

TEST_CASE("YamlNode accepts empty mapping scalar value",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("name", "");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].value.isScalar());
    REQUIRE(node.mapping()[0].value.scalar().empty());
}

TEST_CASE("YamlNode mapping copies key string_view",
          "[job_yaml][node][owned]")
{
    char source[] = "name";
    const std::string_view key{source, 4};

    YamlNode node;

    node.setMemberScalar(key, "alpha");

    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");

    source[0] = 'X';

    REQUIRE(node.mapping()[0].key == "name");
}

TEST_CASE("YamlNode mapping respects key string_view length",
          "[job_yaml][node]")
{
    constexpr char source[] = "name-extra";
    const std::string_view key{source, 4};

    YamlNode node;

    node.setMemberScalar(key, "alpha");

    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");
}

TEST_CASE("YamlNode mapping owning value respects string_view length",
          "[job_yaml][node][owned]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    YamlNode node;

    node.setMemberScalar("name", value);

    REQUIRE(node.mapping().size() == 1);

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.ownsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() != value.data());
}

TEST_CASE("YamlNode mapping preserves embedded null owning scalar value",
          "[job_yaml][node][owned]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    YamlNode node;

    node.setMemberScalar(
        "value",
        std::string_view{source, sizeof(source)});

    REQUIRE(node.mapping().size() == 1);

    const YamlNode &scalarNode = node.mapping()[0].value;
    const std::string_view value = scalarNode.scalar();

    REQUIRE(scalarNode.ownsScalar());
    REQUIRE(value.size() == sizeof(source));
    REQUIRE(value[0] == 'A');
    REQUIRE(value[1] == '\0');
    REQUIRE(value[2] == 'B');
}

// ========================================
// Borrowed mapping scalars
// ========================================

TEST_CASE("YamlNode setMemberScalarView stores borrowed scalar",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    node.setMemberScalarView("name", source);

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() == source.data());
}

TEST_CASE("YamlNode setMemberScalarView borrows exact source slice",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    node.setMemberScalarView("name", value);

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

TEST_CASE("YamlNode setMemberScalarView keeps mapping key owned",
          "[job_yaml][node][borrowed]")
{
    char keySource[] = "name";
    constexpr std::string_view value = "alpha";

    YamlNode node;

    node.setMemberScalarView(std::string_view{keySource, 4}, value);

    keySource[0] = 'X';

    REQUIRE(node.mapping()[0].key == "name");

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == value.data());
}

// ========================================
// Sequence
// ========================================

TEST_CASE("YamlNode appendScalar changes null node to sequence",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("alpha");

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);
    REQUIRE(node.sequence()[0].isScalar());

    const YamlNode &value = node.sequence()[0];

    REQUIRE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
}

TEST_CASE("YamlNode appendScalar preserves sequence order",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("first");
    node.appendScalar("second");
    node.appendScalar("third");

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 3);

    REQUIRE(node.sequence()[0].scalar() == "first");
    REQUIRE(node.sequence()[1].scalar() == "second");
    REQUIRE(node.sequence()[2].scalar() == "third");
}

TEST_CASE("YamlNode appendScalar accepts empty scalar",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("");

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);
    REQUIRE(node.sequence()[0].isScalar());
    REQUIRE(node.sequence()[0].scalar().empty());
}

TEST_CASE("YamlNode appendScalar respects string_view length",
          "[job_yaml][node]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    YamlNode node;

    node.appendScalar(value);

    REQUIRE(node.sequence().size() == 1);

    const YamlNode &scalar = node.sequence()[0];

    REQUIRE(scalar.ownsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() != value.data());
}

TEST_CASE("YamlNode appendScalar preserves embedded null",
          "[job_yaml][node]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    YamlNode node;

    node.appendScalar(
        std::string_view{source, sizeof(source)});

    REQUIRE(node.sequence().size() == 1);

    const YamlNode &scalarNode = node.sequence()[0];
    const std::string_view value = scalarNode.scalar();

    REQUIRE(scalarNode.ownsScalar());
    REQUIRE(value.size() == sizeof(source));
    REQUIRE(value[0] == 'A');
    REQUIRE(value[1] == '\0');
    REQUIRE(value[2] == 'B');
}

// ========================================
// Borrowed sequence scalars
// ========================================

TEST_CASE("YamlNode appendScalarView stores borrowed scalar",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    node.appendScalarView(source);

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);

    const YamlNode &value = node.sequence()[0];

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() == source.data());
}

TEST_CASE("YamlNode appendScalarView borrows exact source slice",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    node.appendScalarView(value);

    const YamlNode &scalar = node.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

// ========================================
// Copy and move storage semantics
// ========================================

TEST_CASE("YamlNode copy of owned scalar remains owned",
          "[job_yaml][node][owned][copy]")
{
    const YamlNode source{"alpha"};
    const YamlNode copy{source};

    REQUIRE(source.ownsScalar());
    REQUIRE(copy.ownsScalar());
    REQUIRE(copy.scalar() == "alpha");
    REQUIRE(copy.scalar().data() != source.scalar().data());
}

TEST_CASE("YamlNode copy of borrowed scalar remains borrowed",
          "[job_yaml][node][borrowed][copy]")
{
    constexpr std::string_view source = "alpha";

    YamlNode original;
    original.setScalarView(source);

    const YamlNode copy{original};

    REQUIRE(copy.borrowsScalar());
    REQUIRE(copy.scalar() == "alpha");
    REQUIRE(copy.scalar().data() == source.data());
}

TEST_CASE("YamlNode move of borrowed scalar preserves source view",
          "[job_yaml][node][borrowed][move]")
{
    constexpr std::string_view source = "alpha";

    YamlNode original;
    original.setScalarView(source);

    const YamlNode moved{std::move(original)};

    REQUIRE(moved.borrowsScalar());
    REQUIRE(moved.scalar() == "alpha");
    REQUIRE(moved.scalar().data() == source.data());
}

// ========================================
// Type transitions
// ========================================

TEST_CASE("YamlNode owned scalar to mapping transition clears scalar state",
          "[job_yaml][node][transition]")
{
    YamlNode node{"old"};

    node.setMemberScalar("name", "alpha");

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.mapping().size() == 1);
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode borrowed scalar to mapping transition clears borrowed state",
          "[job_yaml][node][transition][borrowed]")
{
    constexpr std::string_view source = "old";

    YamlNode node;
    node.setScalarView(source);

    node.setMemberScalar("name", "alpha");

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode mapping to sequence transition clears mapping state",
          "[job_yaml][node][transition]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);

    node.appendScalar("item");

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().size() == 1);
    REQUIRE(result.sequence()[0].scalar() == "item");
}

TEST_CASE("YamlNode sequence to scalar transition clears sequence state",
          "[job_yaml][node][transition]")
{
    YamlNode node;

    node.appendScalar("one");
    node.appendScalar("two");

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);

    node.setScalar("replacement");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "replacement");
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode sequence to borrowed scalar transition clears sequence state",
          "[job_yaml][node][transition][borrowed]")
{
    constexpr std::string_view source = "replacement";

    YamlNode node;

    node.appendScalar("one");
    node.appendScalar("two");

    node.setScalarView(source);

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "replacement");
    REQUIRE(result.scalar().data() == source.data());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode sequence to mapping transition clears sequence state",
          "[job_yaml][node][transition]")
{
    YamlNode node;

    node.appendScalar("one");
    node.appendScalar("two");

    node.setMemberScalar("name", "alpha");

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.sequence().empty());
    REQUIRE(result.mapping().size() == 1);
    REQUIRE(result.mapping()[0].key == "name");
}

TEST_CASE("YamlNode mapping to scalar transition clears mapping state",
          "[job_yaml][node][transition]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");
    node.setMemberScalar("count", "42");

    node.setScalar("replacement");

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "replacement");
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

// ========================================
// clear
// ========================================

TEST_CASE("YamlNode clear resets owned scalar node to null",
          "[job_yaml][node]")
{
    YamlNode node{"alpha"};

    node.clear();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode clear resets borrowed scalar node to null",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    node.clear();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode clear resets mapping node to null",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");
    node.clear();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode clear resets sequence node to null",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("one");
    node.appendScalar("two");
    node.clear();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

// ========================================
// Mutable accessors
// ========================================

TEST_CASE("YamlNode exposes mutable owned scalar storage",
          "[job_yaml][node]")
{
    YamlNode node{"old"};

    node.scalar() = "new";

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "new");
}

TEST_CASE("YamlNode exposes mutable mapping storage",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");

    node.mapping()[0].key = "renamed";
    node.mapping()[0].value.setScalar("JOB");

    REQUIRE(node.mapping()[0].key == "renamed");

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(value.scalar() == "JOB");
}

TEST_CASE("YamlNode exposes mutable sequence storage",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("old");

    node.sequence()[0].setScalar("new");

    const YamlNode &value = node.sequence()[0];

    REQUIRE(value.scalar() == "new");
}

// ========================================
// Const accessors
// ========================================

TEST_CASE("YamlNode exposes const scalar view",
          "[job_yaml][node]")
{
    const YamlNode node{"alpha"};

    const std::string_view scalar = node.scalar();

    REQUIRE(scalar == "alpha");
    REQUIRE(node.ownsScalar());
}

TEST_CASE("YamlNode const borrowed scalar access does not materialize",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode mutableNode;
    mutableNode.setScalarView(source);

    const YamlNode &node = mutableNode;
    const std::string_view scalar = node.scalar();

    REQUIRE(node.borrowsScalar());
    REQUIRE_FALSE(node.ownsScalar());
    REQUIRE(scalar == "alpha");
    REQUIRE(scalar.data() == source.data());
}

TEST_CASE("YamlNode exposes const mapping storage",
          "[job_yaml][node]")
{
    YamlNode mutableNode;
    mutableNode.setMemberScalar("name", "alpha");

    const YamlNode &node = mutableNode;
    const YamlNode::Mapping &mapping = node.mapping();

    REQUIRE(mapping.size() == 1);
    REQUIRE(mapping[0].key == "name");
    REQUIRE(mapping[0].value.scalar() == "alpha");
}

TEST_CASE("YamlNode exposes const sequence storage",
          "[job_yaml][node]")
{
    YamlNode mutableNode;
    mutableNode.appendScalar("alpha");

    const YamlNode &node = mutableNode;
    const YamlNode::Sequence &sequence = node.sequence();

    REQUIRE(sequence.size() == 1);
    REQUIRE(sequence[0].scalar() == "alpha");
}

// ========================================
// UTF-8 preservation
// ========================================

TEST_CASE("YamlNode preserves UTF-8 owning scalar bytes",
          "[job_yaml][node]")
{
    constexpr std::string_view value = "alpha € 😀";

    const YamlNode node{value};

    REQUIRE(node.ownsScalar());
    REQUIRE(node.scalar() == value);
}

TEST_CASE("YamlNode preserves UTF-8 borrowed scalar bytes",
          "[job_yaml][node][borrowed]")
{
    constexpr std::string_view value = "alpha € 😀";

    YamlNode mutableNode;
    mutableNode.setScalarView(value);

    const YamlNode &node = mutableNode;

    REQUIRE(node.borrowsScalar());
    REQUIRE(node.scalar() == value);
    REQUIRE(node.scalar().data() == value.data());
}

TEST_CASE("YamlNode preserves UTF-8 mapping values",
          "[job_yaml][node]")
{
    YamlNode node;

    node.setMemberScalar("message", "Hello € 😀");

    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].value.scalar() == "Hello € 😀");
}

TEST_CASE("YamlNode preserves UTF-8 sequence values",
          "[job_yaml][node]")
{
    YamlNode node;

    node.appendScalar("€");
    node.appendScalar("😀");

    REQUIRE(node.sequence().size() == 2);
    REQUIRE(node.sequence()[0].scalar() == "€");
    REQUIRE(node.sequence()[1].scalar() == "😀");
}

// ========================================
// YamlNodeSink owning integration
// ========================================

TEST_CASE("YamlNodeSink writes owning scalar into YamlNode",
          "[job_yaml][node][node_sink][owned]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    REQUIRE(YamlNodeSink::scalar(node, source));

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.ownsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() != source.data());
}

TEST_CASE("YamlNodeSink writes owning mapping member into YamlNode",
          "[job_yaml][node][node_sink][owned]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    REQUIRE(YamlNodeSink::member(node, "name", source));

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");

    const YamlNode &value = node.mapping()[0].value;

    REQUIRE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() != source.data());
}

TEST_CASE("YamlNodeSink appends owning sequence scalar into YamlNode",
          "[job_yaml][node][node_sink][owned]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    REQUIRE(YamlNodeSink::append(node, source));

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);

    const YamlNode &value = node.sequence()[0];

    REQUIRE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() != source.data());
}

// ========================================
// YamlNodeSink borrowed integration
// ========================================

TEST_CASE("YamlNodeSink writes borrowed scalar view into YamlNode",
          "[job_yaml][node][node_sink][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;

    REQUIRE(YamlNodeSink::scalarView(node, source));

    const YamlNode &result = node;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink writes borrowed mapping scalar view into YamlNode",
          "[job_yaml][node][node_sink][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    REQUIRE(YamlNodeSink::memberView(node, "name", value));

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");

    const YamlNode &scalar = node.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
}

TEST_CASE("YamlNodeSink appends borrowed sequence scalar view into YamlNode",
          "[job_yaml][node][node_sink][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode node;

    REQUIRE(YamlNodeSink::appendView(node, value));

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);

    const YamlNode &scalar = node.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
}

TEST_CASE("YamlNodeSink preserves repeated mapping events in YamlNode",
          "[job_yaml][node][node_sink]")
{
    YamlNode node;

    REQUIRE(YamlNodeSink::member(node, "first", "1"));
    REQUIRE(YamlNodeSink::member(node, "second", "2"));

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 2);
    REQUIRE(node.mapping()[0].key == "first");
    REQUIRE(node.mapping()[1].key == "second");
}

TEST_CASE("YamlNodeSink preserves repeated sequence events in YamlNode",
          "[job_yaml][node][node_sink]")
{
    YamlNode node;

    REQUIRE(YamlNodeSink::append(node, "first"));
    REQUIRE(YamlNodeSink::append(node, "second"));

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 2);
    REQUIRE(node.sequence()[0].scalar() == "first");
    REQUIRE(node.sequence()[1].scalar() == "second");
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlNode scalar benchmark", "[job_yaml][node][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode setScalar owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        node.setScalar(value);

        const YamlNode &result = node;

        benchmarkClobber(node);

        return result.scalar().size();
    };

    BENCHMARK("YamlNode setScalarView borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        node.setScalarView(value);

        const YamlNode &result = node;

        benchmarkClobber(node);

        return result.scalar().size();
    };

    BENCHMARK("direct string assign")
    {
        std::string destination;

        benchmarkDoNotOptimize(value);

        destination.assign(value);

        benchmarkClobber(destination);

        return destination.size();
    };

    BENCHMARK("direct string_view assign")
    {
        std::string_view destination;

        benchmarkDoNotOptimize(value);

        destination = value;

        benchmarkDoNotOptimize(destination);

        return destination.size();
    };

    BENCHMARK("YamlNodeSink scalar owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::scalar(node, value);
        const YamlNode &parsed = node;

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return parsed.scalar().size();
    };

    BENCHMARK("YamlNodeSink scalar borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::scalarView(node, value);
        const YamlNode &parsed = node;

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return parsed.scalar().size();
    };
}

TEST_CASE("YamlNode mapping benchmark", "[job_yaml][node][benchmark]")
{
    std::string_view key = "name";
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode setMemberScalar owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        node.setMemberScalar(key, value);

        benchmarkClobber(node);

        return node.mapping().size();
    };

    BENCHMARK("YamlNode setMemberScalarView borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        node.setMemberScalarView(key, value);

        benchmarkClobber(node);

        return node.mapping().size();
    };

    BENCHMARK("manual MappingEntry push_back owning")
    {
        YamlNode::Mapping mapping;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        mapping.push_back(YamlNode::MappingEntry{
            .key = std::string{key},
            .value = YamlNode{value}
        });

        benchmarkClobber(mapping);

        return mapping.size();
    };

    BENCHMARK("YamlNodeSink member owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::member(node, key, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.mapping().size();
    };

    BENCHMARK("YamlNodeSink member borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::memberView(node, key, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.mapping().size();
    };
}

TEST_CASE("YamlNode sequence benchmark", "[job_yaml][node][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode appendScalar owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        node.appendScalar(value);

        benchmarkClobber(node);

        return node.sequence().size();
    };

    BENCHMARK("YamlNode appendScalarView borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        node.appendScalarView(value);

        benchmarkClobber(node);

        return node.sequence().size();
    };

    BENCHMARK("manual Sequence emplace_back owning")
    {
        YamlNode::Sequence sequence;

        benchmarkDoNotOptimize(value);

        sequence.emplace_back(value);

        benchmarkClobber(sequence);

        return sequence.size();
    };

    BENCHMARK("YamlNodeSink append owning")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::append(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.sequence().size();
    };

    BENCHMARK("YamlNodeSink append borrowed")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::appendView(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.sequence().size();
    };
}

#endif

} // namespace job::yaml::tests