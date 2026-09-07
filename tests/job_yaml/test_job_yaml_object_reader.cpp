#include <catch2/catch_test_macros.hpp>

#if JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#include <job_yaml_object_reader.h>

#include "../tests-fast-math-workaround.h"
#include "test_job_yaml_fixtures.h"
#include "test_job_yaml_utils.h"

using namespace std::string_view_literals;

namespace job::yaml::tests {

// ========================================
// Signed integer
// ========================================

TEST_CASE("YamlObjectReader reads signed integer member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", "42"));
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectReader overwrites existing signed integer member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 7
    };

    REQUIRE(YamlObjectReader::readScalar(object, "count", "-42"));
    REQUIRE(object.count == -42);
}

TEST_CASE("YamlObjectReader rejects invalid signed integer without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 77
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "hello"));
    REQUIRE(object.count == 77);
}

TEST_CASE("YamlObjectReader rejects signed integer overflow without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 77
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "2147483648"));
    REQUIRE(object.count == 77);
}

// ========================================
// Unsigned integer
// ========================================

TEST_CASE("YamlObjectReader reads unsigned integer member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "size", "123"));
    REQUIRE(object.size == 123U);
}

TEST_CASE("YamlObjectReader reads hexadecimal unsigned integer member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "size", "0xFF"));
    REQUIRE(object.size == 255U);
}

TEST_CASE("YamlObjectReader reads octal unsigned integer member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "size", "0o755"));
    REQUIRE(object.size == 493U);
}

TEST_CASE("YamlObjectReader rejects negative value for unsigned member",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .size = 99U
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "size", "-1"));
    REQUIRE(object.size == 99U);
}

TEST_CASE("YamlObjectReader rejects unsigned overflow without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .size = 99U
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "size", "4294967296"));
    REQUIRE(object.size == 99U);
}

// ========================================
// Boolean
// ========================================

TEST_CASE("YamlObjectReader reads true boolean member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "enabled", "true"));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectReader reads false boolean member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .enabled = true
    };

    REQUIRE(YamlObjectReader::readScalar(object, "enabled", "FALSE"));
    REQUIRE_FALSE(object.enabled);
}

TEST_CASE("YamlObjectReader rejects invalid boolean without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .enabled = true
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "enabled", "yes"));
    REQUIRE(object.enabled);
}

// ========================================
// Float
// ========================================

TEST_CASE("YamlObjectReader reads float member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "scale", "2.5"));
    REQUIRE(object.scale == 2.5F);
}

TEST_CASE("YamlObjectReader overwrites existing float member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .scale = 1.25F
    };

    REQUIRE(YamlObjectReader::readScalar(object, "scale", "-4.5"));
    REQUIRE(object.scale == -4.5F);
}

TEST_CASE("YamlObjectReader rejects invalid float without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .scale = 1.25F
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "scale", "hello"));
    REQUIRE(object.scale == 1.25F);
}

// ========================================
// Double
// ========================================

TEST_CASE("YamlObjectReader reads double member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "ratio", "3.1415926535"));
    REQUIRE(object.ratio == 3.1415926535);
}

TEST_CASE("YamlObjectReader reads scientific notation into double member",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "ratio", "1e3"));
    REQUIRE(object.ratio == 1000.0);
}

TEST_CASE("YamlObjectReader rejects malformed double without changing destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .ratio = 2.0
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "ratio", "1.2.3"));
    REQUIRE(object.ratio == 2.0);
}

// ========================================
// String
// ========================================

TEST_CASE("YamlObjectReader reads string member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "alpha"));
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectReader overwrites existing string member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .name = "old"
    };

    REQUIRE(YamlObjectReader::readScalar(object, "name", "new"));
    REQUIRE(object.name == "new");
}

TEST_CASE("YamlObjectReader accepts empty string value", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .name = "not empty"
    };

    REQUIRE(YamlObjectReader::readScalar(object, "name", ""));
    REQUIRE(object.name.empty());
}

TEST_CASE("YamlObjectReader preserves exact string input", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "hello world"));
    REQUIRE(object.name == "hello world");
}

TEST_CASE("YamlObjectReader does not strip quotes from string input",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "\"alpha\""));
    REQUIRE(object.name == "\"alpha\"");
}

TEST_CASE("YamlObjectReader does not interpret escapes in string input",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "hello\\nworld"));
    REQUIRE(object.name == "hello\\nworld");
}

TEST_CASE("YamlObjectReader string destination owns source value",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    ObjectReaderObject object{};

    REQUIRE(value == "alpha");
    REQUIRE(value.data() == source.data() + 6);

    REQUIRE(YamlObjectReader::readScalar(object, "name", value));

    REQUIRE(object.name == "alpha");
    REQUIRE(object.name.data() != value.data());
}

TEST_CASE("YamlObjectReader string destination survives source mutation",
          "[job_yaml][object_reader][source_view]")
{
    char source[] = "alpha";
    const std::string_view value{source, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", value));
    REQUIRE(object.name == "alpha");

    source[0] = 'X';

    REQUIRE(value == "Xlpha");
    REQUIRE(object.name == "alpha");
}

// ========================================
// string_view
// ========================================

TEST_CASE("YamlObjectReader reads string_view member", "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "alpha";

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", source));
    REQUIRE(object.view == "alpha");
}

TEST_CASE("YamlObjectReader string_view destination borrows exact source slice",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "prefixalphasuffix";
    const std::string_view value = source.substr(6, 5);

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));

    REQUIRE(object.view == "alpha");
    REQUIRE(object.view.data() == value.data());
    REQUIRE(object.view.data() == source.data() + 6);
    REQUIRE(object.view.size() == value.size());
}

TEST_CASE("YamlObjectReader overwrites string_view member without copying",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view first = "alpha";
    constexpr std::string_view second = "beta";

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", first));
    REQUIRE(object.view.data() == first.data());

    REQUIRE(YamlObjectReader::readScalar(object, "view", second));
    REQUIRE(object.view == "beta");
    REQUIRE(object.view.data() == second.data());
}

TEST_CASE("YamlObjectReader accepts empty string_view value",
          "[job_yaml][object_reader][source_view]")
{
    ObjectReaderObject object{
        .view = "alpha"
    };

    REQUIRE(YamlObjectReader::readScalar(object, "view", ""));
    REQUIRE(object.view.empty());
}

TEST_CASE("YamlObjectReader string_view preserves exact value bounds",
          "[job_yaml][object_reader][source_view]")
{
    constexpr char source[] = {
        'x',
        'a', 'l', 'p', 'h', 'a',
        'x'
    };

    const std::string_view value{source + 1, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view == "alpha");
    REQUIRE(object.view.data() == source + 1);
    REQUIRE(object.view.size() == 5);
}

TEST_CASE("YamlObjectReader string_view reflects borrowed source mutation",
          "[job_yaml][object_reader][source_view]")
{
    char source[] = "alpha";
    const std::string_view value{source, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view == "alpha");

    source[0] = 'X';

    REQUIRE(object.view == "Xlpha");
    REQUIRE(object.view.data() == source);
}

// ========================================
// Enum
// ========================================

TEST_CASE("YamlObjectReader reads enum member", "[job_yaml][object_reader][enum]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "mode", "2"));
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

TEST_CASE("YamlObjectReader reads hexadecimal enum value", "[job_yaml][object_reader][enum]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "mode", "0x1"));
    REQUIRE(object.mode == ObjectReaderMode::One);
}

TEST_CASE("YamlObjectReader rejects invalid enum without changing destination",
          "[job_yaml][object_reader][enum]")
{
    ObjectReaderObject object{
        .mode = ObjectReaderMode::One
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "mode", "invalid"));
    REQUIRE(object.mode == ObjectReaderMode::One);
}

TEST_CASE("YamlObjectReader rejects negative value for unsigned enum",
          "[job_yaml][object_reader][enum]")
{
    ObjectReaderObject object{
        .mode = ObjectReaderMode::One
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "mode", "-1"));
    REQUIRE(object.mode == ObjectReaderMode::One);
}

TEST_CASE("YamlObjectReader parses enum directly from source slice",
          "[job_yaml][object_reader][enum][source_view]")
{
    constexpr std::string_view source = "prefix2suffix";
    const std::string_view value = source.substr(6, 1);

    ObjectReaderObject object{};

    REQUIRE(value.data() == source.data() + 6);
    REQUIRE(YamlObjectReader::readScalar(object, "mode", value));
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

// ========================================
// Unknown keys
// ========================================

TEST_CASE("YamlObjectReader returns false for unknown key", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "missing", "42"));
}

TEST_CASE("YamlObjectReader unknown key leaves entire object unchanged",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 7,
        .size = 11,
        .enabled = true,
        .scale = 1.25F,
        .ratio = 2.5,
        .name = "alpha",
        .view = "beta",
        .mode = ObjectReaderMode::Two
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "missing", "999"));

    REQUIRE(object.count == 7);
    REQUIRE(object.size == 11U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.25F);
    REQUIRE(object.ratio == 2.5);
    REQUIRE(object.name == "alpha");
    REQUIRE(object.view == "beta");
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

TEST_CASE("YamlObjectReader returns false for empty key", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "", "42"));
}

// ========================================
// Member isolation
// ========================================

TEST_CASE("YamlObjectReader changes only selected member", "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 1,
        .size = 2,
        .enabled = false,
        .scale = 3.0F,
        .ratio = 4.0,
        .name = "five",
        .view = "six",
        .mode = ObjectReaderMode::One
    };

    REQUIRE(YamlObjectReader::readScalar(object, "count", "99"));

    REQUIRE(object.count == 99);
    REQUIRE(object.size == 2U);
    REQUIRE_FALSE(object.enabled);
    REQUIRE(object.scale == 3.0F);
    REQUIRE(object.ratio == 4.0);
    REQUIRE(object.name == "five");
    REQUIRE(object.view == "six");
    REQUIRE(object.mode == ObjectReaderMode::One);
}

// ========================================
// Repeated assignments
// ========================================

TEST_CASE("YamlObjectReader supports repeated assignments to same member",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", "1"));
    REQUIRE(object.count == 1);

    REQUIRE(YamlObjectReader::readScalar(object, "count", "2"));
    REQUIRE(object.count == 2);

    REQUIRE(YamlObjectReader::readScalar(object, "count", "-3"));
    REQUIRE(object.count == -3);
}

TEST_CASE("YamlObjectReader supports assigning multiple members of same object",
          "[job_yaml][object_reader]")
{
    constexpr std::string_view view = "beta";

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", "42"));
    REQUIRE(YamlObjectReader::readScalar(object, "size", "1024"));
    REQUIRE(YamlObjectReader::readScalar(object, "enabled", "true"));
    REQUIRE(YamlObjectReader::readScalar(object, "scale", "1.5"));
    REQUIRE(YamlObjectReader::readScalar(object, "ratio", "2.25"));
    REQUIRE(YamlObjectReader::readScalar(object, "name", "alpha"));
    REQUIRE(YamlObjectReader::readScalar(object, "view", view));
    REQUIRE(YamlObjectReader::readScalar(object, "mode", "2"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "alpha");
    REQUIRE(object.view == "beta");
    REQUIRE(object.view.data() == view.data());
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

// ========================================
// Failed conversion isolation
// ========================================

TEST_CASE("YamlObjectReader failed conversion does not affect other members",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{
        .count = 42,
        .size = 1024,
        .enabled = true,
        .scale = 1.5F,
        .ratio = 2.25,
        .name = "alpha",
        .view = "beta",
        .mode = ObjectReaderMode::Two
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "not-an-int"));

    REQUIRE(object.count == 42);
    REQUIRE(object.size == 1024U);
    REQUIRE(object.enabled);
    REQUIRE(object.scale == 1.5F);
    REQUIRE(object.ratio == 2.25);
    REQUIRE(object.name == "alpha");
    REQUIRE(object.view == "beta");
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

TEST_CASE("YamlObjectReader remains usable after failed conversion",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "bad"));
    REQUIRE(object.count == 0);

    REQUIRE(YamlObjectReader::readScalar(object, "count", "42"));
    REQUIRE(object.count == 42);
}

// ========================================
// Type routing
// ========================================

TEST_CASE("YamlObjectReader routes bool before integral handling",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "enabled", "true"));
    REQUIRE(object.enabled);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "enabled", "1"));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectReader does not parse numeric text into string destination",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "42"));
    REQUIRE(object.name == "42");
}

TEST_CASE("YamlObjectReader does not parse numeric text into string_view destination",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view value = "42";

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view == "42");
    REQUIRE(object.view.data() == value.data());
}

TEST_CASE("YamlObjectReader does not classify string destination before assignment",
          "[job_yaml][object_reader]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "true"));
    REQUIRE(object.name == "true");

    REQUIRE(YamlObjectReader::readScalar(object, "name", "null"));
    REQUIRE(object.name == "null");

    REQUIRE(YamlObjectReader::readScalar(object, "name", "3.14"));
    REQUIRE(object.name == "3.14");
}

TEST_CASE("YamlObjectReader does not classify string_view destination before assignment",
          "[job_yaml][object_reader][source_view]")
{
    ObjectReaderObject object{};

    constexpr std::string_view boolean = "true";
    constexpr std::string_view nullValue = "null";
    constexpr std::string_view floating = "3.14";

    REQUIRE(YamlObjectReader::readScalar(object, "view", boolean));
    REQUIRE(object.view == "true");
    REQUIRE(object.view.data() == boolean.data());

    REQUIRE(YamlObjectReader::readScalar(object, "view", nullValue));
    REQUIRE(object.view == "null");
    REQUIRE(object.view.data() == nullValue.data());

    REQUIRE(YamlObjectReader::readScalar(object, "view", floating));
    REQUIRE(object.view == "3.14");
    REQUIRE(object.view.data() == floating.data());
}

// ========================================
// Numeric boundaries
// ========================================

TEST_CASE("YamlObjectReader reads signed integer destination boundaries",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", "-2147483648"));
    REQUIRE(object.count == std::numeric_limits<int>::min());

    REQUIRE(YamlObjectReader::readScalar(object, "count", "2147483647"));
    REQUIRE(object.count == std::numeric_limits<int>::max());
}

TEST_CASE("YamlObjectReader reads unsigned integer maximum",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "size", "4294967295"));
    REQUIRE(object.size == std::numeric_limits<std::uint32_t>::max());
}

TEST_CASE("YamlObjectReader rejects integer values immediately outside destination range",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .count = 17,
        .size = 23
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "-2147483649"));
    REQUIRE(object.count == 17);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "2147483648"));
    REQUIRE(object.count == 17);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "size", "4294967296"));
    REQUIRE(object.size == 23U);
}

// ========================================
// Special floating point values
// ========================================

TEST_CASE("YamlObjectReader reads positive YAML infinity into float",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "scale", ".inf"));
    REQUIRE(isSafeInfinity(object.scale));
    REQUIRE(isSafePositiveInfinity(object.scale));
}

TEST_CASE("YamlObjectReader reads negative YAML infinity into double",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "ratio", "-.INF"));
    REQUIRE(isSafeInfinity(object.ratio));
    REQUIRE(isSafeNegativeInfinity(object.ratio));
}

TEST_CASE("YamlObjectReader reads YAML NaN into floating point destination",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "scale", ".nan"));
    REQUIRE(isSafeNaN(object.scale));

    REQUIRE(YamlObjectReader::readScalar(object, "ratio", ".NAN"));
    REQUIRE(isSafeNaN(object.ratio));
}

TEST_CASE("YamlObjectReader rejects non YAML infinity and NaN spellings",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .scale = 1.5F,
        .ratio = 2.5
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "scale", "inf"));
    REQUIRE(object.scale == 1.5F);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "ratio", "nan"));
    REQUIRE(object.ratio == 2.5);
}

// ========================================
// Empty scalar behavior
// ========================================

TEST_CASE("YamlObjectReader rejects empty scalar for numeric destinations",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .count = 10,
        .size = 20,
        .scale = 1.5F,
        .ratio = 2.5
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", ""));
    REQUIRE(object.count == 10);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "size", ""));
    REQUIRE(object.size == 20U);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "scale", ""));
    REQUIRE(object.scale == 1.5F);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "ratio", ""));
    REQUIRE(object.ratio == 2.5);
}

TEST_CASE("YamlObjectReader rejects empty scalar for boolean destination",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .enabled = true
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "enabled", ""));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectReader rejects empty scalar for enum destination",
          "[job_yaml][object_reader][edge][enum]")
{
    ObjectReaderObject object{
        .mode = ObjectReaderMode::Two
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "mode", ""));
    REQUIRE(object.mode == ObjectReaderMode::Two);
}

TEST_CASE("YamlObjectReader accepts empty scalar for string destinations",
          "[job_yaml][object_reader][edge][source_view]")
{
    ObjectReaderObject object{
        .name = "alpha",
        .view = "beta"
    };

    REQUIRE(YamlObjectReader::readScalar(object, "name", ""));
    REQUIRE(object.name.empty());

    REQUIRE(YamlObjectReader::readScalar(object, "view", ""));
    REQUIRE(object.view.empty());
}

// ========================================
// Whitespace ownership
// ========================================

TEST_CASE("YamlObjectReader does not trim integer scalar input",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .count = 42
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", " 7"));
    REQUIRE(object.count == 42);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "7 "));
    REQUIRE(object.count == 42);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "count", "\t7"));
    REQUIRE(object.count == 42);
}

TEST_CASE("YamlObjectReader does not trim floating point scalar input",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .ratio = 2.5
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "ratio", " 3.5"));
    REQUIRE(object.ratio == 2.5);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "ratio", "3.5 "));
    REQUIRE(object.ratio == 2.5);
}

TEST_CASE("YamlObjectReader does not trim boolean scalar input",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{
        .enabled = true
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "enabled", " false"));
    REQUIRE(object.enabled);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "enabled", "false "));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectReader does not trim enum scalar input",
          "[job_yaml][object_reader][edge][enum]")
{
    ObjectReaderObject object{
        .mode = ObjectReaderMode::One
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "mode", " 2"));
    REQUIRE(object.mode == ObjectReaderMode::One);

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "mode", "2 "));
    REQUIRE(object.mode == ObjectReaderMode::One);
}

TEST_CASE("YamlObjectReader preserves whitespace for string destination",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", "  alpha  "));
    REQUIRE(object.name == "  alpha  ");
}

TEST_CASE("YamlObjectReader preserves whitespace for string_view destination",
          "[job_yaml][object_reader][edge][source_view]")
{
    constexpr std::string_view value = "  alpha  ";

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view == value);
    REQUIRE(object.view.data() == value.data());
}

// ========================================
// Direct source slice handling
// ========================================

TEST_CASE("YamlObjectReader parses integer directly from source slice",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "prefix123suffix";
    const std::string_view value = source.substr(6, 3);

    ObjectReaderObject object{};

    REQUIRE(value.data() == source.data() + 6);
    REQUIRE(YamlObjectReader::readScalar(object, "count", value));
    REQUIRE(object.count == 123);
}

TEST_CASE("YamlObjectReader parses boolean directly from source slice",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "prefixtruesuffix";
    const std::string_view value = source.substr(6, 4);

    ObjectReaderObject object{};

    REQUIRE(value.data() == source.data() + 6);
    REQUIRE(YamlObjectReader::readScalar(object, "enabled", value));
    REQUIRE(object.enabled);
}

TEST_CASE("YamlObjectReader parses floating point directly from source slice",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "prefix12.5suffix";
    const std::string_view value = source.substr(6, 4);

    ObjectReaderObject object{};

    REQUIRE(value.data() == source.data() + 6);
    REQUIRE(YamlObjectReader::readScalar(object, "ratio", value));
    REQUIRE(object.ratio == 12.5);
}

TEST_CASE("YamlObjectReader source slice boundaries are respected",
          "[job_yaml][object_reader][source_view]")
{
    constexpr std::string_view source = "x123y";
    const std::string_view value = source.substr(1, 3);

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", value));
    REQUIRE(object.count == 123);
}

// ========================================
// string_view boundaries
// ========================================

TEST_CASE("YamlObjectReader accepts non null terminated key string_view",
          "[job_yaml][object_reader][edge]")
{
    constexpr char source[] = {
        'x',
        'c', 'o', 'u', 'n', 't',
        'x'
    };

    const std::string_view key{source + 1, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, key, "73"));
    REQUIRE(object.count == 73);
}

TEST_CASE("YamlObjectReader accepts non null terminated value string_view",
          "[job_yaml][object_reader][edge]")
{
    constexpr char source[] = {
        'x',
        '1', '2', '3',
        'x'
    };

    const std::string_view value{source + 1, 3};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "count", value));
    REQUIRE(object.count == 123);
}

TEST_CASE("YamlObjectReader string assignment respects string_view length",
          "[job_yaml][object_reader][edge]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", value));
    REQUIRE(object.name == "alpha");
}

TEST_CASE("YamlObjectReader string_view assignment respects string_view length",
          "[job_yaml][object_reader][edge][source_view]")
{
    constexpr char source[] = "alpha-extra";
    const std::string_view value{source, 5};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view == "alpha");
    REQUIRE(object.view.data() == source);
    REQUIRE(object.view.size() == 5);
}

TEST_CASE("YamlObjectReader preserves embedded null in string scalar",
          "[job_yaml][object_reader][edge]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "name", value));
    REQUIRE(object.name.size() == sizeof(source));
    REQUIRE(object.name[0] == 'J');
    REQUIRE(object.name[2] == 'B');
    REQUIRE(object.name[3] == '\0');
    REQUIRE(object.name[4] == 'Y');
    REQUIRE(object.name[7] == 'L');
}

TEST_CASE("YamlObjectReader preserves embedded null in string_view scalar",
          "[job_yaml][object_reader][edge][source_view]")
{
    constexpr char source[] = {
        'J', 'O', 'B', '\0', 'Y', 'A', 'M', 'L'
    };

    const std::string_view value{source, sizeof(source)};

    ObjectReaderObject object{};

    REQUIRE(YamlObjectReader::readScalar(object, "view", value));
    REQUIRE(object.view.size() == sizeof(source));
    REQUIRE(object.view.data() == source);
    REQUIRE(object.view[0] == 'J');
    REQUIRE(object.view[2] == 'B');
    REQUIRE(object.view[3] == '\0');
    REQUIRE(object.view[4] == 'Y');
    REQUIRE(object.view[7] == 'L');
}

// ========================================
// Unsupported destination type
// ========================================

TEST_CASE("YamlObjectReader returns false for unsupported destination type",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderUnsupportedObject object{
        .count = 7,
        .values = {1, 2, 3}
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "values", "42"));

    REQUIRE(object.count == 7);
    REQUIRE(object.values.size() == 3);
    REQUIRE(object.values[0] == 1);
    REQUIRE(object.values[1] == 2);
    REQUIRE(object.values[2] == 3);
}

TEST_CASE("YamlObjectReader unsupported member does not prevent supported member reads",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderUnsupportedObject object{
        .values = {1, 2, 3}
    };

    REQUIRE(YamlObjectReader::readScalar(object, "count", "99"));

    REQUIRE(object.count == 99);
    REQUIRE(object.values.size() == 3);
}

TEST_CASE("YamlObjectReader unknown and unsupported keys remain distinct operational cases",
          "[job_yaml][object_reader][edge]")
{
    ObjectReaderUnsupportedObject object{
        .count = 5,
        .values = {10, 20}
    };

    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "missing", "42"));
    REQUIRE_FALSE(YamlObjectReader::readScalar(object, "values", "42"));

    REQUIRE(object.count == 5);
    REQUIRE(object.values.size() == 2);
    REQUIRE(object.values[0] == 10);
    REQUIRE(object.values[1] == 20);
}

// ========================================
// Benchmarks
// ========================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("YamlObjectReader scalar benchmarks",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view intKey = "count";
    std::string_view intValue = "42";

    benchmarkDoNotOptimize(intKey);
    benchmarkDoNotOptimize(intValue);

    BENCHMARK("YamlObjectReader integer")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(intKey);
        benchmarkDoNotOptimize(intValue);

        const bool result = YamlObjectReader::readScalar(object, intKey, intValue);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };

    BENCHMARK("YamlKeyDispatch plus ScalarKernel integer")
    {
        ObjectReaderObject object{};
        bool converted = false;

        benchmarkDoNotOptimize(intKey);
        benchmarkDoNotOptimize(intValue);

        YamlKeyDispatch::dispatch<ObjectReaderObject>(intKey, [&]<auto member> {
            auto &destination = object.[:member:];
            using MemberType = std::remove_cvref_t<decltype(destination)>;

            if constexpr (std::same_as<MemberType, int>)
                converted = YamlScalarKernel::parse(intValue, destination);
        });

        benchmarkClobber(object);
        benchmarkDoNotOptimize(converted);

        return object.count;
    };

    BENCHMARK("direct ScalarKernel integer")
    {
        int value{};

        benchmarkDoNotOptimize(intValue);

        const bool converted = YamlScalarKernel::parse(intValue, value);

        benchmarkDoNotOptimize(value);
        benchmarkDoNotOptimize(converted);

        return value;
    };
}

TEST_CASE("YamlObjectReader boolean benchmark",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view key = "enabled";
    std::string_view value = "true";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectReader boolean")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.enabled;
    };

    BENCHMARK("direct ScalarKernel boolean")
    {
        bool destination = false;

        benchmarkDoNotOptimize(value);

        const bool result = YamlScalarKernel::parse(value, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };
}

TEST_CASE("YamlObjectReader floating point benchmark",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view key = "ratio";
    std::string_view value = "3.1415926535";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectReader double")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.ratio;
    };

    BENCHMARK("direct ScalarKernel double")
    {
        double destination{};

        benchmarkDoNotOptimize(value);

        const bool result = YamlScalarKernel::parse(value, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return destination;
    };
}

TEST_CASE("YamlObjectReader string benchmark",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view key = "name";
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectReader string")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.name.size();
    };

    BENCHMARK("direct string assignment")
    {
        std::string destination;

        benchmarkDoNotOptimize(value);

        destination.assign(value);

        benchmarkClobber(destination);

        return destination.size();
    };
}

TEST_CASE("YamlObjectReader string_view benchmark",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view key = "view";
    std::string_view value = "alpha";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectReader string_view")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.view.size();
    };

    BENCHMARK("direct string_view assignment")
    {
        std::string_view destination;

        benchmarkDoNotOptimize(value);

        destination = value;

        benchmarkDoNotOptimize(destination);

        return destination.size();
    };
}

TEST_CASE("YamlObjectReader enum benchmark", "[job_yaml][object_reader][benchmark]")
{
    std::string_view key = "mode";
    std::string_view value = "2";

    benchmarkDoNotOptimize(key);
    benchmarkDoNotOptimize(value);

    BENCHMARK("YamlObjectReader enum")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(key);
        benchmarkDoNotOptimize(value);

        const bool result = YamlObjectReader::readScalar(object, key, value);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return static_cast<std::uint8_t>(object.mode);
    };

    BENCHMARK("direct ScalarKernel enum")
    {
        ObjectReaderMode destination = ObjectReaderMode::Zero;

        benchmarkDoNotOptimize(value);

        const bool result = YamlScalarKernel::parse(value, destination);

        benchmarkDoNotOptimize(destination);
        benchmarkDoNotOptimize(result);

        return static_cast<std::uint8_t>(destination);
    };
}

TEST_CASE("YamlObjectReader failure benchmarks",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view missingKey = "missing";
    std::string_view validValue = "42";
    std::string_view countKey = "count";
    std::string_view invalidValue = "not-an-integer";

    benchmarkDoNotOptimize(missingKey);
    benchmarkDoNotOptimize(validValue);
    benchmarkDoNotOptimize(countKey);
    benchmarkDoNotOptimize(invalidValue);

    BENCHMARK("YamlObjectReader missing key")
    {
        ObjectReaderObject object{};

        benchmarkDoNotOptimize(missingKey);
        benchmarkDoNotOptimize(validValue);

        const bool result = YamlObjectReader::readScalar(object, missingKey, validValue);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return result;
    };

    BENCHMARK("YamlObjectReader invalid integer")
    {
        ObjectReaderObject object{
            .count = 17
        };

        benchmarkDoNotOptimize(countKey);
        benchmarkDoNotOptimize(invalidValue);

        const bool result = YamlObjectReader::readScalar(object, countKey, invalidValue);

        benchmarkClobber(object);
        benchmarkDoNotOptimize(result);

        return object.count;
    };
}

TEST_CASE("YamlObjectReader complete object benchmark",
          "[job_yaml][object_reader][benchmark]")
{
    std::string_view countKey = "count";
    std::string_view countValue = "42";

    std::string_view sizeKey = "size";
    std::string_view sizeValue = "1024";

    std::string_view enabledKey = "enabled";
    std::string_view enabledValue = "true";

    std::string_view scaleKey = "scale";
    std::string_view scaleValue = "1.5";

    std::string_view ratioKey = "ratio";
    std::string_view ratioValue = "2.25";

    std::string_view nameKey = "name";
    std::string_view nameValue = "alpha";

    std::string_view viewKey = "view";
    std::string_view viewValue = "beta";

    std::string_view modeKey = "mode";
    std::string_view modeValue = "2";

    benchmarkDoNotOptimize(countKey);
    benchmarkDoNotOptimize(countValue);
    benchmarkDoNotOptimize(sizeKey);
    benchmarkDoNotOptimize(sizeValue);
    benchmarkDoNotOptimize(enabledKey);
    benchmarkDoNotOptimize(enabledValue);
    benchmarkDoNotOptimize(scaleKey);
    benchmarkDoNotOptimize(scaleValue);
    benchmarkDoNotOptimize(ratioKey);
    benchmarkDoNotOptimize(ratioValue);
    benchmarkDoNotOptimize(nameKey);
    benchmarkDoNotOptimize(nameValue);
    benchmarkDoNotOptimize(viewKey);
    benchmarkDoNotOptimize(viewValue);
    benchmarkDoNotOptimize(modeKey);
    benchmarkDoNotOptimize(modeValue);

    BENCHMARK("YamlObjectReader eight member object")
    {
        ObjectReaderObject object{};

        bool ok = true;
        ok &= YamlObjectReader::readScalar(object, countKey, countValue);
        ok &= YamlObjectReader::readScalar(object, sizeKey, sizeValue);
        ok &= YamlObjectReader::readScalar(object, enabledKey, enabledValue);
        ok &= YamlObjectReader::readScalar(object, scaleKey, scaleValue);
        ok &= YamlObjectReader::readScalar(object, ratioKey, ratioValue);
        ok &= YamlObjectReader::readScalar(object, nameKey, nameValue);
        ok &= YamlObjectReader::readScalar(object, viewKey, viewValue);
        ok &= YamlObjectReader::readScalar(object, modeKey, modeValue);

        benchmarkDoNotOptimize(ok);
        benchmarkClobber(object);

        return object.count +
               static_cast<int>(object.size) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.scale) +
               static_cast<int>(object.ratio) +
               static_cast<int>(object.name.size()) +
               static_cast<int>(object.view.size()) +
               static_cast<int>(object.mode);
    };

    BENCHMARK("manual eight member object")
    {
        ObjectReaderObject object{};

        bool ok = true;
        ok &= YamlScalarKernel::parse(countValue, object.count);
        ok &= YamlScalarKernel::parse(sizeValue, object.size);
        ok &= YamlScalarKernel::parse(enabledValue, object.enabled);
        ok &= YamlScalarKernel::parse(scaleValue, object.scale);
        ok &= YamlScalarKernel::parse(ratioValue, object.ratio);
        object.name.assign(nameValue);
        object.view = viewValue;
        ok &= YamlScalarKernel::parse(modeValue, object.mode);

        benchmarkDoNotOptimize(ok);
        benchmarkClobber(object);

        return object.count +
               static_cast<int>(object.size) +
               static_cast<int>(object.enabled) +
               static_cast<int>(object.scale) +
               static_cast<int>(object.ratio) +
               static_cast<int>(object.name.size()) +
               static_cast<int>(object.view.size()) +
               static_cast<int>(object.mode);
    };
}
#endif

} // namespace job::yaml::tests