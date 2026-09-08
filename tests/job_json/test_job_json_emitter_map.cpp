#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <unordered_map>

#include <job_json_emitter.h>
#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonEmitter emits empty map", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[]");
}

TEST_CASE("JsonEmitter emits string integer map", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"one", 1},
        {"three", 3},
        {"two", 2}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"([["one",1],["three",3],["two",2]])");
}

TEST_CASE("JsonEmitter emits integer string map", "[job_json][emitter][map]")
{
    const std::map<int, std::string> value{
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"([[1,"one"],[2,"two"],[3,"three"]])");
}

TEST_CASE("JsonEmitter escapes map string keys", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"Cake\nCourt", 42}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"([["Cake\nCourt",42]])");
}

TEST_CASE("JsonEmitter preserves UTF8 map keys", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"世界 😀", 42}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[[\"世界 😀\",42]]");
}

TEST_CASE("JsonEmitter emits reflected object mapped value", "[job_json][emitter][map]")
{
    std::map<std::string, job::json::tests::ParserNestedObject> value;

    value["one"].port = 8001;
    value["one"].host = "alpha";

    value["two"].port = 8002;
    value["two"].host = "beta";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(
        output ==
        R"([["one",{"port":8001,"host":"alpha"}],["two",{"port":8002,"host":"beta"}]])");
}

TEST_CASE("JsonEmitter emits nested map", "[job_json][emitter][map]")
{
    const std::map<std::string, std::map<std::string, int>> value{
        {
            "outer",
            {
                {"one", 1},
                {"two", 2}
            }
        }
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(
        output ==
        R"([["outer",[["one",1],["two",2]]]])");
}

TEST_CASE("JsonEmitter emits map with optional values", "[job_json][emitter][map]")
{
    const std::map<std::string, std::optional<int>> value{
        {"one", 1},
        {"three", 3},
        {"two", std::nullopt}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(
        output ==
        R"([["one",1],["three",3],["two",null]])");
}

TEST_CASE("JsonEmitter emits map with array values", "[job_json][emitter][map]")
{
    const std::map<std::string, std::vector<int>> value{
        {"one", {1, 2}},
        {"two", {3, 4}}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(
        output ==
        R"([["one",[1,2]],["two",[3,4]]])");
}

TEST_CASE("JsonEmitter emits map with object keys", "[job_json][emitter][map]")
{
    struct Key
    {
        int id{};

        [[nodiscard]] constexpr bool operator<(const Key &other) const noexcept
        {
            return id < other.id;
        }
    };

    const std::map<Key, int> value{
        {Key{1}, 10},
        {Key{2}, 20}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"([[{"id":1},10],[{"id":2},20]])");
}

TEST_CASE("JsonEmitter emits unordered map", "[job_json][emitter][map]")
{
    const std::unordered_map<std::string, int> value{
        {"one", 1},
        {"two", 2}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    std::unordered_map<std::string, int> parsed;

    job::json::JsonParser parser{output};

    REQUIRE(parser.parse(parsed));
    REQUIRE(parsed == value);
}

TEST_CASE("JsonEmitter map output parses back into map", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"one", 1},
        {"two", 2},
        {"three", 3}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    std::map<std::string, int> parsed;

    job::json::JsonParser parser{output};

    REQUIRE(parser.parse(parsed));
    REQUIRE(parsed == value);
}

TEST_CASE("JsonEmitter nested map output parses back into map", "[job_json][emitter][map]")
{
    const std::map<std::string, std::map<std::string, int>> value{
        {
            "outer",
            {
                {"one", 1},
                {"two", 2}
            }
        }
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    std::map<std::string, std::map<std::string, int>> parsed;

    job::json::JsonParser parser{output};

    REQUIRE(parser.parse(parsed));
    REQUIRE(parsed == value);
}

TEST_CASE("JsonEmitter appends map to existing sink", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"one", 1}
    };

    std::string output = "prefix:";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(prefix:[["one",1]])");
}

TEST_CASE("JsonEmitter emits map without extra whitespace", "[job_json][emitter][map]")
{
    const std::map<std::string, int> value{
        {"one", 1},
        {"two", 2}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output.find(' ') == std::string::npos);
    REQUIRE(output.find('\n') == std::string::npos);
    REQUIRE(output.find('\t') == std::string::npos);
}

