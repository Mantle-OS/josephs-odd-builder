#include <catch2/catch_test_macros.hpp>
#ifdef JOB_TEST_BENCHMARKS
    #include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include <job_yaml_emitter.h>
#include <job_yaml_scalar_kernel.h>

#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"
#include "../tests-fast-math-workaround.h"

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Sink concept
// ========================================

static_assert(YamlOutputSink<std::string>);
static_assert(YamlOutputSink<FixedStringSink>);

// ========================================
// Scalar bool
// ========================================

TEST_CASE("YamlEmitter emits true scalar", "[job_yaml][emitter]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(true, output));
    REQUIRE(output == "true");
}

TEST_CASE("YamlEmitter emits false scalar", "[job_yaml][emitter]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(false, output));
    REQUIRE(output == "false");
}

// ========================================
// Scalar integers
// ========================================

TEST_CASE("YamlEmitter emits signed integer scalars", "[job_yaml][emitter]")
{
    SECTION("zero")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(0, output));
        REQUIRE(output == "0");
    }

    SECTION("positive")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(42, output));
        REQUIRE(output == "42");
    }

    SECTION("negative")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(-42, output));
        REQUIRE(output == "-42");
    }
}

TEST_CASE("YamlEmitter emits signed integer boundaries", "[job_yaml][emitter]")
{
    SECTION("minimum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::int32_t>::min(), output));
        REQUIRE(output == "-2147483648");
    }

    SECTION("maximum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::int32_t>::max(), output));
        REQUIRE(output == "2147483647");
    }
}

TEST_CASE("YamlEmitter emits unsigned integer boundaries", "[job_yaml][emitter]")
{
    SECTION("zero")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::uint32_t{0}, output));
        REQUIRE(output == "0");
    }

    SECTION("maximum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::uint32_t>::max(), output));
        REQUIRE(output == "4294967295");
    }
}

TEST_CASE("YamlEmitter emits 64 bit integer boundaries", "[job_yaml][emitter]")
{
    SECTION("signed minimum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::int64_t>::min(), output));
        REQUIRE(output == "-9223372036854775808");
    }

    SECTION("signed maximum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::int64_t>::max(), output));
        REQUIRE(output == "9223372036854775807");
    }

    SECTION("unsigned maximum")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(std::numeric_limits<std::uint64_t>::max(), output));
        REQUIRE(output == "18446744073709551615");
    }
}

// ========================================
// Scalar floating point
// ========================================

TEST_CASE("YamlEmitter emits ordinary float scalars", "[job_yaml][emitter]")
{
    SECTION("zero")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(0.0F, output));
        REQUIRE(output == "0");
    }

    SECTION("positive")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(1.5F, output));
        REQUIRE(output == "1.5");
    }

    SECTION("negative")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(-4.25F, output));
        REQUIRE(output == "-4.25");
    }
}

TEST_CASE("YamlEmitter emits ordinary double scalars", "[job_yaml][emitter]")
{
    SECTION("positive")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(2.25, output));
        REQUIRE(output == "2.25");
    }

    SECTION("scientific")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(1.0e20, output));
        REQUIRE_FALSE(output.empty());
    }
}

// ========================================
// Special floating point values
// ========================================

TEST_CASE("YamlEmitter emits positive infinity using YAML spelling",
          "[job_yaml][emitter][fast_math]")
{
    SECTION("float")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeInfinity<float>(), output));
        REQUIRE(output == ".inf");
    }

    SECTION("double")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeInfinity<double>(), output));
        REQUIRE(output == ".inf");
    }
}

TEST_CASE("YamlEmitter emits negative infinity using YAML spelling",
          "[job_yaml][emitter][fast_math]")
{
    SECTION("float")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeNegativeInfinity<float>(), output));
        REQUIRE(output == "-.inf");
    }

    SECTION("double")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeNegativeInfinity<double>(), output));
        REQUIRE(output == "-.inf");
    }
}

TEST_CASE("YamlEmitter emits NaN using YAML spelling",
          "[job_yaml][emitter][fast_math]")
{
    SECTION("float")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeNaN<float>(), output));
        REQUIRE(output == ".nan");
    }

    SECTION("double")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(safeNaN<double>(), output));
        REQUIRE(output == ".nan");
    }
}

// ========================================
// Scalar strings
// ========================================

TEST_CASE("YamlEmitter always double quotes string scalar", "[job_yaml][emitter]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(std::string{"Joseph"}, output));
    REQUIRE(output == "\"Joseph\"");
}

TEST_CASE("YamlEmitter emits string_view scalar", "[job_yaml][emitter]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar("JOB YAML"sv, output));
    REQUIRE(output == "\"JOB YAML\"");
}

TEST_CASE("YamlEmitter emits empty string scalar", "[job_yaml][emitter]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(std::string_view{}, output));
    REQUIRE(output == "\"\"");
}

// ========================================
// Named string escapes
// ========================================

TEST_CASE("YamlEmitter escapes YAML named control characters", "[job_yaml][emitter]")
{
    struct TestCase
    {
        char input;
        std::string_view expected;
    };

    constexpr TestCase cases[] = {
                                  {'\0', "\"\\0\""sv},
                                  {'\a', "\"\\a\""sv},
                                  {'\b', "\"\\b\""sv},
                                  {'\t', "\"\\t\""sv},
                                  {'\n', "\"\\n\""sv},
                                  {'\v', "\"\\v\""sv},
                                  {'\f', "\"\\f\""sv},
                                  {'\r', "\"\\r\""sv},
                                  {static_cast<char>(0x1B), "\"\\e\""sv},
                                  };

    for (const auto &test : cases) {
        std::string output;
        const std::string_view input{&test.input, 1};

        REQUIRE(YamlEmitter::emitScalar(input, output));
        REQUIRE(output == test.expected);
    }
}

TEST_CASE("YamlEmitter escapes quote and backslash", "[job_yaml][emitter]")
{
    SECTION("double quote")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar("\""sv, output));
        REQUIRE(output == "\"\\\"\"");
    }

    SECTION("backslash")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar("\\"sv, output));
        REQUIRE(output == "\"\\\\\"");
    }

    SECTION("combined")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar("a\"b\\c"sv, output));
        REQUIRE(output == "\"a\\\"b\\\\c\"");
    }
}

// ========================================
// Generic control-byte escaping
// ========================================

TEST_CASE("YamlEmitter emits unnamed control bytes as hex escapes", "[job_yaml][emitter]")
{
    struct TestCase
    {
        unsigned char input;
        std::string_view expected;
    };

    constexpr TestCase cases[] = {
                                  {0x01, "\"\\x01\""sv},
                                  {0x02, "\"\\x02\""sv},
                                  {0x06, "\"\\x06\""sv},
                                  {0x0E, "\"\\x0E\""sv},
                                  {0x1F, "\"\\x1F\""sv},
                                  {0x7F, "\"\\x7F\""sv},
                                  };

    for (const auto &test : cases) {
        const char input = static_cast<char>(test.input);
        const std::string_view value{&input, 1};
        std::string output;

        REQUIRE(YamlEmitter::emitScalar(value, output));
        REQUIRE(output == test.expected);
    }
}

// ========================================
// Embedded null and binary-safe input
// ========================================

TEST_CASE("YamlEmitter preserves string_view length across embedded null",
          "[job_yaml][emitter]")
{
    constexpr char input[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    std::string output;

    REQUIRE(YamlEmitter::emitScalar(std::string_view{input, sizeof(input)}, output));
    REQUIRE(output == "\"JOB\\0YAML\"");
}

// ========================================
// UTF-8 preservation
// ========================================

TEST_CASE("YamlEmitter preserves UTF-8 bytes in strings", "[job_yaml][emitter]")
{
    constexpr std::string_view input = "Joseph € 😀";

    std::string output;

    REQUIRE(YamlEmitter::emitScalar(input, output));
    REQUIRE(output == "\"Joseph € 😀\"");
}

TEST_CASE("YamlEmitter does not escape ordinary high UTF-8 bytes individually",
          "[job_yaml][emitter]")
{
    constexpr char input[] = {
        static_cast<char>(0xE2),
        static_cast<char>(0x82),
        static_cast<char>(0xAC)
    };

    std::string output;

    REQUIRE(YamlEmitter::emitScalar(std::string_view{input, sizeof(input)}, output));

    REQUIRE(output.size() == 5);
    REQUIRE(output.front() == '"');
    REQUIRE(static_cast<unsigned char>(output[1]) == 0xE2);
    REQUIRE(static_cast<unsigned char>(output[2]) == 0x82);
    REQUIRE(static_cast<unsigned char>(output[3]) == 0xAC);
    REQUIRE(output.back() == '"');
}

// ========================================
// Existing sink content
// ========================================

TEST_CASE("YamlEmitter appends scalar to existing sink content", "[job_yaml][emitter]")
{
    std::string output = "prefix:";

    REQUIRE(YamlEmitter::emitScalar(42, output));
    REQUIRE(output == "prefix:42");
}

// ========================================
// Fixed no-allocation sink
// ========================================

TEST_CASE("YamlEmitter emits scalar into FixedStringSink", "[job_yaml][emitter]")
{
    FixedStringSink output;

    REQUIRE(YamlEmitter::emitScalar(42, output));
    REQUIRE(output.view() == "42");
}

TEST_CASE("YamlEmitter emits escaped string into FixedStringSink", "[job_yaml][emitter]")
{
    FixedStringSink output;

    REQUIRE(YamlEmitter::emitScalar("hello\nworld"sv, output));
    REQUIRE(output.view() == "\"hello\\nworld\"");
}

// ========================================
// Object output
// ========================================

TEST_CASE("YamlEmitter emits reflected object in declaration order",
          "[job_yaml][emitter]")
{
    const EmitterObject object{
        .count = 42,
        .size = 1024,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "Joseph",
    };

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));

    REQUIRE(output ==
            "count: 42\n"
            "size: 1024\n"
            "enabled: true\n"
            "scale: 1.5\n"
            "ratio: 2.25\n"
            "name: \"Joseph\"\n");
}

TEST_CASE("YamlEmitter object output ends with newline", "[job_yaml][emitter]")
{
    const EmitterObject object{};

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));
    REQUIRE_FALSE(output.empty());
    REQUIRE(output.back() == '\n');
}

TEST_CASE("YamlEmitter emits reflected object into FixedStringSink",
          "[job_yaml][emitter]")
{
    const EmitterObject object{
        .count = 42,
        .size = 10,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "JOB",
    };

    FixedStringSink output;

    REQUIRE(YamlEmitter::emitObject(object, output));

    REQUIRE(output.view() ==
            "count: 42\n"
            "size: 10\n"
            "enabled: true\n"
            "scale: 1.5\n"
            "ratio: 2.25\n"
            "name: \"JOB\"\n");
}

TEST_CASE("YamlEmitter object string member is escaped", "[job_yaml][emitter]")
{
    const EmitterObject object{
        .name = "hello\n\"JOB\"\\yaml",
    };

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));
    REQUIRE(output.find("name: \"hello\\n\\\"JOB\\\"\\\\yaml\"\n") != std::string::npos);
}

TEST_CASE("YamlEmitter emits special floating values in reflected object",
          "[job_yaml][emitter][fast_math]")
{
    EmitterObject object{};
    object.scale = safeInfinity<float>();
    object.ratio = safeNaN<double>();

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));
    REQUIRE(output.find("scale: .inf\n") != std::string::npos);
    REQUIRE(output.find("ratio: .nan\n") != std::string::npos);
}

// ========================================
// Object output appends
// ========================================

TEST_CASE("YamlEmitter appends object to existing sink", "[job_yaml][emitter]")
{
    const EmitterObject object{
        .count = 1,
    };

    std::string output = "---\n";

    REQUIRE(YamlEmitter::emitObject(object, output));
    REQUIRE(output.starts_with("---\n"));
    REQUIRE(output.find("count: 1\n") != std::string::npos);
}

// ========================================
// Round-trip scalar compatibility
// ========================================

TEST_CASE("YamlEmitter integer output can be read by YamlScalarKernel",
          "[job_yaml][emitter][roundtrip]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(-12345, output));

    int value{};
    REQUIRE(YamlScalarKernel::parseInteger(output, value));
    REQUIRE(value == -12345);
}

TEST_CASE("YamlEmitter float output can be read by YamlScalarKernel",
          "[job_yaml][emitter][roundtrip]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(2.25, output));

    double value{};
    REQUIRE(YamlScalarKernel::parseFloat(output, value));
    REQUIRE(value == 2.25);
}

TEST_CASE("YamlEmitter infinity output can be read by YamlScalarKernel",
          "[job_yaml][emitter][roundtrip]")
{
    std::string output;

    REQUIRE(YamlEmitter::emitScalar(safeNegativeInfinity<double>(), output));
    REQUIRE(output == "-.inf");

    double value{};

    REQUIRE(YamlScalarKernel::parseFloat(output, value));
    REQUIRE(isSafeNegativeInfinity(value));
}


// Pointers and what not
// ========================================
// Null emission
// ========================================

TEST_CASE("YamlEmitter emits null YamlNode", "[job_yaml][emitter][null]")
{
    YamlNode node;
    node.setNull();

    std::string output;

    REQUIRE(YamlEmitter::emitNode(node, output));
    REQUIRE(output == "null");
}

TEST_CASE("YamlEmitter distinguishes null node from string null",
          "[job_yaml][emitter][null][string]")
{
    SECTION("null node")
    {
        YamlNode node;
        node.setNull();

        std::string output;

        REQUIRE(YamlEmitter::emitNode(node, output));
        REQUIRE(output == "null");
    }

    SECTION("string null")
    {
        std::string output;

        REQUIRE(YamlEmitter::emitScalar("null"sv, output));
        REQUIRE(output == "\"null\"");
    }
}

// ========================================
// Optional emission
// ========================================

TEST_CASE("YamlEmitter emits empty optional as null",
          "[job_yaml][emitter][optional][null]")
{
    const std::optional<int> value;

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("YamlEmitter emits populated optional value",
          "[job_yaml][emitter][optional]")
{
    const std::optional<int> value{42};

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "42");
}

// ========================================
// Pointer emission
// ========================================

TEST_CASE("YamlEmitter emits null shared pointer as null",
          "[job_yaml][emitter][pointer][shared_pointer][null]")
{
    const std::shared_ptr<int> value;

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("YamlEmitter emits populated shared pointer value",
          "[job_yaml][emitter][pointer][shared_pointer]")
{
    const auto value = std::make_shared<int>(42);

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("YamlEmitter emits null unique pointer as null",
          "[job_yaml][emitter][pointer][unique_pointer][null]")
{
    const std::unique_ptr<int> value;

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("YamlEmitter emits populated unique pointer value",
          "[job_yaml][emitter][pointer][unique_pointer]")
{
    const auto value = std::make_unique<int>(42);

    std::string output;

    REQUIRE(YamlEmitter::emitValue(value, output));
    REQUIRE(output == "42");
}

// ========================================
// Nullable reflected object members
// ========================================

TEST_CASE("YamlEmitter emits empty nullable reflected members as null",
          "[job_yaml][emitter][object][null]")
{
    const EmitterNullableObject object{};

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));

    REQUIRE(output ==
            "optionalValue: null\n"
            "sharedValue: null\n"
            "uniqueValue: null\n");
}

TEST_CASE("YamlEmitter emits populated nullable reflected members",
          "[job_yaml][emitter][object][optional][pointer]")
{
    EmitterNullableObject object{
        .optionalValue = 42,
        .sharedValue = std::make_shared<int>(43),
        .uniqueValue = std::make_unique<int>(44),
    };

    std::string output;

    REQUIRE(YamlEmitter::emitObject(object, output));

    REQUIRE(output ==
            "optionalValue: 42\n"
            "sharedValue: 43\n"
            "uniqueValue: 44\n");
}

// ========================================
// Arbitrary mapping key emission
// ========================================

TEST_CASE("YamlEmitter safely quotes mapping keys",
          "[job_yaml][emitter][node][mapping][string]")
{
    YamlNode node;
    node.setMapping();

    YamlNode value;
    value.setScalar("value");

    node.setMember("key: # [danger]", std::move(value));

    std::string output;

    REQUIRE(YamlEmitter::emitNode(node, output));
    REQUIRE(output == "\"key: # [danger]\": \"value\"\n");
}

TEST_CASE("YamlEmitter safely escapes mapping keys",
          "[job_yaml][emitter][node][mapping][string]")
{
    YamlNode node;
    node.setMapping();

    YamlNode value;
    value.setScalar("value");

    node.setMember("hello\n\"JOB\"\\yaml", std::move(value));

    std::string output;

    REQUIRE(YamlEmitter::emitNode(node, output));
    REQUIRE(output == "\"hello\\n\\\"JOB\\\"\\\\yaml\": \"value\"\n");
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlEmitter scalar benchmarks", "[job_yaml][emitter][benchmark]")
{
    std::int32_t integerValue = 42;
    std::string_view stringValue = "Joseph";

    benchmarkDoNotOptimize(integerValue);
    benchmarkDoNotOptimize(stringValue);

    BENCHMARK("YamlEmitter integer")
    {
        FixedStringSink output;

        benchmarkDoNotOptimize(integerValue);

        const bool result = YamlEmitter::emitScalar(integerValue, output);

        benchmarkClobber(output);
        benchmarkDoNotOptimize(result);

        return output.used;
    };

    BENCHMARK("direct to_chars integer")
    {
        char buffer[64];

        benchmarkDoNotOptimize(integerValue);

        const auto [ptr, ec] =
            std::to_chars(buffer, buffer + sizeof(buffer), integerValue);

        benchmarkDoNotOptimize(ptr);
        benchmarkDoNotOptimize(ec);

        return static_cast<std::size_t>(ptr - buffer);
    };

    BENCHMARK("YamlEmitter string")
    {
        FixedStringSink output;

        benchmarkDoNotOptimize(stringValue);

        const bool result = YamlEmitter::emitScalar(stringValue, output);

        benchmarkClobber(output);
        benchmarkDoNotOptimize(result);

        return output.used;
    };
}

TEST_CASE("YamlEmitter complete object benchmark",
          "[job_yaml][emitter][benchmark]")
{
    EmitterObject object{
        .count = 42,
        .size = 1024,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "Joseph",
    };

    benchmarkDoNotOptimize(object);

    BENCHMARK("YamlEmitter six member object")
    {
        FixedStringSink output;

        benchmarkDoNotOptimize(object);

        const bool result = YamlEmitter::emitObject(object, output);

        benchmarkClobber(output);
        benchmarkDoNotOptimize(result);

        return output.used;
    };

    BENCHMARK("manual six member object")
    {
        FixedStringSink output;

        benchmarkDoNotOptimize(object);

        char buffer[128];

        output.append("count: ");
        auto [ptr1, ec1] = std::to_chars(buffer, buffer + sizeof(buffer), object.count);
        output.append(std::string_view{buffer, static_cast<std::size_t>(ptr1 - buffer)});
        output.push_back('\n');

        output.append("size: ");
        auto [ptr2, ec2] = std::to_chars(buffer, buffer + sizeof(buffer), object.size);
        output.append(std::string_view{buffer, static_cast<std::size_t>(ptr2 - buffer)});
        output.push_back('\n');

        output.append("enabled: ");
        output.append(object.enabled ? "true" : "false");
        output.push_back('\n');

        output.append("scale: ");
        auto [ptr3, ec3] =
            std::to_chars(buffer, buffer + sizeof(buffer), object.scale, std::chars_format::general);
        output.append(std::string_view{buffer, static_cast<std::size_t>(ptr3 - buffer)});
        output.push_back('\n');

        output.append("ratio: ");
        auto [ptr4, ec4] =
            std::to_chars(buffer, buffer + sizeof(buffer), object.ratio, std::chars_format::general);
        output.append(std::string_view{buffer, static_cast<std::size_t>(ptr4 - buffer)});
        output.push_back('\n');

        output.append("name: \"");
        output.append(object.name);
        output.append("\"\n");

        benchmarkDoNotOptimize(ec1);
        benchmarkDoNotOptimize(ec2);
        benchmarkDoNotOptimize(ec3);
        benchmarkDoNotOptimize(ec4);
        benchmarkClobber(output);

        return output.used;
    };
}

#endif

} // namespace job::yaml::tests