#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_yaml_node.h>
#include <job_yaml_node_sink.h>

#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

#include <string>
#include <string_view>

namespace job::yaml::tests {

// ========================================
// Concept
// ========================================

static_assert(YamlNodeDestination<NodeSinkFixture>);
static_assert(YamlNodeDestination<YamlNode>);

// ========================================
// Scalar destination
// ========================================

TEST_CASE("YamlNodeSink writes scalar destination", "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "alpha"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink overwrites previous scalar destination",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "first"));
    REQUIRE(YamlNodeSink::scalar(destination, "second"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "second");
}

TEST_CASE("YamlNodeSink preserves scalar whitespace exactly",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "  alpha  "));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value == "  alpha  ");
}

TEST_CASE("YamlNodeSink preserves scalar syntax exactly",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "true"));
    REQUIRE(destination.value == "true");

    REQUIRE(YamlNodeSink::scalar(destination, "42"));
    REQUIRE(destination.value == "42");

    REQUIRE(YamlNodeSink::scalar(destination, ".inf"));
    REQUIRE(destination.value == ".inf");

    REQUIRE(YamlNodeSink::scalar(destination, "\"alpha\""));
    REQUIRE(destination.value == "\"alpha\"");
}

TEST_CASE("YamlNodeSink does not interpret scalar escapes",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "hello\\nworld"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value == "hello\\nworld");
}

// ========================================
// Empty scalar contract
// ========================================

TEST_CASE("YamlNodeSink accepts default empty string_view scalar",
          "[job_yaml][node_sink][contract]")
{
    NodeSinkFixture destination;
    const std::string_view value{};

    REQUIRE(YamlNodeSink::scalar(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value.empty());
}

TEST_CASE("YamlNodeSink accepts non null empty string_view scalar",
          "[job_yaml][node_sink][contract]")
{
    NodeSinkFixture destination;
    constexpr char source[] = "";
    const std::string_view value{source, 0};

    REQUIRE(YamlNodeSink::scalar(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value.empty());
}

// ========================================
// Scalar string_view boundaries
// ========================================

TEST_CASE("YamlNodeSink respects scalar string_view length",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink accepts non null terminated scalar string_view",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = {
        'x',
        'J', 'O', 'B',
        'x'
    };

    const std::string_view value{source + 1, 3};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value == "JOB");
}

TEST_CASE("YamlNodeSink preserves embedded null in scalar",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value.size() == sizeof(source));
    REQUIRE(destination.value[0] == 'J');
    REQUIRE(destination.value[2] == 'B');
    REQUIRE(destination.value[3] == '\0');
    REQUIRE(destination.value[4] == 'Y');
    REQUIRE(destination.value[7] == 'L');
}

// ========================================
// Borrowed scalar destination
// ========================================

TEST_CASE("YamlNodeSink scalarView stores borrowed scalar",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode destination;

    REQUIRE(YamlNodeSink::scalarView(destination, source));

    const YamlNode &result = destination;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE_FALSE(result.ownsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink scalarView preserves exact source slice",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode destination;

    REQUIRE(YamlNodeSink::scalarView(destination, value));

    const YamlNode &result = destination;

    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar() == "alpha");
    REQUIRE(result.scalar().data() == source.data() + 6);
    REQUIRE(result.scalar().data() == value.data());
    REQUIRE(result.scalar().size() == 5);
}

TEST_CASE("YamlNodeSink scalarView accepts empty source view",
          "[job_yaml][node_sink][source_view]")
{
    const std::string_view value{};

    YamlNode destination;

    REQUIRE(YamlNodeSink::scalarView(destination, value));

    const YamlNode &result = destination;

    REQUIRE(result.isScalar());
    REQUIRE(result.borrowsScalar());
    REQUIRE(result.scalar().empty());
}

TEST_CASE("YamlNodeSink scalarView preserves embedded null",
          "[job_yaml][node_sink][source_view]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    const std::string_view value{source, sizeof(source)};

    YamlNode destination;

    REQUIRE(YamlNodeSink::scalarView(destination, value));

    const YamlNode &result = destination;
    const std::string_view scalar = result.scalar();

    REQUIRE(result.borrowsScalar());
    REQUIRE(scalar.data() == source);
    REQUIRE(scalar.size() == sizeof(source));
    REQUIRE(scalar[0] == 'A');
    REQUIRE(scalar[1] == '\0');
    REQUIRE(scalar[2] == 'B');
}

// ========================================
// Mapping member destination
// ========================================

TEST_CASE("YamlNodeSink writes mapping member scalar", "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "name", "alpha"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink preserves mapping key and value exactly",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "strange key", "  value  "));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "strange key");
    REQUIRE(destination.value == "  value  ");
}

TEST_CASE("YamlNodeSink accepts empty mapping key", "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "", "value"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "value");
}

TEST_CASE("YamlNodeSink accepts empty mapping value", "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "name", ""));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value.empty());
}

TEST_CASE("YamlNodeSink respects mapping key string_view length",
          "[job_yaml][node_sink]")
{
    constexpr char keySource[] = "name-extra";
    const std::string_view key{keySource, 4};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, key, "alpha"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink respects mapping value string_view length",
          "[job_yaml][node_sink]")
{
    constexpr char valueSource[] = "alpha-extra";
    const std::string_view value{valueSource, 5};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "name", value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink preserves embedded null in mapping value",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "name", value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value.size() == sizeof(source));
    REQUIRE(destination.value[3] == '\0');
}

// ========================================
// Borrowed mapping member destination
// ========================================

TEST_CASE("YamlNodeSink memberView stores borrowed scalar",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode destination;

    REQUIRE(YamlNodeSink::memberView(destination, "name", source));

    REQUIRE(destination.isMapping());
    REQUIRE(destination.mapping().size() == 1);
    REQUIRE(destination.mapping()[0].key == "name");

    const YamlNode &value = destination.mapping()[0].value;

    REQUIRE(value.borrowsScalar());
    REQUIRE_FALSE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink memberView preserves exact source slice",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode destination;

    REQUIRE(YamlNodeSink::memberView(destination, "name", value));

    const YamlNode &scalar = destination.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

TEST_CASE("YamlNodeSink memberView keeps mapping key owned",
          "[job_yaml][node_sink][source_view]")
{
    char keySource[] = "name";
    constexpr std::string_view value = "alpha";

    YamlNode destination;

    REQUIRE(YamlNodeSink::memberView(
        destination,
        std::string_view{keySource, 4},
        value));

    keySource[0] = 'X';

    REQUIRE(destination.mapping()[0].key == "name");

    const YamlNode &scalar = destination.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == value.data());
}

TEST_CASE("YamlNodeSink memberView accepts empty scalar view",
          "[job_yaml][node_sink][source_view]")
{
    YamlNode destination;

    REQUIRE(YamlNodeSink::memberView(destination, "name", {}));

    REQUIRE(destination.mapping().size() == 1);

    const YamlNode &value = destination.mapping()[0].value;

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar().empty());
}

// ========================================
// Sequence append destination
// ========================================

TEST_CASE("YamlNodeSink appends scalar", "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(destination, "alpha"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink preserves appended scalar exactly",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(destination, "  42  "));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "  42  ");
}

TEST_CASE("YamlNodeSink accepts empty appended scalar",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(destination, ""));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.value.empty());
}

TEST_CASE("YamlNodeSink respects appended scalar string_view length",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(destination, value));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.value == "alpha");
}

TEST_CASE("YamlNodeSink preserves embedded null in appended scalar",
          "[job_yaml][node_sink]")
{
    constexpr char source[] = {
        'A', '\0', 'B'
    };

    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(
        destination,
        std::string_view{source, sizeof(source)}));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.value.size() == sizeof(source));
    REQUIRE(destination.value[0] == 'A');
    REQUIRE(destination.value[1] == '\0');
    REQUIRE(destination.value[2] == 'B');
}

// ========================================
// Borrowed sequence append destination
// ========================================

TEST_CASE("YamlNodeSink appendView stores borrowed scalar",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode destination;

    REQUIRE(YamlNodeSink::appendView(destination, source));

    REQUIRE(destination.isSequence());
    REQUIRE(destination.sequence().size() == 1);

    const YamlNode &value = destination.sequence()[0];

    REQUIRE(value.borrowsScalar());
    REQUIRE_FALSE(value.ownsScalar());
    REQUIRE(value.scalar() == "alpha");
    REQUIRE(value.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink appendView preserves exact source slice",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    YamlNode destination;

    REQUIRE(YamlNodeSink::appendView(destination, value));

    const YamlNode &scalar = destination.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar() == "alpha");
    REQUIRE(scalar.scalar().data() == source.data() + 6);
    REQUIRE(scalar.scalar().size() == 5);
}

TEST_CASE("YamlNodeSink appendView accepts empty scalar view",
          "[job_yaml][node_sink][source_view]")
{
    YamlNode destination;

    REQUIRE(YamlNodeSink::appendView(destination, {}));

    REQUIRE(destination.sequence().size() == 1);

    const YamlNode &value = destination.sequence()[0];

    REQUIRE(value.borrowsScalar());
    REQUIRE(value.scalar().empty());
}

// ========================================
// Structural node routing
// ========================================

TEST_CASE("YamlNodeSink writes null structural node",
          "[job_yaml][node_sink][structural]")
{
    YamlNode destination{"alpha"};

    REQUIRE(YamlNodeSink::null(destination));

    REQUIRE(destination.isNull());
}

TEST_CASE("YamlNodeSink writes mapping structural node",
          "[job_yaml][node_sink][structural]")
{
    YamlNode destination{"alpha"};

    REQUIRE(YamlNodeSink::mapping(destination));

    REQUIRE(destination.isMapping());
    REQUIRE(destination.mapping().empty());
}

TEST_CASE("YamlNodeSink writes sequence structural node",
          "[job_yaml][node_sink][structural]")
{
    YamlNode destination{"alpha"};

    REQUIRE(YamlNodeSink::sequence(destination));

    REQUIRE(destination.isSequence());
    REQUIRE(destination.sequence().empty());
}

TEST_CASE("YamlNodeSink moves node into mapping member",
          "[job_yaml][node_sink][structural]")
{
    YamlNode value{"alpha"};
    YamlNode destination;

    REQUIRE(YamlNodeSink::member(destination, "name", std::move(value)));

    REQUIRE(destination.isMapping());
    REQUIRE(destination.mapping().size() == 1);
    REQUIRE(destination.mapping()[0].key == "name");
    REQUIRE(destination.mapping()[0].value.scalar() == "alpha");
}

TEST_CASE("YamlNodeSink moves borrowed node into mapping without materializing",
          "[job_yaml][node_sink][structural][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode value;
    value.setScalarView(source);

    YamlNode destination;

    REQUIRE(YamlNodeSink::member(destination, "name", std::move(value)));

    const YamlNode &scalar = destination.mapping()[0].value;

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink moves node into sequence",
          "[job_yaml][node_sink][structural]")
{
    YamlNode value{"alpha"};
    YamlNode destination;

    REQUIRE(YamlNodeSink::append(destination, std::move(value)));

    REQUIRE(destination.isSequence());
    REQUIRE(destination.sequence().size() == 1);
    REQUIRE(destination.sequence()[0].scalar() == "alpha");
}

TEST_CASE("YamlNodeSink moves borrowed node into sequence without materializing",
          "[job_yaml][node_sink][structural][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode value;
    value.setScalarView(source);

    YamlNode destination;

    REQUIRE(YamlNodeSink::append(destination, std::move(value)));

    const YamlNode &scalar = destination.sequence()[0];

    REQUIRE(scalar.borrowsScalar());
    REQUIRE(scalar.scalar().data() == source.data());
}

// ========================================
// Operation routing
// ========================================

TEST_CASE("YamlNodeSink routes scalar to setScalar only",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "value"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "value");
}

TEST_CASE("YamlNodeSink routes member to setMemberScalar only",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::member(destination, "key", "value"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "key");
    REQUIRE(destination.value == "value");
}

TEST_CASE("YamlNodeSink routes append to appendScalar only",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::append(destination, "value"));

    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "value");
}

// ========================================
// Owning versus borrowed routing
// ========================================

TEST_CASE("YamlNodeSink scalar and scalarView select different storage policies",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode owned;
    YamlNode borrowed;

    REQUIRE(YamlNodeSink::scalar(owned, source));
    REQUIRE(YamlNodeSink::scalarView(borrowed, source));

    const YamlNode &ownedResult = owned;
    const YamlNode &borrowedResult = borrowed;

    REQUIRE(ownedResult.ownsScalar());
    REQUIRE_FALSE(ownedResult.borrowsScalar());
    REQUIRE(ownedResult.scalar() == source);
    REQUIRE(ownedResult.scalar().data() != source.data());

    REQUIRE(borrowedResult.borrowsScalar());
    REQUIRE_FALSE(borrowedResult.ownsScalar());
    REQUIRE(borrowedResult.scalar() == source);
    REQUIRE(borrowedResult.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink member and memberView select different storage policies",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode owned;
    YamlNode borrowed;

    REQUIRE(YamlNodeSink::member(owned, "name", source));
    REQUIRE(YamlNodeSink::memberView(borrowed, "name", source));

    const YamlNode &ownedValue = owned.mapping()[0].value;
    const YamlNode &borrowedValue = borrowed.mapping()[0].value;

    REQUIRE(ownedValue.ownsScalar());
    REQUIRE(ownedValue.scalar().data() != source.data());

    REQUIRE(borrowedValue.borrowsScalar());
    REQUIRE(borrowedValue.scalar().data() == source.data());
}

TEST_CASE("YamlNodeSink append and appendView select different storage policies",
          "[job_yaml][node_sink][source_view]")
{
    constexpr std::string_view source = "alpha";

    YamlNode owned;
    YamlNode borrowed;

    REQUIRE(YamlNodeSink::append(owned, source));
    REQUIRE(YamlNodeSink::appendView(borrowed, source));

    const YamlNode &ownedValue = owned.sequence()[0];
    const YamlNode &borrowedValue = borrowed.sequence()[0];

    REQUIRE(ownedValue.ownsScalar());
    REQUIRE(ownedValue.scalar().data() != source.data());

    REQUIRE(borrowedValue.borrowsScalar());
    REQUIRE(borrowedValue.scalar().data() == source.data());
}

// ========================================
// Sequential operations
// ========================================

TEST_CASE("YamlNodeSink remains usable across different operations",
          "[job_yaml][node_sink]")
{
    NodeSinkFixture destination;

    REQUIRE(YamlNodeSink::scalar(destination, "root"));
    REQUIRE(destination.operation == NodeSinkFixture::Operation::Scalar);
    REQUIRE(destination.value == "root");

    REQUIRE(YamlNodeSink::member(destination, "name", "alpha"));
    REQUIRE(destination.operation == NodeSinkFixture::Operation::Member);
    REQUIRE(destination.key == "name");
    REQUIRE(destination.value == "alpha");

    REQUIRE(YamlNodeSink::append(destination, "item"));
    REQUIRE(destination.operation == NodeSinkFixture::Operation::Append);
    REQUIRE(destination.key.empty());
    REQUIRE(destination.value == "item");
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlNodeSink scalar benchmark",
          "[job_yaml][node_sink][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNodeSink scalar owning")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::scalar(destination, value);
        const YamlNode &parsed = destination;

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return parsed.scalar().size();
    };

    BENCHMARK("YamlNodeSink scalar borrowed")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::scalarView(destination, value);
        const YamlNode &parsed = destination;

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return parsed.scalar().size();
    };

    BENCHMARK("direct setScalar")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        destination.setScalar(value);

        const YamlNode &parsed = destination;

        benchmarkClobber(destination);

        return parsed.scalar().size();
    };

    BENCHMARK("direct setScalarView")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        destination.setScalarView(value);

        const YamlNode &parsed = destination;

        benchmarkClobber(destination);

        return parsed.scalar().size();
    };
}

TEST_CASE("YamlNodeSink member benchmark",
          "[job_yaml][node_sink][benchmark]")
{
    std::string_view key = "name";
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNodeSink member owning")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::member(destination, key, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination.mapping().size();
    };

    BENCHMARK("YamlNodeSink member borrowed")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::memberView(destination, key, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination.mapping().size();
    };

    BENCHMARK("direct setMemberScalar")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        destination.setMemberScalar(key, value);

        benchmarkClobber(destination);

        return destination.mapping().size();
    };

    BENCHMARK("direct setMemberScalarView")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        destination.setMemberScalarView(key, value);

        benchmarkClobber(destination);

        return destination.mapping().size();
    };
}

TEST_CASE("YamlNodeSink append benchmark",
          "[job_yaml][node_sink][benchmark]")
{
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlNodeSink append owning")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::append(destination, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination.sequence().size();
    };

    BENCHMARK("YamlNodeSink append borrowed")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        const bool result = YamlNodeSink::appendView(destination, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination.sequence().size();
    };

    BENCHMARK("direct appendScalar")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        destination.appendScalar(value);

        benchmarkClobber(destination);

        return destination.sequence().size();
    };

    BENCHMARK("direct appendScalarView")
    {
        YamlNode destination;

        benchmarkDoNotOptimize(value);

        destination.appendScalarView(value);

        benchmarkClobber(destination);

        return destination.sequence().size();
    };
}

#endif

} // namespace job::yaml::tests