#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <memory>
#include <optional>
#include <cstdint>
#include <string>
#include <string_view>

#include <job_yaml.h>

#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

namespace job::yaml::tests {

// ========================================
// Public umbrella
// ========================================

static_assert(YamlNodeDestination<YamlNode>);
static_assert(YamlOutputSink<std::string>);
static_assert(YamlOutputSink<FixedStringSink>);

// ========================================
// Scalar assignment
// ========================================

TEST_CASE("JobYaml assigns signed integer scalar",
          "[job_yaml][facade]")
{
    int value{};

    REQUIRE(JobYaml::assign(value, "42"));
    REQUIRE(value == 42);
}

TEST_CASE("JobYaml assigns unsigned integer scalar",
          "[job_yaml][facade]")
{
    std::uint32_t value{};

    REQUIRE(JobYaml::assign(value, "42"));
    REQUIRE(value == 42u);
}

TEST_CASE("JobYaml assigns boolean scalar",
          "[job_yaml][facade]")
{
    bool value{};

    REQUIRE(JobYaml::assign(value, "true"));
    REQUIRE(value);
}

TEST_CASE("JobYaml assigns floating point scalar",
          "[job_yaml][facade]")
{
    double value{};

    REQUIRE(JobYaml::assign(value, "42.5"));
    REQUIRE(value == 42.5);
}

TEST_CASE("JobYaml assigns owned string scalar",
          "[job_yaml][facade]")
{
    std::string value;

    REQUIRE(JobYaml::assign(value, "Joseph"));
    REQUIRE(value == "Joseph");
}

TEST_CASE("JobYaml rejects invalid scalar conversion",
          "[job_yaml][facade]")
{
    int value{123};

    REQUIRE_FALSE(JobYaml::assign(value, "Joseph"));
    REQUIRE(value == 123);
}

TEST_CASE("JobYaml preserves exact owned string scalar",
          "[job_yaml][facade]")
{
    std::string value;

    REQUIRE(JobYaml::assign(value, "  Joseph  "));
    REQUIRE(value == "  Joseph  ");
}

TEST_CASE("JobYaml scalar assignment respects string_view length",
          "[job_yaml][facade]")
{
    constexpr char source[] = "42-extra";
    const std::string_view input{source, 2};

    int value{};

    REQUIRE(JobYaml::assign(value, input));
    REQUIRE(value == 42);
}

TEST_CASE("JobYaml string assignment preserves embedded null",
          "[job_yaml][facade]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    std::string value;

    REQUIRE(JobYaml::assign(
        value,
        std::string_view{source, sizeof(source)}));

    REQUIRE(value.size() == sizeof(source));
    REQUIRE(value[0] == 'J');
    REQUIRE(value[3] == '\0');
    REQUIRE(value[4] == 'Y');
    REQUIRE(value[7] == 'L');
}

// ========================================
// Reflected object assignment
// ========================================

TEST_CASE("JobYaml assigns reflected integer member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "count", "42"));
    REQUIRE(object.count == 42);
}

TEST_CASE("JobYaml assigns reflected unsigned member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "size", "4096"));
    REQUIRE(object.size == 4096u);
}

TEST_CASE("JobYaml assigns reflected boolean member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "enabled", "true"));
    REQUIRE(object.enabled);
}

TEST_CASE("JobYaml assigns reflected float member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "scale", "1.5"));
    REQUIRE(object.scale == 1.5f);
}

TEST_CASE("JobYaml assigns reflected double member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "ratio", "2.25"));
    REQUIRE(object.ratio == 2.25);
}

TEST_CASE("JobYaml assigns reflected string member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE(JobYaml::assign(object, "name", "Joseph"));
    REQUIRE(object.name == "Joseph");
}

TEST_CASE("JobYaml rejects unknown reflected member",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;

    REQUIRE_FALSE(JobYaml::assign(
        object,
        "does_not_exist",
        "42"));
}

TEST_CASE("JobYaml rejects invalid reflected member value",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;
    object.count = 123;

    REQUIRE_FALSE(JobYaml::assign(
        object,
        "count",
        "Joseph"));

    REQUIRE(object.count == 123);
}

// ========================================
// Scalar validation
// ========================================

TEST_CASE("JobYaml validates signed integer scalar",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<int>("42"));
    REQUIRE(JobYaml::validate<int>("-42"));
    REQUIRE_FALSE(JobYaml::validate<int>("Joseph"));
}

TEST_CASE("JobYaml validates unsigned integer scalar",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<std::uint32_t>("42"));
    REQUIRE_FALSE(JobYaml::validate<std::uint32_t>("-1"));
}

TEST_CASE("JobYaml validates boolean scalar",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<bool>("true"));
    REQUIRE(JobYaml::validate<bool>("false"));
    REQUIRE_FALSE(JobYaml::validate<bool>("yes please"));
}

TEST_CASE("JobYaml validates floating point scalar",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<double>("42.5"));
    REQUIRE(JobYaml::validate<double>(".inf"));
    REQUIRE_FALSE(JobYaml::validate<double>("Joseph"));
}

TEST_CASE("JobYaml validates owned string scalar",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<std::string>(""));
    REQUIRE(JobYaml::validate<std::string>("Joseph"));
    REQUIRE(JobYaml::validate<std::string>("42"));
}

// ========================================
// Reflected object validation
// ========================================

TEST_CASE("JobYaml validates reflected member without object instance",
          "[job_yaml][facade]")
{
    REQUIRE(JobYaml::validate<ObjectReaderObject>(
        "count",
        "42"));

    REQUIRE(JobYaml::validate<ObjectReaderObject>(
        "enabled",
        "true"));

    REQUIRE(JobYaml::validate<ObjectReaderObject>(
        "ratio",
        "2.5"));

    REQUIRE(JobYaml::validate<ObjectReaderObject>(
        "name",
        "Joseph"));
}

TEST_CASE("JobYaml rejects invalid reflected member value during validation",
          "[job_yaml][facade]")
{
    REQUIRE_FALSE(JobYaml::validate<ObjectReaderObject>(
        "count",
        "Joseph"));

    REQUIRE_FALSE(JobYaml::validate<ObjectReaderObject>(
        "enabled",
        "Joseph"));
}

TEST_CASE("JobYaml rejects unknown reflected member during validation",
          "[job_yaml][facade]")
{
    REQUIRE_FALSE(JobYaml::validate<ObjectReaderObject>(
        "does_not_exist",
        "42"));
}

// ========================================
// Scalar emission
// ========================================

TEST_CASE("JobYaml emits signed integer",
          "[job_yaml][facade]")
{
    std::string output;

    REQUIRE(JobYaml::emit(42, output));
    REQUIRE(output == "42");
}

TEST_CASE("JobYaml emits boolean",
          "[job_yaml][facade]")
{
    std::string output;

    REQUIRE(JobYaml::emit(true, output));
    REQUIRE(output == "true");
}

TEST_CASE("JobYaml emits floating point",
          "[job_yaml][facade]")
{
    std::string output;

    REQUIRE(JobYaml::emit(42.5, output));
    REQUIRE(output == "42.5");
}

TEST_CASE("JobYaml emits string",
          "[job_yaml][facade]")
{
    std::string output;

    REQUIRE(JobYaml::emit(
        std::string{"Joseph"},
        output));

    REQUIRE(output == "\"Joseph\"");
}

TEST_CASE("JobYaml emits scalar into fixed sink",
          "[job_yaml][facade]")
{
    FixedStringSink sink;

    REQUIRE(JobYaml::emit(42, sink));
    REQUIRE(sink.view() == "42");
}

// ========================================
// Reflected object emission
// ========================================

TEST_CASE("JobYaml emits reflected object",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;
    object.count = 42;
    object.size = 4096;
    object.enabled = true;
    object.scale = 1.5f;
    object.ratio = 2.25;
    object.name = "Joseph";

    std::string output;

    REQUIRE(JobYaml::emit(object, output));

    REQUIRE(output ==
            "count: 42\n"
            "size: 4096\n"
            "enabled: true\n"
            "scale: 1.5\n"
            "ratio: 2.25\n"
            "name: \"Joseph\"\n"
            "view: \"\"\n"
            "mode: 0\n");
}

TEST_CASE("JobYaml emits reflected object into fixed sink",
          "[job_yaml][facade]")
{
    ObjectReaderObject object;
    object.count = 42;
    object.size = 64;
    object.enabled = true;
    object.scale = 1.0f;
    object.ratio = 2.0;
    object.name = "JOB";

    FixedStringSink sink;

    REQUIRE(JobYaml::emit(object, sink));

    REQUIRE(sink.view() ==
            "count: 42\n"
            "size: 64\n"
            "enabled: true\n"
            "scale: 1\n"
            "ratio: 2\n"
            "name: \"JOB\"\n"
            "view: \"\"\n"
            "mode: 0\n");
}

// ========================================
// Dynamic node scalar
// ========================================

TEST_CASE("JobYaml writes scalar dynamic node",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::node(node, "Joseph"));

    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "Joseph");
}

TEST_CASE("JobYaml dynamic node preserves exact scalar text",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::node(node, "42"));

    REQUIRE(node.isScalar());
    REQUIRE(node.scalar() == "42");
}

// ========================================
// Dynamic node mapping
// ========================================

TEST_CASE("JobYaml writes dynamic node mapping member",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::node(node, "name", "Joseph"));

    REQUIRE(node.isMapping());
    REQUIRE(node.mapping().size() == 1);
    REQUIRE(node.mapping()[0].key == "name");
    REQUIRE(node.mapping()[0].value.scalar() == "Joseph");
}

TEST_CASE("JobYaml preserves dynamic node mapping order",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::node(node, "first", "1"));
    REQUIRE(JobYaml::node(node, "second", "2"));
    REQUIRE(JobYaml::node(node, "third", "3"));

    REQUIRE(node.mapping().size() == 3);
    REQUIRE(node.mapping()[0].key == "first");
    REQUIRE(node.mapping()[1].key == "second");
    REQUIRE(node.mapping()[2].key == "third");
}

// ========================================
// Dynamic node sequence
// ========================================

TEST_CASE("JobYaml appends dynamic node sequence item",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::append(node, "Joseph"));

    REQUIRE(node.isSequence());
    REQUIRE(node.sequence().size() == 1);
    REQUIRE(node.sequence()[0].scalar() == "Joseph");
}

TEST_CASE("JobYaml preserves dynamic node sequence order",
          "[job_yaml][facade][node]")
{
    YamlNode node;

    REQUIRE(JobYaml::append(node, "first"));
    REQUIRE(JobYaml::append(node, "second"));
    REQUIRE(JobYaml::append(node, "third"));

    REQUIRE(node.sequence().size() == 3);
    REQUIRE(node.sequence()[0].scalar() == "first");
    REQUIRE(node.sequence()[1].scalar() == "second");
    REQUIRE(node.sequence()[2].scalar() == "third");
}

// ========================================
// Facade semantic agreement
// ========================================

TEST_CASE("JobYaml scalar assignment agrees with YamlSink",
          "[job_yaml][facade]")
{
    int facadeValue{};
    int directValue{};

    REQUIRE(JobYaml::assign(facadeValue, "42"));
    REQUIRE(YamlSink::scalar(directValue, "42"));

    REQUIRE(facadeValue == directValue);
}

TEST_CASE("JobYaml member assignment agrees with YamlObjectSink",
          "[job_yaml][facade]")
{
    ObjectReaderObject facadeObject;
    ObjectReaderObject directObject;

    REQUIRE(JobYaml::assign(
        facadeObject,
        "count",
        "42"));

    REQUIRE(YamlObjectSink::member(
        directObject,
        "count",
        "42"));

    REQUIRE(facadeObject.count == directObject.count);
}

TEST_CASE("JobYaml scalar validation agrees with YamlValidateSink",
          "[job_yaml][facade]")
{
    REQUIRE(
        JobYaml::validate<int>("42") ==
        YamlValidateSink::scalar<int>("42"));

    REQUIRE(
        JobYaml::validate<int>("Joseph") ==
        YamlValidateSink::scalar<int>("Joseph"));
}

TEST_CASE("JobYaml member validation agrees with YamlValidateSink",
          "[job_yaml][facade]")
{
    REQUIRE(
        JobYaml::validate<ObjectReaderObject>(
            "count",
            "42") ==
        YamlValidateSink::member<ObjectReaderObject>(
            "count",
            "42"));
}

TEST_CASE("JobYaml node scalar agrees with YamlNodeSink",
          "[job_yaml][facade][node]")
{
    YamlNode facadeNode;
    YamlNode directNode;

    REQUIRE(JobYaml::node(
        facadeNode,
        "Joseph"));

    REQUIRE(YamlNodeSink::scalar(
        directNode,
        "Joseph"));

    REQUIRE(facadeNode.type() == directNode.type());
    REQUIRE(facadeNode.scalar() == directNode.scalar());
}

TEST_CASE("JobYaml node member agrees with YamlNodeSink",
          "[job_yaml][facade][node]")
{
    YamlNode facadeNode;
    YamlNode directNode;

    REQUIRE(JobYaml::node(
        facadeNode,
        "name",
        "Joseph"));

    REQUIRE(YamlNodeSink::member(
        directNode,
        "name",
        "Joseph"));

    REQUIRE(facadeNode.type() == directNode.type());
    REQUIRE(facadeNode.mapping().size() ==
            directNode.mapping().size());

    REQUIRE(facadeNode.mapping()[0].key ==
            directNode.mapping()[0].key);

    REQUIRE(facadeNode.mapping()[0].value.scalar() ==
            directNode.mapping()[0].value.scalar());
}

TEST_CASE("JobYaml node append agrees with YamlNodeSink",
          "[job_yaml][facade][node]")
{
    YamlNode facadeNode;
    YamlNode directNode;

    REQUIRE(JobYaml::append(
        facadeNode,
        "Joseph"));

    REQUIRE(YamlNodeSink::append(
        directNode,
        "Joseph"));

    REQUIRE(facadeNode.type() == directNode.type());
    REQUIRE(facadeNode.sequence().size() ==
            directNode.sequence().size());

    REQUIRE(facadeNode.sequence()[0].scalar() ==
            directNode.sequence()[0].scalar());
}


// ========================================
// Nullable emission
// ========================================

TEST_CASE("JobYaml emits empty optional as null",
          "[job_yaml][facade][optional][null]")
{
    const std::optional<int> value;

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JobYaml emits populated optional value",
          "[job_yaml][facade][optional]")
{
    const std::optional<int> value{42};

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JobYaml emits null shared pointer as null",
          "[job_yaml][facade][pointer][shared_pointer][null]")
{
    const std::shared_ptr<int> value;

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JobYaml emits populated shared pointer value",
          "[job_yaml][facade][pointer][shared_pointer]")
{
    const auto value = std::make_shared<int>(42);

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JobYaml emits null unique pointer as null",
          "[job_yaml][facade][pointer][unique_pointer][null]")
{
    const std::unique_ptr<int> value;

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JobYaml emits populated unique pointer value",
          "[job_yaml][facade][pointer][unique_pointer]")
{
    const auto value = std::make_unique<int>(42);

    std::string output;

    REQUIRE(JobYaml::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JobYaml emits null dynamic node",
          "[job_yaml][facade][node][null]")
{
    YamlNode node;
    node.setNull();

    std::string output;

    REQUIRE(JobYaml::emit(node, output));
    REQUIRE(output == "null");
}



// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("JobYaml scalar assignment benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view value = "42";

    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml assign scalar")
    {
        int destination{};

        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::assign(destination, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };

    BENCHMARK("YamlSink scalar")
    {
        int destination{};

        benchmarkDoNotOptimize(value);

        const bool result =
            YamlSink::scalar(destination, value);

        benchmarkClobber(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };
}

TEST_CASE("JobYaml reflected member assignment benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view key = "count";
    std::string_view value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml assign member")
    {
        ObjectReaderObject object;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::assign(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };

    BENCHMARK("YamlObjectSink member")
    {
        ObjectReaderObject object;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlObjectSink::member(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("JobYaml scalar validation benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view value = "42";

    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml validate scalar")
    {
        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::validate<int>(value);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlValidateSink scalar")
    {
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlValidateSink::scalar<int>(value);

        benchmarkDoNotOptimize(result);

        return result;
    };
}

TEST_CASE("JobYaml reflected member validation benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view key = "count";
    std::string_view value = "42";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml validate member")
    {
        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::validate<ObjectReaderObject>(
                key,
                value);

        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlValidateSink member")
    {
        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlValidateSink::member<ObjectReaderObject>(
                key,
                value);

        benchmarkDoNotOptimize(result);

        return result;
    };
}

TEST_CASE("JobYaml scalar emission benchmark",
          "[job_yaml][facade][benchmark]")
{
    int value = 42;

    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml emit scalar")
    {
        FixedStringSink sink;

        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::emit(value, sink);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.used;
    };

    BENCHMARK("YamlEmitter scalar")
    {
        FixedStringSink sink;

        benchmarkDoNotOptimize(value);

        const bool result =
            YamlEmitter::emitScalar(value, sink);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.used;
    };
}

TEST_CASE("JobYaml reflected object emission benchmark",
          "[job_yaml][facade][benchmark]")
{
    ObjectReaderObject object;
    object.count = 42;
    object.size = 64;
    object.enabled = true;
    object.scale = 1.5f;
    object.ratio = 2.25;
    object.name = "Joseph";

    benchmarkDoNotOptimize(object);

    BENCHMARK("JobYaml emit object")
    {
        FixedStringSink sink;

        benchmarkDoNotOptimize(object);

        const bool result =
            JobYaml::emit(object, sink);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.used;
    };

    BENCHMARK("YamlEmitter object")
    {
        FixedStringSink sink;

        benchmarkDoNotOptimize(object);

        const bool result =
            YamlEmitter::emitObject(object, sink);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.used;
    };
}

TEST_CASE("JobYaml dynamic scalar node benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml node scalar")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::node(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.scalar().size();
    };

    BENCHMARK("YamlNodeSink scalar")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result =
            YamlNodeSink::scalar(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.scalar().size();
    };
}

TEST_CASE("JobYaml dynamic mapping node benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view key = "name";
    std::string_view value = "Joseph";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml node member")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::node(node, key, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.mapping().size();
    };

    BENCHMARK("YamlNodeSink member")
    {
        YamlNode node;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result =
            YamlNodeSink::member(node, key, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.mapping().size();
    };
}

TEST_CASE("JobYaml dynamic sequence node benchmark",
          "[job_yaml][facade][benchmark]")
{
    std::string_view value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("JobYaml node append")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result =
            JobYaml::append(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.sequence().size();
    };

    BENCHMARK("YamlNodeSink append")
    {
        YamlNode node;

        benchmarkDoNotOptimize(value);

        const bool result =
            YamlNodeSink::append(node, value);

        benchmarkClobber(node);
        benchmarkDoNotOptimize(result);

        return node.sequence().size();
    };
}

#endif

} // namespace job::yaml::tests