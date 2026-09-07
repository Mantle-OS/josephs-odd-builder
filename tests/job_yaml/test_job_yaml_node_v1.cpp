#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_node.h>

#include "test_job_yaml_utils.h"

#include <string_view>
#include <utility>

namespace job::yaml::tests {

// ========================================
// Explicit structural state
// ========================================

TEST_CASE("YamlNode setNull resets node to null",
          "[job_yaml][node][v1]")
{
    YamlNode node{"alpha"};

    node.setNull();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE_FALSE(result.isScalar());
    REQUIRE_FALSE(result.isMapping());
    REQUIRE_FALSE(result.isSequence());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode setNull clears borrowed scalar state",
          "[job_yaml][node][v1][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    REQUIRE(node.borrowsScalar());

    node.setNull();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode setMapping creates empty mapping",
          "[job_yaml][node][v1]")
{
    YamlNode node;

    node.setMapping();

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.sequence().empty());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
}

TEST_CASE("YamlNode setSequence creates empty sequence",
          "[job_yaml][node][v1]")
{
    YamlNode node;

    node.setSequence();

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.sequence().empty());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
}

// ========================================
// Explicit structural transitions
// ========================================

TEST_CASE("YamlNode setMapping clears owned scalar state",
          "[job_yaml][node][v1][transition]")
{
    YamlNode node{"alpha"};

    node.setMapping();

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode setMapping clears borrowed scalar state",
          "[job_yaml][node][v1][transition][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    node.setMapping();

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
}

TEST_CASE("YamlNode setSequence clears mapping state",
          "[job_yaml][node][v1][transition]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);

    node.setSequence();

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

TEST_CASE("YamlNode setSequence clears borrowed scalar state",
          "[job_yaml][node][v1][transition][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode node;
    node.setScalarView(source);

    node.setSequence();

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNode setNull clears sequence state",
          "[job_yaml][node][v1][transition]")
{
    YamlNode node;

    node.appendScalar("one");
    node.appendScalar("two");

    node.setNull();

    const YamlNode &result = node;

    REQUIRE(result.isNull());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().empty());
}

// ========================================
// Structural mapping members
// ========================================

TEST_CASE("YamlNode setMember inserts owning scalar node",
          "[job_yaml][node][v1]")
{
    YamlNode root;
    YamlNode value{"alpha"};

    root.setMember("name", std::move(value));

    REQUIRE(root.isMapping());
    REQUIRE(root.mapping().size() == 1);
    REQUIRE(root.mapping()[0].key == "name");
    REQUIRE(root.mapping()[0].value.isScalar());

    const YamlNode &scalar = root.mapping()[0].value;

    REQUIRE(scalar.ownsScalar());
    REQUIRE(scalar.scalar() == "alpha");
}

TEST_CASE("YamlNode setMember inserts borrowed scalar node without materializing",
          "[job_yaml][node][v1][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode value;
    value.setScalarView(source);

    YamlNode root;
    root.setMember("name", std::move(value));

    REQUIRE(root.isMapping());
    REQUIRE(root.mapping().size() == 1);

    const YamlNode &scalar = root.mapping()[0].value;

    REQUIRE(scalar.isScalar());
    REQUIRE(scalar.borrowsScalar());
    REQUIRE_FALSE(scalar.ownsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data());
}

TEST_CASE("YamlNode setMember inserts null node",
          "[job_yaml][node][v1]")
{
    YamlNode root;
    YamlNode value;

    root.setMember("value", std::move(value));

    REQUIRE(root.isMapping());
    REQUIRE(root.mapping().size() == 1);
    REQUIRE(root.mapping()[0].key == "value");
    REQUIRE(root.mapping()[0].value.isNull());
}

TEST_CASE("YamlNode setMember preserves duplicate keys",
          "[job_yaml][node][v1]")
{
    YamlNode root;

    root.setMember("name", YamlNode{"first"});
    root.setMember("name", YamlNode{"second"});

    REQUIRE(root.mapping().size() == 2);
    REQUIRE(root.mapping()[0].key == "name");
    REQUIRE(root.mapping()[0].value.scalar() == "first");
    REQUIRE(root.mapping()[1].key == "name");
    REQUIRE(root.mapping()[1].value.scalar() == "second");
}

TEST_CASE("YamlNode setMember preserves mapping insertion order",
          "[job_yaml][node][v1]")
{
    YamlNode root;

    root.setMember("first", YamlNode{"1"});
    root.setMember("second", YamlNode{"2"});
    root.setMember("third", YamlNode{"3"});

    REQUIRE(root.mapping().size() == 3);
    REQUIRE(root.mapping()[0].key == "first");
    REQUIRE(root.mapping()[1].key == "second");
    REQUIRE(root.mapping()[2].key == "third");
}

TEST_CASE("YamlNode setMember respects key string_view length",
          "[job_yaml][node][v1]")
{
    constexpr char source[] = "name-extra";
    const std::string_view key{source, 4};

    YamlNode root;

    root.setMember(key, YamlNode{"alpha"});

    REQUIRE(root.mapping().size() == 1);
    REQUIRE(root.mapping()[0].key == "name");
}

TEST_CASE("YamlNode setMember copies key string_view",
          "[job_yaml][node][v1]")
{
    char source[] = "name";
    const std::string_view key{source, 4};

    YamlNode root;

    root.setMember(key, YamlNode{"alpha"});

    source[0] = 'X';

    REQUIRE(root.mapping()[0].key == "name");
}

// ========================================
// Nested mappings
// ========================================

TEST_CASE("YamlNode mapping can contain mapping",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode child;
    child.setMapping();
    child.setMemberScalar("name", "alpha");
    child.setMemberScalar("count", "42");

    YamlNode root;
    root.setMapping();
    root.setMember("child", std::move(child));

    REQUIRE(root.isMapping());
    REQUIRE(root.mapping().size() == 1);

    const auto &nested = root.mapping()[0].value;

    REQUIRE(root.mapping()[0].key == "child");
    REQUIRE(nested.isMapping());
    REQUIRE(nested.mapping().size() == 2);
    REQUIRE(nested.mapping()[0].key == "name");
    REQUIRE(nested.mapping()[0].value.scalar() == "alpha");
    REQUIRE(nested.mapping()[1].key == "count");
    REQUIRE(nested.mapping()[1].value.scalar() == "42");
}

TEST_CASE("YamlNode nested mapping preserves borrowed scalar",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode child;
    child.setMemberScalarView("name", value);

    YamlNode root;
    root.setMember("child", std::move(child));

    const YamlNode &nested = root.mapping()[0].value;
    const YamlNode &scalar = nested.mapping()[0].value;

    REQUIRE(nested.isMapping());
    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
}

TEST_CASE("YamlNode supports deeply nested mappings",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode leaf;
    leaf.setMemberScalar("value", "alpha");

    YamlNode level2;
    level2.setMember("level3", std::move(leaf));

    YamlNode level1;
    level1.setMember("level2", std::move(level2));

    YamlNode root;
    root.setMember("level1", std::move(level1));

    const auto &node1 = root.mapping()[0].value;
    const auto &node2 = node1.mapping()[0].value;
    const auto &node3 = node2.mapping()[0].value;

    REQUIRE(root.mapping()[0].key == "level1");
    REQUIRE(node1.isMapping());
    REQUIRE(node1.mapping()[0].key == "level2");
    REQUIRE(node2.isMapping());
    REQUIRE(node2.mapping()[0].key == "level3");
    REQUIRE(node3.isMapping());
    REQUIRE(node3.mapping()[0].key == "value");
    REQUIRE(node3.mapping()[0].value.scalar() == "alpha");
}

TEST_CASE("YamlNode deeply nested borrowed scalar retains source pointer",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode leaf;
    leaf.setMemberScalarView("value", source);

    YamlNode level2;
    level2.setMember("level3", std::move(leaf));

    YamlNode level1;
    level1.setMember("level2", std::move(level2));

    YamlNode root;
    root.setMember("level1", std::move(level1));

    const YamlNode &node1 = root.mapping()[0].value;
    const YamlNode &node2 = node1.mapping()[0].value;
    const YamlNode &node3 = node2.mapping()[0].value;
    const YamlNode &scalar = node3.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data());
}

// ========================================
// Structural sequence append
// ========================================

TEST_CASE("YamlNode append inserts owning scalar node",
          "[job_yaml][node][v1]")
{
    YamlNode sequence;

    sequence.append(YamlNode{"alpha"});

    REQUIRE(sequence.isSequence());
    REQUIRE(sequence.sequence().size() == 1);
    REQUIRE(sequence.sequence()[0].isScalar());

    const YamlNode &scalar = sequence.sequence()[0];

    REQUIRE(scalar.ownsScalar());
    REQUIRE(scalar.scalar() == "alpha");
}

TEST_CASE("YamlNode append inserts borrowed scalar node without materializing",
          "[job_yaml][node][v1][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode value;
    value.setScalarView(source);

    YamlNode sequence;
    sequence.append(std::move(value));

    REQUIRE(sequence.isSequence());
    REQUIRE(sequence.sequence().size() == 1);

    const YamlNode &scalar = sequence.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data());
}

TEST_CASE("YamlNode append inserts null node",
          "[job_yaml][node][v1]")
{
    YamlNode sequence;

    sequence.append(YamlNode{});

    REQUIRE(sequence.isSequence());
    REQUIRE(sequence.sequence().size() == 1);
    REQUIRE(sequence.sequence()[0].isNull());
}

TEST_CASE("YamlNode append preserves sequence order",
          "[job_yaml][node][v1]")
{
    YamlNode sequence;

    sequence.append(YamlNode{"first"});
    sequence.append(YamlNode{"second"});
    sequence.append(YamlNode{"third"});

    REQUIRE(sequence.sequence().size() == 3);
    REQUIRE(sequence.sequence()[0].scalar() == "first");
    REQUIRE(sequence.sequence()[1].scalar() == "second");
    REQUIRE(sequence.sequence()[2].scalar() == "third");
}

// ========================================
// Nested sequences
// ========================================

TEST_CASE("YamlNode sequence can contain sequence",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode child;
    child.setSequence();
    child.appendScalar("one");
    child.appendScalar("two");

    YamlNode root;
    root.setSequence();
    root.append(std::move(child));

    REQUIRE(root.sequence().size() == 1);

    const auto &nested = root.sequence()[0];

    REQUIRE(nested.isSequence());
    REQUIRE(nested.sequence().size() == 2);
    REQUIRE(nested.sequence()[0].scalar() == "one");
    REQUIRE(nested.sequence()[1].scalar() == "two");
}

TEST_CASE("YamlNode nested sequence preserves borrowed scalars",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view second = "two";

    YamlNode child;
    child.setSequence();
    child.appendScalarView(first);
    child.appendScalarView(second);

    YamlNode root;
    root.append(std::move(child));

    const YamlNode &nested = root.sequence()[0];

    REQUIRE(nested.isSequence());
    REQUIRE(nested.sequence().size() == 2);

    const YamlNode &firstValue = nested.sequence()[0];
    const YamlNode &secondValue = nested.sequence()[1];

    REQUIRE(firstValue.borrowsScalar());
    REQUIRE(firstValue.scalar().data() == first.data());

    REQUIRE(secondValue.borrowsScalar());
    REQUIRE(secondValue.scalar().data() == second.data());
}

TEST_CASE("YamlNode supports sequence of sequences",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode first;
    first.setSequence();
    first.appendScalar("key1");
    first.appendScalar("value1");

    YamlNode second;
    second.setSequence();
    second.appendScalar("key2");
    second.appendScalar("value2");

    YamlNode root;
    root.setSequence();
    root.append(std::move(first));
    root.append(std::move(second));

    REQUIRE(root.sequence().size() == 2);

    REQUIRE(root.sequence()[0].isSequence());
    REQUIRE(root.sequence()[0].sequence().size() == 2);
    REQUIRE(root.sequence()[0].sequence()[0].scalar() == "key1");
    REQUIRE(root.sequence()[0].sequence()[1].scalar() == "value1");

    REQUIRE(root.sequence()[1].isSequence());
    REQUIRE(root.sequence()[1].sequence().size() == 2);
    REQUIRE(root.sequence()[1].sequence()[0].scalar() == "key2");
    REQUIRE(root.sequence()[1].sequence()[1].scalar() == "value2");
}

// ========================================
// Mixed recursive structure
// ========================================

TEST_CASE("YamlNode mapping can contain sequence",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode values;
    values.setSequence();
    values.appendScalar("one");
    values.appendScalar("two");
    values.appendScalar("three");

    YamlNode root;
    root.setMember("values", std::move(values));

    REQUIRE(root.isMapping());
    REQUIRE(root.mapping().size() == 1);

    const auto &sequence = root.mapping()[0].value;

    REQUIRE(sequence.isSequence());
    REQUIRE(sequence.sequence().size() == 3);
    REQUIRE(sequence.sequence()[0].scalar() == "one");
    REQUIRE(sequence.sequence()[1].scalar() == "two");
    REQUIRE(sequence.sequence()[2].scalar() == "three");
}

TEST_CASE("YamlNode mapping can contain sequence of borrowed scalars",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view first = "one";
    constexpr std::string_view second = "two";

    YamlNode values;
    values.appendScalarView(first);
    values.appendScalarView(second);

    YamlNode root;
    root.setMember("values", std::move(values));

    const YamlNode &sequence = root.mapping()[0].value;

    REQUIRE(sequence.isSequence());
    REQUIRE(sequence.sequence().size() == 2);

    REQUIRE(sequence.sequence()[0].borrowsScalar());
    REQUIRE(sequence.sequence()[0].scalar().data() == first.data());

    REQUIRE(sequence.sequence()[1].borrowsScalar());
    REQUIRE(sequence.sequence()[1].scalar().data() == second.data());
}

TEST_CASE("YamlNode sequence can contain mapping",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode object;
    object.setMapping();
    object.setMemberScalar("name", "alpha");
    object.setMemberScalar("count", "42");

    YamlNode root;
    root.setSequence();
    root.append(std::move(object));

    REQUIRE(root.isSequence());
    REQUIRE(root.sequence().size() == 1);

    const auto &mapping = root.sequence()[0];

    REQUIRE(mapping.isMapping());
    REQUIRE(mapping.mapping().size() == 2);
    REQUIRE(mapping.mapping()[0].key == "name");
    REQUIRE(mapping.mapping()[0].value.scalar() == "alpha");
    REQUIRE(mapping.mapping()[1].key == "count");
    REQUIRE(mapping.mapping()[1].value.scalar() == "42");
}

TEST_CASE("YamlNode sequence can contain mapping with borrowed scalar",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode object;
    object.setMemberScalarView("name", source);

    YamlNode root;
    root.append(std::move(object));

    const YamlNode &mapping = root.sequence()[0];
    const YamlNode &scalar = mapping.mapping()[0].value;

    REQUIRE(mapping.isMapping());
    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data());
}

TEST_CASE("YamlNode supports mixed recursive document structure",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode firstObject;
    firstObject.setMemberScalar("name", "first");
    firstObject.setMemberScalar("value", "1");

    YamlNode secondObject;
    secondObject.setMemberScalar("name", "second");
    secondObject.setMemberScalar("value", "2");

    YamlNode objects;
    objects.setSequence();
    objects.append(std::move(firstObject));
    objects.append(std::move(secondObject));

    YamlNode metadata;
    metadata.setMemberScalar("author", "alpha");
    metadata.setMemberScalar("format", "JOB YAML");

    YamlNode root;
    root.setMapping();
    root.setMember("metadata", std::move(metadata));
    root.setMember("objects", std::move(objects));

    REQUIRE(root.mapping().size() == 2);

    REQUIRE(root.mapping()[0].key == "metadata");
    REQUIRE(root.mapping()[0].value.isMapping());
    REQUIRE(root.mapping()[0].value.mapping().size() == 2);

    REQUIRE(root.mapping()[1].key == "objects");
    REQUIRE(root.mapping()[1].value.isSequence());
    REQUIRE(root.mapping()[1].value.sequence().size() == 2);

    REQUIRE(root.mapping()[1].value.sequence()[0].isMapping());
    REQUIRE(root.mapping()[1].value.sequence()[0].mapping()[0].value.scalar() == "first");

    REQUIRE(root.mapping()[1].value.sequence()[1].isMapping());
    REQUIRE(root.mapping()[1].value.sequence()[1].mapping()[0].value.scalar() == "second");
}

// ========================================
// Existing scalar helpers use structural path
// ========================================

TEST_CASE("YamlNode setMemberScalar produces equivalent owning structural member",
          "[job_yaml][node][v1]")
{
    YamlNode scalarHelper;
    YamlNode structural;

    scalarHelper.setMemberScalar("name", "alpha");
    structural.setMember("name", YamlNode{"alpha"});

    REQUIRE(scalarHelper.type() == structural.type());
    REQUIRE(scalarHelper.mapping().size() == structural.mapping().size());
    REQUIRE(scalarHelper.mapping()[0].key == structural.mapping()[0].key);
    REQUIRE(scalarHelper.mapping()[0].value.scalar() ==
            structural.mapping()[0].value.scalar());
    REQUIRE(scalarHelper.mapping()[0].value.ownsScalar());
    REQUIRE(structural.mapping()[0].value.ownsScalar());
}

TEST_CASE("YamlNode setMemberScalarView produces equivalent borrowed structural member",
          "[job_yaml][node][v1][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode scalarHelper;
    YamlNode structural;

    scalarHelper.setMemberScalarView("name", source);

    YamlNode value;
    value.setScalarView(source);
    structural.setMember("name", std::move(value));

    const YamlNode &helperValue = scalarHelper.mapping()[0].value;
    const YamlNode &structuralValue = structural.mapping()[0].value;

    REQUIRE(helperValue.scalar() == structuralValue.scalar());
    REQUIRE(helperValue.borrowsScalar());
    REQUIRE(structuralValue.borrowsScalar());
    REQUIRE(helperValue.scalar().data() == source.data());
    REQUIRE(structuralValue.scalar().data() == source.data());
}

TEST_CASE("YamlNode appendScalar produces equivalent owning structural item",
          "[job_yaml][node][v1]")
{
    YamlNode scalarHelper;
    YamlNode structural;

    scalarHelper.appendScalar("alpha");
    structural.append(YamlNode{"alpha"});

    REQUIRE(scalarHelper.type() == structural.type());
    REQUIRE(scalarHelper.sequence().size() == structural.sequence().size());
    REQUIRE(scalarHelper.sequence()[0].scalar() ==
            structural.sequence()[0].scalar());
    REQUIRE(scalarHelper.sequence()[0].ownsScalar());
    REQUIRE(structural.sequence()[0].ownsScalar());
}

TEST_CASE("YamlNode appendScalarView produces equivalent borrowed structural item",
          "[job_yaml][node][v1][borrowed]")
{
    constexpr std::string_view source = "alpha";

    YamlNode scalarHelper;
    YamlNode structural;

    scalarHelper.appendScalarView(source);

    YamlNode value;
    value.setScalarView(source);
    structural.append(std::move(value));

    const YamlNode &helperValue = scalarHelper.sequence()[0];
    const YamlNode &structuralValue = structural.sequence()[0];

    REQUIRE(helperValue.scalar() == structuralValue.scalar());
    REQUIRE(helperValue.borrowsScalar());
    REQUIRE(structuralValue.borrowsScalar());
    REQUIRE(helperValue.scalar().data() == source.data());
    REQUIRE(structuralValue.scalar().data() == source.data());
}

// ========================================
// Structural type replacement
// ========================================

TEST_CASE("YamlNode setMember replaces previous owned scalar kind with mapping",
          "[job_yaml][node][v1][transition]")
{
    YamlNode node{"old"};

    node.setMember("child", YamlNode{"new"});

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE(result.scalar().empty());
    REQUIRE(result.sequence().empty());
    REQUIRE(result.mapping().size() == 1);
    REQUIRE(result.mapping()[0].value.scalar() == "new");
}

TEST_CASE("YamlNode setMember replaces previous borrowed scalar kind with mapping",
          "[job_yaml][node][v1][transition][borrowed]")
{
    constexpr std::string_view source = "old";

    YamlNode node;
    node.setScalarView(source);

    node.setMember("child", YamlNode{"new"});

    const YamlNode &result = node;

    REQUIRE(result.isMapping());
    REQUIRE(result.scalarStorage() == YamlNode::ScalarStorage::None);
    REQUIRE_FALSE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().size() == 1);
}

TEST_CASE("YamlNode append replaces previous mapping kind with sequence",
          "[job_yaml][node][v1][transition]")
{
    YamlNode node;

    node.setMemberScalar("name", "alpha");

    node.append(YamlNode{"item"});

    const YamlNode &result = node;

    REQUIRE(result.isSequence());
    REQUIRE(result.scalar().empty());
    REQUIRE(result.mapping().empty());
    REQUIRE(result.sequence().size() == 1);
    REQUIRE(result.sequence()[0].scalar() == "item");
}

// ========================================
// Empty structural values
// ========================================

TEST_CASE("YamlNode mapping can contain empty mapping",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode emptyMapping;
    emptyMapping.setMapping();

    YamlNode root;
    root.setMember("empty", std::move(emptyMapping));

    REQUIRE(root.mapping().size() == 1);
    REQUIRE(root.mapping()[0].value.isMapping());
    REQUIRE(root.mapping()[0].value.mapping().empty());
}

TEST_CASE("YamlNode mapping can contain empty sequence",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode emptySequence;
    emptySequence.setSequence();

    YamlNode root;
    root.setMember("empty", std::move(emptySequence));

    REQUIRE(root.mapping().size() == 1);
    REQUIRE(root.mapping()[0].value.isSequence());
    REQUIRE(root.mapping()[0].value.sequence().empty());
}

// ========================================
// Binary-safe recursive scalar payloads
// ========================================

TEST_CASE("YamlNode nested owning scalar preserves embedded null",
          "[job_yaml][node][v1][recursive]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    YamlNode child{
        std::string_view{source, sizeof(source)}
    };

    YamlNode root;
    root.setMember("value", std::move(child));

    const YamlNode &scalar = root.mapping()[0].value;
    const std::string_view value = scalar.scalar();

    REQUIRE(scalar.ownsScalar());
    REQUIRE(value.size() == sizeof(source));
    REQUIRE(value[0] == 'A');
    REQUIRE(value[1] == '\0');
    REQUIRE(value[2] == 'B');
}

TEST_CASE("YamlNode nested borrowed scalar preserves embedded null",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    YamlNode child;
    child.setScalarView(std::string_view{source, sizeof(source)});

    YamlNode root;
    root.setMember("value", std::move(child));

    const YamlNode &scalar = root.mapping()[0].value;
    const std::string_view value = scalar.scalar();

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(value.data() == source);
    REQUIRE(value.size() == sizeof(source));
    REQUIRE(value[0] == 'A');
    REQUIRE(value[1] == '\0');
    REQUIRE(value[2] == 'B');
}

TEST_CASE("YamlNode nested owning scalar preserves UTF-8",
          "[job_yaml][node][v1][recursive]")
{
    YamlNode root;
    root.setMember("message", YamlNode{"alpha € 😀"});

    REQUIRE(root.mapping()[0].value.scalar() == "alpha € 😀");
}

TEST_CASE("YamlNode nested borrowed scalar preserves UTF-8",
          "[job_yaml][node][v1][recursive][borrowed]")
{
    constexpr std::string_view source = "alpha € 😀";

    YamlNode child;
    child.setScalarView(source);

    YamlNode root;
    root.setMember("message", std::move(child));

    const YamlNode &scalar = root.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == source);
    REQUIRE(scalar.scalar().data() == source.data());
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlNode structural mapping benchmark",
          "[job_yaml][node][v1][benchmark]")
{
    std::string_view key = "child";
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode structural setMember owning")
    {
        YamlNode root;
        YamlNode child{value};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        root.setMember(key, std::move(child));

        benchmarkClobber(root);

        return root.mapping().size();
    };

    BENCHMARK("YamlNode structural setMember borrowed")
    {
        YamlNode root;
        YamlNode child;
        child.setScalarView(value);

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        root.setMember(key, std::move(child));

        benchmarkClobber(root);

        return root.mapping().size();
    };

    BENCHMARK("YamlNode scalar setMemberScalar owning")
    {
        YamlNode root;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        root.setMemberScalar(key, value);

        benchmarkClobber(root);

        return root.mapping().size();
    };

    BENCHMARK("YamlNode scalar setMemberScalarView borrowed")
    {
        YamlNode root;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        root.setMemberScalarView(key, value);

        benchmarkClobber(root);

        return root.mapping().size();
    };
}

TEST_CASE("YamlNode structural sequence benchmark",
          "[job_yaml][node][v1][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode structural append owning")
    {
        YamlNode root;
        YamlNode child{value};

        benchmarkDoNotOptimize(value);

        root.append(std::move(child));

        benchmarkClobber(root);

        return root.sequence().size();
    };

    BENCHMARK("YamlNode structural append borrowed")
    {
        YamlNode root;
        YamlNode child;
        child.setScalarView(value);

        benchmarkDoNotOptimize(value);

        root.append(std::move(child));

        benchmarkClobber(root);

        return root.sequence().size();
    };

    BENCHMARK("YamlNode scalar appendScalar owning")
    {
        YamlNode root;

        benchmarkDoNotOptimize(value);

        root.appendScalar(value);

        benchmarkClobber(root);

        return root.sequence().size();
    };

    BENCHMARK("YamlNode scalar appendScalarView borrowed")
    {
        YamlNode root;

        benchmarkDoNotOptimize(value);

        root.appendScalarView(value);

        benchmarkClobber(root);

        return root.sequence().size();
    };
}

TEST_CASE("YamlNode nested mapping construction benchmark",
          "[job_yaml][node][v1][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode mapping containing mapping owning")
    {
        YamlNode child;
        child.setMemberScalar("name", value);
        child.setMemberScalar("count", "42");

        YamlNode root;
        root.setMember("child", std::move(child));

        benchmarkClobber(root);

        return root.mapping()[0].value.mapping().size();
    };

    BENCHMARK("YamlNode mapping containing mapping borrowed")
    {
        YamlNode child;
        child.setMemberScalarView("name", value);
        child.setMemberScalarView("count", "42");

        YamlNode root;
        root.setMember("child", std::move(child));

        benchmarkClobber(root);

        return root.mapping()[0].value.mapping().size();
    };
}

TEST_CASE("YamlNode sequence of sequences benchmark",
          "[job_yaml][node][v1][benchmark]")
{
    std::string_view key = "key";
    std::string_view value = "value";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNode sequence of two-value owning sequences")
    {
        YamlNode entry;
        entry.setSequence();
        entry.appendScalar(key);
        entry.appendScalar(value);

        YamlNode root;
        root.setSequence();
        root.append(std::move(entry));

        benchmarkClobber(root);

        return root.sequence()[0].sequence().size();
    };

    BENCHMARK("YamlNode sequence of two-value borrowed sequences")
    {
        YamlNode entry;
        entry.setSequence();
        entry.appendScalarView(key);
        entry.appendScalarView(value);

        YamlNode root;
        root.setSequence();
        root.append(std::move(entry));

        benchmarkClobber(root);

        return root.sequence()[0].sequence().size();
    };
}

#endif

} // namespace job::yaml::tests