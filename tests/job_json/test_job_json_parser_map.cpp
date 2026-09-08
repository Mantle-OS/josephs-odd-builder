#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <unordered_map>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


        TEST_CASE("JsonParser parses empty map", "[job_json][parser][map]")
{
    std::map<std::string, int> value{
        {"before", 42}
    };

    job::json::JsonParser parser{"[]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.empty());
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser parses string integer map", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1],["two",2],["three",3]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 3);
    REQUIRE(value.at("one") == 1);
    REQUIRE(value.at("two") == 2);
    REQUIRE(value.at("three") == 3);
}

TEST_CASE("JsonParser parses integer string map", "[job_json][parser][map]")
{
    std::map<int, std::string> value;

    job::json::JsonParser parser{
        R"([[1,"one"],[2,"two"],[3,"three"]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 3);
    REQUIRE(value.at(1) == "one");
    REQUIRE(value.at(2) == "two");
    REQUIRE(value.at(3) == "three");
}

TEST_CASE("JsonParser parses map with whitespace", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"(
        [
            ["one",   1],
            ["two",   2],
            ["three", 3]
        ]
        )"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 3);
    REQUIRE(value.at("one") == 1);
    REQUIRE(value.at("two") == 2);
    REQUIRE(value.at("three") == 3);
}

TEST_CASE("JsonParser replaces existing map", "[job_json][parser][map]")
{
    std::map<std::string, int> value{
        {"old", 99}
    };

    job::json::JsonParser parser{
        R"([["new",42]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("new") == 42);
    REQUIRE_FALSE(value.contains("old"));
}

TEST_CASE("JsonParser parses escaped map key", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["Cake\nCourt",42]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("Cake\nCourt") == 42);
}

TEST_CASE("JsonParser parses Unicode map key", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["\u004A\u004F\u0042",42]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("JOB") == 42);
}

TEST_CASE("JsonParser parses reflected object mapped value", "[job_json][parser][map]")
{
    std::map<std::string, job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"([
            ["one",{"port":8001,"host":"alpha"}],
            ["two",{"port":8002,"host":"beta"}]
        ])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 2);

    REQUIRE(value.at("one").port == 8001);
    REQUIRE(value.at("one").host == "alpha");

    REQUIRE(value.at("two").port == 8002);
    REQUIRE(value.at("two").host == "beta");
}

TEST_CASE("JsonParser parses map inside reflected object", "[job_json][parser][map]")
{
    struct LocalFixture
    {
        std::map<std::string, int> values{};
    };

    LocalFixture value;

    job::json::JsonParser parser{
        R"({"values":[["one",1],["two",2]]})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.values.size() == 2);
    REQUIRE(value.values.at("one") == 1);
    REQUIRE(value.values.at("two") == 2);
}

TEST_CASE("JsonParser parses nested map", "[job_json][parser][map]")
{
    std::map<std::string, std::map<std::string, int>> value;

    job::json::JsonParser parser{
        R"([
            ["outer",[
                ["one",1],
                ["two",2]
            ]]
        ])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("outer").size() == 2);
    REQUIRE(value.at("outer").at("one") == 1);
    REQUIRE(value.at("outer").at("two") == 2);
}

TEST_CASE("JsonParser parses unordered map", "[job_json][parser][map]")
{
    std::unordered_map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1],["two",2]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 2);
    REQUIRE(value.at("one") == 1);
    REQUIRE(value.at("two") == 2);
}

TEST_CASE("JsonParser rejects object representation for map", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"({"one":1,"two":2})"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects scalar representation for map", "[job_json][parser][map]")
{
    std::map<std::string, int> value{
        {"before", 42}
    };

    job::json::JsonParser parser{"42"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("before") == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects map entry that is not an array", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"(["one",1])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ContainerError);
}

TEST_CASE("JsonParser rejects map entry missing value", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one"]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects map entry ending after key", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one")"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects map entry ending after separator", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",)"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects map entry missing closing bracket", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1)"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects map entry with too many elements", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1,2]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedArrayEnd);
}

TEST_CASE("JsonParser rejects wrong map key type", "[job_json][parser][map]")
{
    std::map<int, int> value;

    job::json::JsonParser parser{
        R"([["one",1]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects wrong map value type", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one","bad"]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser preserves successfully inserted map entries before failure", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1],["two","bad"],["three",3]])"
    };

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("one") == 1);
    REQUIRE_FALSE(value.contains("two"));
    REQUIRE_FALSE(value.contains("three"));

    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser duplicate map key preserves first value with emplace", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1],["one",42]])"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value.at("one") == 1);
}

TEST_CASE("JsonParser rejects missing separator between map entries", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1] ["two",2]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects trailing comma after map entry", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1],])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ContainerError);
}

TEST_CASE("JsonParser rejects map missing outer closing bracket", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects invalid token in map key", "[job_json][parser][map]")
{
    std::map<int, int> value;

    job::json::JsonParser parser{
        R"([[@,1]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects invalid token in map value", "[job_json][parser][map]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",@]])"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

