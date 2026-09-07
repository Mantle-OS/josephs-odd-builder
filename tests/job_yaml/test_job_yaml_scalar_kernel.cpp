#include <catch2/catch_test_macros.hpp>

#include <job_yaml_scalar_kernel.h>

#include "../tests-fast-math-workaround.h"
#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

enum class ScalarKernelEnum : std::uint8_t {
    Zero = 0,
    One = 1,
    Two = 2
};

// ========================================
// Compile-time behavior
// ========================================

static_assert(YamlScalarKernel::isNull("~"sv));
static_assert(YamlScalarKernel::isNull("null"sv));
static_assert(YamlScalarKernel::isNull("Null"sv));
static_assert(YamlScalarKernel::isNull("NULL"sv));
static_assert(!YamlScalarKernel::isNull(""sv));
static_assert(!YamlScalarKernel::isNull("none"sv));

static_assert(YamlScalarKernel::isBoolean("true"sv));
static_assert(YamlScalarKernel::isBoolean("True"sv));
static_assert(YamlScalarKernel::isBoolean("TRUE"sv));
static_assert(YamlScalarKernel::isBoolean("false"sv));
static_assert(YamlScalarKernel::isBoolean("False"sv));
static_assert(YamlScalarKernel::isBoolean("FALSE"sv));
static_assert(!YamlScalarKernel::isBoolean("yes"sv));
static_assert(!YamlScalarKernel::isBoolean("no"sv));

constexpr bool constexprBoolTest()
{
    bool value = false;

    if (!YamlScalarKernel::parseBool("true"sv, value) || !value)
        return false;

    if (!YamlScalarKernel::parseBool("FALSE"sv, value) || value)
        return false;

    return !YamlScalarKernel::parseBool("yes"sv, value);
}

static_assert(constexprBoolTest());

constexpr bool constexprIntegerTest()
{
    std::int32_t signedValue{};
    std::uint32_t unsignedValue{};

    if (!YamlScalarKernel::parseInteger("42"sv, signedValue) || signedValue != 42)
        return false;

    if (!YamlScalarKernel::parseInteger("-42"sv, signedValue) || signedValue != -42)
        return false;

    if (!YamlScalarKernel::parseInteger("0xFF"sv, unsignedValue) || unsignedValue != 255)
        return false;

    if (!YamlScalarKernel::parseInteger("0o755"sv, unsignedValue) || unsignedValue != 493)
        return false;

    return true;
}

static_assert(constexprIntegerTest());

constexpr bool constexprGenericParseTest()
{
    bool boolean{};
    std::int32_t integer{};
    ScalarKernelEnum enumeration{};

    if (!YamlScalarKernel::parse("true"sv, boolean) || !boolean)
        return false;

    if (!YamlScalarKernel::parse("-42"sv, integer) || integer != -42)
        return false;

    if (!YamlScalarKernel::parse("2"sv, enumeration) || enumeration != ScalarKernelEnum::Two)
        return false;

    return true;
}

static_assert(constexprGenericParseTest());

// ========================================
// Classification
// ========================================

TEST_CASE("YamlScalarKernel classifies null scalars", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::classify("~") == YamlScalarKind::Null);
    REQUIRE(YamlScalarKernel::classify("null") == YamlScalarKind::Null);
    REQUIRE(YamlScalarKernel::classify("Null") == YamlScalarKind::Null);
    REQUIRE(YamlScalarKernel::classify("NULL") == YamlScalarKind::Null);
}

TEST_CASE("YamlScalarKernel classifies boolean scalars", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::classify("true") == YamlScalarKind::Boolean);
    REQUIRE(YamlScalarKernel::classify("True") == YamlScalarKind::Boolean);
    REQUIRE(YamlScalarKernel::classify("TRUE") == YamlScalarKind::Boolean);

    REQUIRE(YamlScalarKernel::classify("false") == YamlScalarKind::Boolean);
    REQUIRE(YamlScalarKernel::classify("False") == YamlScalarKind::Boolean);
    REQUIRE(YamlScalarKernel::classify("FALSE") == YamlScalarKind::Boolean);
}

TEST_CASE("YamlScalarKernel classifies integer scalars", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::classify("0") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("42") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("+42") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("-42") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("0xFF") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("0Xff") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("0o755") == YamlScalarKind::Integer);
    REQUIRE(YamlScalarKernel::classify("0O755") == YamlScalarKind::Integer);
}

TEST_CASE("YamlScalarKernel classifies floating point scalars", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::classify("1.0") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify("-1.5") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify("1e3") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify("1E-3") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify(".inf") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify("-.INF") == YamlScalarKind::FloatingPoint);
    REQUIRE(YamlScalarKernel::classify(".nan") == YamlScalarKind::FloatingPoint);
}

TEST_CASE("YamlScalarKernel leaves ordinary text as strings", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::classify("") == YamlScalarKind::String);
    REQUIRE(YamlScalarKernel::classify("hello") == YamlScalarKind::String);
    REQUIRE(YamlScalarKernel::classify("yes") == YamlScalarKind::String);
    REQUIRE(YamlScalarKernel::classify("no") == YamlScalarKind::String);
    REQUIRE(YamlScalarKernel::classify("12abc") == YamlScalarKind::String);
}

// ========================================
// Null
// ========================================

TEST_CASE("YamlScalarKernel recognizes only supported null spellings", "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::isNull("~"));
    REQUIRE(YamlScalarKernel::isNull("null"));
    REQUIRE(YamlScalarKernel::isNull("Null"));
    REQUIRE(YamlScalarKernel::isNull("NULL"));

    REQUIRE_FALSE(YamlScalarKernel::isNull(""));
    REQUIRE_FALSE(YamlScalarKernel::isNull("nil"));
    REQUIRE_FALSE(YamlScalarKernel::isNull("none"));
    REQUIRE_FALSE(YamlScalarKernel::isNull("NULL "));
    REQUIRE_FALSE(YamlScalarKernel::isNull(" null"));
}

// ========================================
// Boolean
// ========================================

TEST_CASE("YamlScalarKernel parses true boolean values", "[job_yaml][scalar_kernel]")
{
    for (const std::string_view text : {"true"sv, "True"sv, "TRUE"sv}) {
        bool value = false;

        REQUIRE(YamlScalarKernel::parseBool(text, value));
        REQUIRE(value);
    }
}

TEST_CASE("YamlScalarKernel parses false boolean values", "[job_yaml][scalar_kernel]")
{
    for (const std::string_view text : {"false"sv, "False"sv, "FALSE"sv}) {
        bool value = true;

        REQUIRE(YamlScalarKernel::parseBool(text, value));
        REQUIRE_FALSE(value);
    }
}

TEST_CASE("YamlScalarKernel rejects invalid boolean values without changing destination",
          "[job_yaml][scalar_kernel]")
{
    for (const std::string_view text : {""sv, "yes"sv, "no"sv, "TRUE "sv, " true"sv, "1"sv}) {
        bool value = true;

        REQUIRE_FALSE(YamlScalarKernel::parseBool(text, value));
        REQUIRE(value);
    }
}

// ========================================
// Signed integers
// ========================================

TEST_CASE("YamlScalarKernel parses signed decimal integers", "[job_yaml][scalar_kernel]")
{
    const ScalarCase<std::int64_t> cases[] = {
                                              {"0", 0, true},
                                              {"42", 42, true},
                                              {"+42", 42, true},
                                              {"-42", -42, true},
                                              {"2147483647", 2147483647LL, true},
                                              {"-2147483648", -2147483648LL, true},
                                              };

    for (const auto &test : cases) {
        std::int64_t value{};

        REQUIRE(YamlScalarKernel::parseInteger(test.text, value) == test.valid);

        if (test.valid)
            REQUIRE(value == test.expected);
    }
}

TEST_CASE("YamlScalarKernel parses hexadecimal integers", "[job_yaml][scalar_kernel]")
{
    const ScalarCase<std::uint32_t> cases[] = {
                                               {"0x0", 0, true},
                                               {"0x1", 1, true},
                                               {"0xFF", 255, true},
                                               {"0xff", 255, true},
                                               {"0XFF", 255, true},
                                               {"0XdeadBEEF", 0xDEADBEEF, true},
                                               };

    for (const auto &test : cases) {
        std::uint32_t value{};

        REQUIRE(YamlScalarKernel::parseInteger(test.text, value) == test.valid);

        if (test.valid)
            REQUIRE(value == test.expected);
    }
}

TEST_CASE("YamlScalarKernel parses octal integers", "[job_yaml][scalar_kernel]")
{
    const ScalarCase<std::uint32_t> cases[] = {
                                               {"0o0", 0, true},
                                               {"0o7", 7, true},
                                               {"0o10", 8, true},
                                               {"0o755", 493, true},
                                               {"0O755", 493, true},
                                               };

    for (const auto &test : cases) {
        std::uint32_t value{};

        REQUIRE(YamlScalarKernel::parseInteger(test.text, value) == test.valid);

        if (test.valid)
            REQUIRE(value == test.expected);
    }
}

// ========================================
// Integer destination boundaries
// ========================================

TEST_CASE("YamlScalarKernel parses signed integer minimum and maximum values",
          "[job_yaml][scalar_kernel]")
{
    SECTION("int8")
    {
        std::int8_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("127", value));
        REQUIRE(value == std::numeric_limits<std::int8_t>::max());

        REQUIRE(YamlScalarKernel::parseInteger("-128", value));
        REQUIRE(value == std::numeric_limits<std::int8_t>::min());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("128", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-129", value));
    }

    SECTION("int16")
    {
        std::int16_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("32767", value));
        REQUIRE(value == std::numeric_limits<std::int16_t>::max());

        REQUIRE(YamlScalarKernel::parseInteger("-32768", value));
        REQUIRE(value == std::numeric_limits<std::int16_t>::min());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("32768", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-32769", value));
    }

    SECTION("int32")
    {
        std::int32_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("2147483647", value));
        REQUIRE(value == std::numeric_limits<std::int32_t>::max());

        REQUIRE(YamlScalarKernel::parseInteger("-2147483648", value));
        REQUIRE(value == std::numeric_limits<std::int32_t>::min());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("2147483648", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-2147483649", value));
    }

    SECTION("int64")
    {
        std::int64_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("9223372036854775807", value));
        REQUIRE(value == std::numeric_limits<std::int64_t>::max());

        REQUIRE(YamlScalarKernel::parseInteger("-9223372036854775808", value));
        REQUIRE(value == std::numeric_limits<std::int64_t>::min());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("9223372036854775808", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-9223372036854775809", value));
    }
}

TEST_CASE("YamlScalarKernel parses unsigned integer boundaries", "[job_yaml][scalar_kernel]")
{
    SECTION("uint8")
    {
        std::uint8_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("0", value));
        REQUIRE(value == 0);

        REQUIRE(YamlScalarKernel::parseInteger("255", value));
        REQUIRE(value == std::numeric_limits<std::uint8_t>::max());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("256", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-1", value));
    }

    SECTION("uint16")
    {
        std::uint16_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("65535", value));
        REQUIRE(value == std::numeric_limits<std::uint16_t>::max());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("65536", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-1", value));
    }

    SECTION("uint32")
    {
        std::uint32_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("4294967295", value));
        REQUIRE(value == std::numeric_limits<std::uint32_t>::max());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("4294967296", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-1", value));
    }

    SECTION("uint64")
    {
        std::uint64_t value{};

        REQUIRE(YamlScalarKernel::parseInteger("18446744073709551615", value));
        REQUIRE(value == std::numeric_limits<std::uint64_t>::max());

        REQUIRE_FALSE(YamlScalarKernel::parseInteger("18446744073709551616", value));
        REQUIRE_FALSE(YamlScalarKernel::parseInteger("-1", value));
    }
}

// ========================================
// Invalid integers
// ========================================

TEST_CASE("YamlScalarKernel rejects malformed integers", "[job_yaml][scalar_kernel]")
{
    const std::string_view cases[] = {
        "",
        "+",
        "-",
        "12abc",
        "12.3",
        "0x",
        "0X",
        "0o",
        "0O",
        "0xGG",
        "0o89",
        " 42",
        "42 ",
        "--42",
        "++42",
        "+-42",
    };

    for (const std::string_view text : cases) {
        std::int64_t value = 123;

        REQUIRE_FALSE(YamlScalarKernel::parseInteger(text, value));
    }
}

TEST_CASE("YamlScalarKernel currently rejects numeric separators", "[job_yaml][scalar_kernel]")
{
    std::uint64_t value{};

    REQUIRE_FALSE(YamlScalarKernel::parseInteger("1_000_000", value));
    REQUIRE_FALSE(YamlScalarKernel::parseInteger("0xFF_FF", value));
    REQUIRE_FALSE(YamlScalarKernel::parseInteger("0o7_55", value));
}

// ========================================
// Floating point
// ========================================

TEST_CASE("YamlScalarKernel parses ordinary floating point values", "[job_yaml][scalar_kernel]")
{
    const ScalarCase<double> cases[] = {
                                        {"0.0", 0.0, true},
                                        {"1.0", 1.0, true},
                                        {"-1.0", -1.0, true},
                                        {"+1.0", 1.0, true},
                                        {"1.5", 1.5, true},
                                        {"-42.25", -42.25, true},
                                        {"1e3", 1000.0, true},
                                        {"1E3", 1000.0, true},
                                        {"1e-3", 0.001, true},
                                        {"1E+3", 1000.0, true},
                                        };

    for (const auto &test : cases) {
        double value{};

        REQUIRE(YamlScalarKernel::parseFloat(test.text, value) == test.valid);

        if (test.valid)
            REQUIRE(scalarEquals(value, test.expected));
    }
}

TEST_CASE("YamlScalarKernel parses YAML infinity values", "[job_yaml][scalar_kernel]")
{
    for (const std::string_view text : {".inf"sv, ".Inf"sv, ".INF"sv, "+.inf"sv, "+.Inf"sv, "+.INF"sv}) {
        double value{};

        REQUIRE(YamlScalarKernel::parseFloat(text, value));
        REQUIRE(isSafeInfinity(value));
        REQUIRE(isSafePositiveInfinity(value));
    }

    for (const std::string_view text : {"-.inf"sv, "-.Inf"sv, "-.INF"sv}) {
        double value{};

        REQUIRE(YamlScalarKernel::parseFloat(text, value));
        REQUIRE(isSafeInfinity(value));
        REQUIRE(isSafeNegativeInfinity(value));
    }
}

TEST_CASE("YamlScalarKernel parses YAML NaN values", "[job_yaml][scalar_kernel]")
{
    for (const std::string_view text : {".nan"sv, ".NaN"sv, ".NAN"sv}) {
        double value{};

        REQUIRE(YamlScalarKernel::parseFloat(text, value));
        REQUIRE(isSafeNaN(value));
        REQUIRE(scalarEquals(value, safeNaN<double>()));
    }
}

TEST_CASE("YamlScalarKernel parses directly into float destination", "[job_yaml][scalar_kernel]")
{
    float value{};

    REQUIRE(YamlScalarKernel::parseFloat("12.5", value));
    REQUIRE(value == 12.5F);
}

TEST_CASE("YamlScalarKernel rejects malformed floating point values", "[job_yaml][scalar_kernel]")
{
    const std::string_view cases[] = {
        "",
        "+",
        "-",
        ".",
        "hello",
        "1.2.3",
        "1e",
        "1e+",
        "1e-",
        " 1.5",
        "1.5 ",
        "nan",
        "inf",
    };

    for (const std::string_view text : cases) {
        double value = 123.0;

        REQUIRE_FALSE(YamlScalarKernel::parseFloat(text, value));
    }
}

TEST_CASE("YamlScalarKernel currently rejects floating point numeric separators",
          "[job_yaml][scalar_kernel]")
{
    double value{};

    REQUIRE_FALSE(YamlScalarKernel::parseFloat("1_000.5", value));
    REQUIRE_FALSE(YamlScalarKernel::parseFloat("1.000_001", value));
}

// ========================================
// Floating classification guard
// ========================================

TEST_CASE("YamlScalarKernel does not classify bare integers as floating point",
          "[job_yaml][scalar_kernel]")
{
    REQUIRE_FALSE(YamlScalarKernel::isFloatingPoint("0"));
    REQUIRE_FALSE(YamlScalarKernel::isFloatingPoint("42"));
    REQUIRE_FALSE(YamlScalarKernel::isFloatingPoint("-42"));
}

TEST_CASE("YamlScalarKernel recognizes floating point syntax before parsing",
          "[job_yaml][scalar_kernel]")
{
    REQUIRE(YamlScalarKernel::isFloatingPoint("1.0"));
    REQUIRE(YamlScalarKernel::isFloatingPoint("1e3"));
    REQUIRE(YamlScalarKernel::isFloatingPoint(".inf"));
    REQUIRE(YamlScalarKernel::isFloatingPoint(".nan"));

    REQUIRE_FALSE(YamlScalarKernel::isFloatingPoint("hello"));
    REQUIRE_FALSE(YamlScalarKernel::isFloatingPoint("42"));
}

// ========================================
// Generic scalar parsing
// ========================================

TEST_CASE("YamlScalarKernel generic parse dispatches boolean", "[job_yaml][scalar_kernel][parse]")
{
    bool value = false;

    REQUIRE(YamlScalarKernel::parse("true", value));
    REQUIRE(value);

    REQUIRE(YamlScalarKernel::parse("FALSE", value));
    REQUIRE_FALSE(value);
}

TEST_CASE("YamlScalarKernel generic parse dispatches signed integer", "[job_yaml][scalar_kernel][parse]")
{
    std::int32_t value{};

    REQUIRE(YamlScalarKernel::parse("-42", value));
    REQUIRE(value == -42);
}

TEST_CASE("YamlScalarKernel generic parse dispatches unsigned integer", "[job_yaml][scalar_kernel][parse]")
{
    std::uint32_t value{};

    REQUIRE(YamlScalarKernel::parse("0xFF", value));
    REQUIRE(value == 255);
}

TEST_CASE("YamlScalarKernel generic parse dispatches floating point", "[job_yaml][scalar_kernel][parse]")
{
    double value{};

    REQUIRE(YamlScalarKernel::parse("12.5", value));
    REQUIRE(value == 12.5);
}

TEST_CASE("YamlScalarKernel generic parse dispatches enum through underlying type",
          "[job_yaml][scalar_kernel][parse][enum]")
{
    ScalarKernelEnum value = ScalarKernelEnum::Zero;

    REQUIRE(YamlScalarKernel::parse("2", value));
    REQUIRE(value == ScalarKernelEnum::Two);
}

TEST_CASE("YamlScalarKernel parses enum hexadecimal spelling through underlying type",
          "[job_yaml][scalar_kernel][parse][enum]")
{
    ScalarKernelEnum value = ScalarKernelEnum::Zero;

    REQUIRE(YamlScalarKernel::parseEnum("0x2", value));
    REQUIRE(value == ScalarKernelEnum::Two);
}

TEST_CASE("YamlScalarKernel rejects negative value for unsigned enum underlying type",
          "[job_yaml][scalar_kernel][parse][enum]")
{
    ScalarKernelEnum value = ScalarKernelEnum::One;

    REQUIRE_FALSE(YamlScalarKernel::parse("-1", value));
    REQUIRE(value == ScalarKernelEnum::One);
}

TEST_CASE("YamlScalarKernel generic parse rejects unsupported destination type",
          "[job_yaml][scalar_kernel][parse]")
{
    std::string_view value = "unchanged";

    REQUIRE_FALSE(YamlScalarKernel::parse("alpha", value));
    REQUIRE(value == "unchanged");
}

// ========================================
// Direct source-slice parsing
// ========================================

TEST_CASE("YamlScalarKernel parses integer directly from source slice",
          "[job_yaml][scalar_kernel][source_view]")
{
    constexpr std::string_view source = "prefix42suffix";
    const std::string_view slice = source.substr(6, 2);

    REQUIRE(slice == "42");
    REQUIRE(slice.data() == source.data() + 6);

    std::int32_t value{};

    REQUIRE(YamlScalarKernel::parse(slice, value));
    REQUIRE(value == 42);
}

TEST_CASE("YamlScalarKernel parses boolean directly from source slice",
          "[job_yaml][scalar_kernel][source_view]")
{
    constexpr std::string_view source = "prefixtruesuffix";
    const std::string_view slice = source.substr(6, 4);

    REQUIRE(slice == "true");
    REQUIRE(slice.data() == source.data() + 6);

    bool value = false;

    REQUIRE(YamlScalarKernel::parse(slice, value));
    REQUIRE(value);
}

TEST_CASE("YamlScalarKernel parses floating point directly from source slice",
          "[job_yaml][scalar_kernel][source_view]")
{
    constexpr std::string_view source = "prefix12.5suffix";
    const std::string_view slice = source.substr(6, 4);

    REQUIRE(slice == "12.5");
    REQUIRE(slice.data() == source.data() + 6);

    double value{};

    REQUIRE(YamlScalarKernel::parse(slice, value));
    REQUIRE(value == 12.5);
}

TEST_CASE("YamlScalarKernel parses enum directly from source slice",
          "[job_yaml][scalar_kernel][source_view][enum]")
{
    constexpr std::string_view source = "prefix2suffix";
    const std::string_view slice = source.substr(6, 1);

    REQUIRE(slice == "2");
    REQUIRE(slice.data() == source.data() + 6);

    ScalarKernelEnum value = ScalarKernelEnum::Zero;

    REQUIRE(YamlScalarKernel::parse(slice, value));
    REQUIRE(value == ScalarKernelEnum::Two);
}

TEST_CASE("YamlScalarKernel source slice boundaries are respected",
          "[job_yaml][scalar_kernel][source_view]")
{
    constexpr std::string_view source = "x123y";
    const std::string_view slice = source.substr(1, 3);

    std::int32_t value{};

    REQUIRE(slice == "123");
    REQUIRE(YamlScalarKernel::parse(slice, value));
    REQUIRE(value == 123);
}

// ========================================
// Destination mutation
// ========================================

TEST_CASE("YamlScalarKernel integer failure does not overwrite destination",
          "[job_yaml][scalar_kernel]")
{
    std::int32_t value = 12345;

    REQUIRE_FALSE(YamlScalarKernel::parseInteger("not-a-number", value));
    REQUIRE(value == 12345);

    REQUIRE_FALSE(YamlScalarKernel::parseInteger("2147483648", value));
    REQUIRE(value == 12345);
}

TEST_CASE("YamlScalarKernel floating point failure does not overwrite destination",
          "[job_yaml][scalar_kernel]")
{
    double value = 123.5;

    REQUIRE_FALSE(YamlScalarKernel::parseFloat("not-a-number", value));
    REQUIRE(value == 123.5);

    REQUIRE_FALSE(YamlScalarKernel::parseFloat("1.2.3", value));
    REQUIRE(value == 123.5);
}

TEST_CASE("YamlScalarKernel generic parse failure does not overwrite destination",
          "[job_yaml][scalar_kernel][parse]")
{
    std::int32_t integer = 123;
    bool boolean = true;
    ScalarKernelEnum enumeration = ScalarKernelEnum::One;

    REQUIRE_FALSE(YamlScalarKernel::parse("invalid", integer));
    REQUIRE(integer == 123);

    REQUIRE_FALSE(YamlScalarKernel::parse("invalid", boolean));
    REQUIRE(boolean);

    REQUIRE_FALSE(YamlScalarKernel::parse("invalid", enumeration));
    REQUIRE(enumeration == ScalarKernelEnum::One);
}

TEST_CASE("YamlScalarKernel parses integer spelling into floating destination",
          "[job_yaml][scalar_kernel][float]")
{
    SECTION("float")
    {
        float value{};

        REQUIRE(YamlScalarKernel::parseFloat("250", value));
        CHECK(value == 250.0f);
    }

    SECTION("double")
    {
        double value{};

        REQUIRE(YamlScalarKernel::parseFloat("250", value));
        CHECK(value == 250.0);
    }
}

TEST_CASE("YamlScalarKernel keeps integer spelling classified as integer",
          "[job_yaml][scalar_kernel][classification]")
{
    CHECK(YamlScalarKernel::classify("250") == YamlScalarKind::Integer);
    CHECK(YamlScalarKernel::classify("250.0") == YamlScalarKind::FloatingPoint);
    CHECK(YamlScalarKernel::classify("2.5e2") == YamlScalarKind::FloatingPoint);
}

} // namespace job::yaml::tests