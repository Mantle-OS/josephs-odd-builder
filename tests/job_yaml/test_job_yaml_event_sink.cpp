#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#include "test_job_yaml_utils.h"
#endif

#include <string>
#include <string_view>
#include <utility>

#include "test_job_yaml_fixtures.h"

#include <job_yaml_concepts.h>
#include <job_yaml_event_sink.h>

namespace job::yaml::tests {


static_assert(noexcept(
    YamlEventSink::scalar(
        std::declval<EventSinkFixture &>(),
        std::string_view{})));

static_assert(noexcept(
    YamlEventSink::scalarOwned(
        std::declval<EventSinkFixture &>(),
        std::string{})));

static_assert(noexcept(
    YamlEventSink::key(
        std::declval<EventSinkFixture &>(),
        std::string_view{})));

static_assert(noexcept(
    YamlEventSink::keyOwned(
        std::declval<EventSinkFixture &>(),
        std::string{})));

static_assert(noexcept(
    YamlEventSink::beginMapping(
        std::declval<EventSinkFixture &>())));

static_assert(noexcept(
    YamlEventSink::endMapping(
        std::declval<EventSinkFixture &>())));

static_assert(noexcept(
    YamlEventSink::beginSequence(
        std::declval<EventSinkFixture &>())));

static_assert(noexcept(
    YamlEventSink::endSequence(
        std::declval<EventSinkFixture &>())));

static_assert(!noexcept(
    YamlEventSink::scalar(
        std::declval<ThrowingEventSinkFixture &>(),
        std::string_view{})));

static_assert(!noexcept(
    YamlEventSink::scalarOwned(
        std::declval<ThrowingEventSinkFixture &>(),
        std::string{})));

static_assert(!noexcept(
    YamlEventSink::key(
        std::declval<ThrowingEventSinkFixture &>(),
        std::string_view{})));

static_assert(!noexcept(
    YamlEventSink::keyOwned(
        std::declval<ThrowingEventSinkFixture &>(),
        std::string{})));

static_assert(!noexcept(
    YamlEventSink::beginMapping(
        std::declval<ThrowingEventSinkFixture &>())));

static_assert(!noexcept(
    YamlEventSink::endMapping(
        std::declval<ThrowingEventSinkFixture &>())));

static_assert(!noexcept(
    YamlEventSink::beginSequence(
        std::declval<ThrowingEventSinkFixture &>())));

static_assert(!noexcept(
    YamlEventSink::endSequence(
        std::declval<ThrowingEventSinkFixture &>())));

// ========================================
// Borrowed scalar event
// ========================================

TEST_CASE("YamlEventSink forwards borrowed scalar event",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, "Joseph"));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value == "Joseph");
}

TEST_CASE("YamlEventSink preserves borrowed scalar event exactly",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, "  Joseph  "));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value == "  Joseph  ");
}

TEST_CASE("YamlEventSink accepts empty borrowed scalar event",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, ""));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink respects borrowed scalar string_view boundaries",
          "[job_yaml][event_sink][scalar][borrowed][source_view]")
{
    constexpr char source[] = "Joseph-extra";
    const std::string_view value{source, 6};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, value));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.value == "Joseph");
}

TEST_CASE("YamlEventSink preserves embedded null in borrowed scalar event",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    constexpr char source[] = {'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'};
    const std::string_view value{source, sizeof(source)};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, value));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.value.size() == sizeof(source));
    REQUIRE(sink.value[0] == 'J');
    REQUIRE(sink.value[2] == 'B');
    REQUIRE(sink.value[3] == '\0');
    REQUIRE(sink.value[4] == 'Y');
    REQUIRE(sink.value[7] == 'L');
}

// ========================================
// Owned scalar event
// ========================================

TEST_CASE("YamlEventSink forwards owned scalar event",
          "[job_yaml][event_sink][scalar][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"Joseph"}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value == "Joseph");
}

TEST_CASE("YamlEventSink preserves owned scalar event exactly",
          "[job_yaml][event_sink][scalar][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"  Joseph  "}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value == "  Joseph  ");
}

TEST_CASE("YamlEventSink accepts empty owned scalar event",
          "[job_yaml][event_sink][scalar][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink preserves embedded null in owned scalar event",
          "[job_yaml][event_sink][scalar][owned]")
{
    std::string value{"JOB\0YAML", 8};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalarOwned(sink, std::move(value)));

    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.value.size() == 8);
    REQUIRE(sink.value[0] == 'J');
    REQUIRE(sink.value[2] == 'B');
    REQUIRE(sink.value[3] == '\0');
    REQUIRE(sink.value[4] == 'Y');
    REQUIRE(sink.value[7] == 'L');
}

// ========================================
// Scalar routing remains semantic-neutral
// ========================================

TEST_CASE("YamlEventSink does not classify borrowed scalar payload",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, "true"));
    REQUIRE(sink.value == "true");

    REQUIRE(YamlEventSink::scalar(sink, "42"));
    REQUIRE(sink.value == "42");

    REQUIRE(YamlEventSink::scalar(sink, ".inf"));
    REQUIRE(sink.value == ".inf");

    REQUIRE(YamlEventSink::scalar(sink, "\"Joseph\""));
    REQUIRE(sink.value == "\"Joseph\"");
}

TEST_CASE("YamlEventSink does not interpret borrowed scalar escapes",
          "[job_yaml][event_sink][scalar][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, "hello\\nworld"));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.value == "hello\\nworld");
}

TEST_CASE("YamlEventSink does not reinterpret owned scalar payload",
          "[job_yaml][event_sink][scalar][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"hello\nworld"}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.value == "hello\nworld");
}

// ========================================
// Borrowed key event
// ========================================

TEST_CASE("YamlEventSink forwards borrowed key event",
          "[job_yaml][event_sink][key][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, "name"));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue == "name");
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink preserves borrowed key event exactly",
          "[job_yaml][event_sink][key][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, "strange key"));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue == "strange key");
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink accepts empty borrowed key event",
          "[job_yaml][event_sink][key][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, ""));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink respects borrowed key string_view boundaries",
          "[job_yaml][event_sink][key][borrowed][source_view]")
{
    constexpr char source[] = "name-extra";
    const std::string_view key{source, 4};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, key));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue == "name");
}

TEST_CASE("YamlEventSink preserves embedded null in borrowed key event",
          "[job_yaml][event_sink][key][borrowed]")
{
    constexpr char source[] = {'A', '\0', 'B'};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, std::string_view{source, sizeof(source)}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue.size() == sizeof(source));
    REQUIRE(sink.keyValue[0] == 'A');
    REQUIRE(sink.keyValue[1] == '\0');
    REQUIRE(sink.keyValue[2] == 'B');
}

// ========================================
// Owned key event
// ========================================

TEST_CASE("YamlEventSink forwards owned key event",
          "[job_yaml][event_sink][key][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{"name"}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue == "name");
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink preserves owned key event exactly",
          "[job_yaml][event_sink][key][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{"strange key"}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue == "strange key");
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink accepts empty owned key event",
          "[job_yaml][event_sink][key][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{}));

    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink preserves embedded null in owned key event",
          "[job_yaml][event_sink][key][owned]")
{
    std::string key{"A\0B", 3};

    EventSinkFixture sink;

    REQUIRE(YamlEventSink::keyOwned(sink, std::move(key)));

    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue.size() == 3);
    REQUIRE(sink.keyValue[0] == 'A');
    REQUIRE(sink.keyValue[1] == '\0');
    REQUIRE(sink.keyValue[2] == 'B');
}

// ========================================
// Mapping structure
// ========================================

TEST_CASE("YamlEventSink forwards begin mapping event",
          "[job_yaml][event_sink][mapping]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::BeginMapping);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink forwards end mapping event",
          "[job_yaml][event_sink][mapping]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::endMapping(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

// ========================================
// Sequence structure
// ========================================

TEST_CASE("YamlEventSink forwards begin sequence event",
          "[job_yaml][event_sink][sequence]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginSequence(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::BeginSequence);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

TEST_CASE("YamlEventSink forwards end sequence event",
          "[job_yaml][event_sink][sequence]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::endSequence(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::EndSequence);
    REQUIRE(sink.keyValue.empty());
    REQUIRE(sink.value.empty());
}

// ========================================
// Operation routing
// ========================================

TEST_CASE("YamlEventSink distinguishes borrowed and owned scalar routes",
          "[job_yaml][event_sink][routing][scalar]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::scalar(sink, "borrowed"));
    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.value == "borrowed");

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"owned"}));
    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.value == "owned");
}

TEST_CASE("YamlEventSink distinguishes borrowed and owned key routes",
          "[job_yaml][event_sink][routing][key]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::key(sink, "borrowed"));
    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);
    REQUIRE(sink.keyValue == "borrowed");

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{"owned"}));
    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue == "owned");
}

TEST_CASE("YamlEventSink routes mapping boundaries to consumer",
          "[job_yaml][event_sink][routing][mapping]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::BeginMapping);

    REQUIRE(YamlEventSink::endMapping(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
}

TEST_CASE("YamlEventSink routes sequence boundaries to consumer",
          "[job_yaml][event_sink][routing][sequence]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginSequence(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::BeginSequence);

    REQUIRE(YamlEventSink::endSequence(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::EndSequence);
}

// ========================================
// Canonical parser event streams
// ========================================

TEST_CASE("YamlEventSink forwards borrowed mapping scalar event stream",
          "[job_yaml][event_sink][stream][mapping][borrowed]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::BeginMapping);

    REQUIRE(YamlEventSink::key(sink, "name"));
    REQUIRE(sink.operation == EventSinkFixture::Operation::Key);

    REQUIRE(YamlEventSink::scalar(sink, "Joseph"));
    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);
    REQUIRE(sink.value == "Joseph");

    REQUIRE(YamlEventSink::endMapping(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
}

TEST_CASE("YamlEventSink forwards owned mapping scalar event stream",
          "[job_yaml][event_sink][stream][mapping][owned]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{"name"}));
    REQUIRE(sink.operation == EventSinkFixture::Operation::KeyOwned);
    REQUIRE(sink.keyValue == "name");

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"Joseph"}));
    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);
    REQUIRE(sink.value == "Joseph");

    REQUIRE(YamlEventSink::endMapping(sink));
}

TEST_CASE("YamlEventSink forwards mixed ownership mapping event stream",
          "[job_yaml][event_sink][stream][mapping][ownership]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));

    REQUIRE(YamlEventSink::key(sink, "plain"));
    REQUIRE(YamlEventSink::scalar(sink, "borrowed"));

    REQUIRE(YamlEventSink::keyOwned(sink, std::string{"decoded key"}));
    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"decoded value"}));

    REQUIRE(YamlEventSink::endMapping(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
}

TEST_CASE("YamlEventSink forwards nested mapping event stream",
          "[job_yaml][event_sink][stream][mapping][nested]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));
    REQUIRE(YamlEventSink::key(sink, "profile"));
    REQUIRE(YamlEventSink::beginMapping(sink));
    REQUIRE(YamlEventSink::key(sink, "name"));
    REQUIRE(YamlEventSink::scalar(sink, "Joseph"));
    REQUIRE(YamlEventSink::endMapping(sink));
    REQUIRE(YamlEventSink::endMapping(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
}

TEST_CASE("YamlEventSink forwards sequence scalar event stream",
          "[job_yaml][event_sink][stream][sequence]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginSequence(sink));

    REQUIRE(YamlEventSink::scalar(sink, "one"));
    REQUIRE(sink.operation == EventSinkFixture::Operation::Scalar);

    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"two"}));
    REQUIRE(sink.operation == EventSinkFixture::Operation::ScalarOwned);

    REQUIRE(YamlEventSink::endSequence(sink));
    REQUIRE(sink.operation == EventSinkFixture::Operation::EndSequence);
}

TEST_CASE("YamlEventSink forwards mapping containing sequence event stream",
          "[job_yaml][event_sink][stream][mapping][sequence]")
{
    EventSinkFixture sink;

    REQUIRE(YamlEventSink::beginMapping(sink));
    REQUIRE(YamlEventSink::key(sink, "items"));
    REQUIRE(YamlEventSink::beginSequence(sink));
    REQUIRE(YamlEventSink::scalar(sink, "one"));
    REQUIRE(YamlEventSink::scalarOwned(sink, std::string{"two"}));
    REQUIRE(YamlEventSink::endSequence(sink));
    REQUIRE(YamlEventSink::endMapping(sink));

    REQUIRE(sink.operation == EventSinkFixture::Operation::EndMapping);
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlEventSink borrowed scalar benchmark",
          "[job_yaml][event_sink][benchmark]")
{
    const std::string_view value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlEventSink borrowed scalar")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(value);

        const bool result = YamlEventSink::scalar(sink, value);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };

    BENCHMARK("direct borrowed event scalar")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(value);

        const bool result = sink.scalar(value);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };
}

TEST_CASE("YamlEventSink owned scalar benchmark",
          "[job_yaml][event_sink][benchmark]")
{
    const std::string value = "Joseph";

    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlEventSink owned scalar")
    {
        EventSinkFixture sink;
        std::string copy = value;

        benchmarkDoNotOptimize(copy);

        const bool result = YamlEventSink::scalarOwned(sink, std::move(copy));

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };

    BENCHMARK("direct owned event scalar")
    {
        EventSinkFixture sink;
        std::string copy = value;

        benchmarkDoNotOptimize(copy);

        const bool result = sink.scalarOwned(std::move(copy));

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };
}

TEST_CASE("YamlEventSink borrowed key benchmark",
          "[job_yaml][event_sink][benchmark]")
{
    const std::string_view key = "name";

    benchmarkDoNotOptimize(key);

    BENCHMARK("YamlEventSink borrowed key")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(key);

        const bool result = YamlEventSink::key(sink, key);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.keyValue.size();
    };

    BENCHMARK("direct borrowed event key")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(key);

        const bool result = sink.key(key);

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.keyValue.size();
    };
}

TEST_CASE("YamlEventSink mapping stream benchmark",
          "[job_yaml][event_sink][mapping][benchmark]")
{
    const std::string_view key = "name";
    const std::string_view value = "Joseph";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlEventSink mapping scalar stream")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        bool result = YamlEventSink::beginMapping(sink);
        result = YamlEventSink::key(sink, key) && result;
        result = YamlEventSink::scalar(sink, value) && result;
        result = YamlEventSink::endMapping(sink) && result;

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };

    BENCHMARK("direct mapping scalar stream")
    {
        EventSinkFixture sink;

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        bool result = sink.beginMapping();
        result = sink.key(key) && result;
        result = sink.scalar(value) && result;
        result = sink.endMapping() && result;

        benchmarkClobber(sink);
        benchmarkDoNotOptimize(result);

        return sink.value.size();
    };
}

#endif

} // namespace job::yaml::tests
