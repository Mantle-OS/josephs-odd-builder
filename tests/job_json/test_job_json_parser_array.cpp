#include <catch2/catch_test_macros.hpp>

#include <array>
#include <set>
#include <string>
#include <vector>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonParser parses empty dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value{1, 2, 3};

    job::json::JsonParser parser{"[]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.empty());
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser parses dynamic integer array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,2,3,4]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::vector<int>{1, 2, 3, 4});
}

TEST_CASE("JsonParser parses dynamic array with whitespace", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[  1,\n 2,\t3, 4  ]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::vector<int>{1, 2, 3, 4});
}

TEST_CASE("JsonParser replaces existing dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value{90, 91, 92};

    job::json::JsonParser parser{"[1,2]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::vector<int>{1, 2});
}

TEST_CASE("JsonParser parses dynamic string array", "[job_json][parser][array]")
{
    std::vector<std::string> value;

    job::json::JsonParser parser{R"(["JOB","Cake Court","JSON"])"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 3);
    REQUIRE(value[0] == "JOB");
    REQUIRE(value[1] == "Cake Court");
    REQUIRE(value[2] == "JSON");
}

TEST_CASE("JsonParser decodes escaped strings in dynamic array", "[job_json][parser][array]")
{
    std::vector<std::string> value;

    job::json::JsonParser parser{R"(["Cake\nCourt","\u004A\u004F\u0042"])"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == "Cake\nCourt");
    REQUIRE(value[1] == "JOB");
}

TEST_CASE("JsonParser parses optional elements in dynamic array", "[job_json][parser][array]")
{
    std::vector<std::optional<int>> value;

    job::json::JsonParser parser{"[1,null,3]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 3);

    REQUIRE(value[0].has_value());
    REQUIRE(*value[0] == 1);

    REQUIRE_FALSE(value[1].has_value());

    REQUIRE(value[2].has_value());
    REQUIRE(*value[2] == 3);
}

TEST_CASE("JsonParser parses nested dynamic arrays", "[job_json][parser][array]")
{
    std::vector<std::vector<int>> value;

    job::json::JsonParser parser{"[[1,2],[3,4],[]]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 3);

    REQUIRE(value[0] == std::vector<int>{1, 2});
    REQUIRE(value[1] == std::vector<int>{3, 4});
    REQUIRE(value[2].empty());
}

TEST_CASE("JsonParser parses reflected objects in dynamic array", "[job_json][parser][array]")
{
    std::vector<job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"([{"port":8080,"host":"localhost"},{"port":9090,"host":"job"}])"
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 2);

    REQUIRE(value[0].port == 8080);
    REQUIRE(value[0].host == "localhost");

    REQUIRE(value[1].port == 9090);
    REQUIRE(value[1].host == "job");
}

TEST_CASE("JsonParser parses fixed integer array", "[job_json][parser][array]")
{
    std::array<int, 4> value{};

    job::json::JsonParser parser{"[10,20,30,40]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::array<int, 4>{10, 20, 30, 40});
}

TEST_CASE("JsonParser parses nested fixed arrays", "[job_json][parser][array]")
{
    std::array<std::array<int, 2>, 2> value{};

    job::json::JsonParser parser{"[[1,2],[3,4]]"};

    REQUIRE(parser.parse(value));

    REQUIRE(value[0] == std::array<int, 2>{1, 2});
    REQUIRE(value[1] == std::array<int, 2>{3, 4});
}

TEST_CASE("JsonParser rejects too few fixed array elements", "[job_json][parser][array]")
{
    std::array<int, 3> value{};

    job::json::JsonParser parser{"[1,2]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ContainerError);
}

TEST_CASE("JsonParser rejects empty JSON array for non empty fixed array", "[job_json][parser][array]")
{
    std::array<int, 3> value{};

    job::json::JsonParser parser{"[]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ContainerError);
}

TEST_CASE("JsonParser parses empty zero length fixed array", "[job_json][parser][array]")
{
    std::array<int, 0> value{};

    job::json::JsonParser parser{"[]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.empty());
}

TEST_CASE("JsonParser rejects too many fixed array elements", "[job_json][parser][array]")
{
    std::array<int, 2> value{};

    job::json::JsonParser parser{"[1,2,3]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ContainerError);
}

TEST_CASE("JsonParser parses insert sequence", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[4,1,3,2]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::set<int>{1, 2, 3, 4});
}

TEST_CASE("JsonParser replaces existing insert sequence", "[job_json][parser][array]")
{
    std::set<int> value{90, 91};

    job::json::JsonParser parser{"[3,1,2]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::set<int>{1, 2, 3});
}

TEST_CASE("JsonParser insert sequence naturally collapses duplicates", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[1,2,2,3,1]"};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::set<int>{1, 2, 3});
}

TEST_CASE("JsonParser rejects scalar into dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value{1, 2, 3};

    job::json::JsonParser parser{"42"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value == std::vector<int>{1, 2, 3});
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects object into dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"{}"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects wrong dynamic array element type", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{R"([1,"two",3])"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);

    REQUIRE(value.size() == 1);
    REQUIRE(value[0] == 1);
}

TEST_CASE("JsonParser rejects wrong fixed array element type", "[job_json][parser][array]")
{
    std::array<int, 3> value{10, 20, 30};

    job::json::JsonParser parser{R"([1,"two",3])"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);

    REQUIRE(value[0] == 1);
    REQUIRE(value[1] == 20);
    REQUIRE(value[2] == 30);
}

TEST_CASE("JsonParser rejects wrong insert array element type", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{R"([1,"two",3])"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);

    REQUIRE(value == std::set<int>{1});
}

TEST_CASE("JsonParser rejects missing separator in dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1 2]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects missing separator in fixed array", "[job_json][parser][array]")
{
    std::array<int, 2> value{};

    job::json::JsonParser parser{"[1 2]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects missing separator in insert array", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[1 2]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects trailing comma in dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,2,]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValue);
}

TEST_CASE("JsonParser rejects trailing comma in fixed array", "[job_json][parser][array]")
{
    std::array<int, 2> value{};

    job::json::JsonParser parser{"[1,2,]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValue);
}

TEST_CASE("JsonParser rejects trailing comma in insert array", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[1,2,]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValue);
}

TEST_CASE("JsonParser rejects dynamic array missing closing bracket", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,2"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects fixed array missing closing bracket", "[job_json][parser][array]")
{
    std::array<int, 2> value{};

    job::json::JsonParser parser{"[1,2"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects insert array missing closing bracket", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[1,2"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects dynamic array ending after separator", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,2,"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects fixed array ending after separator", "[job_json][parser][array]")
{
    std::array<int, 3> value{};

    job::json::JsonParser parser{"[1,2,"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects insert array ending after separator", "[job_json][parser][array]")
{
    std::set<int> value;

    job::json::JsonParser parser{"[1,2,"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects invalid token in dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,@,3]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects malformed number in dynamic array", "[job_json][parser][array]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,0x10,3]"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects array as scalar object member", "[job_json][parser][array]")
{
    job::json::tests::ParserScalarObject value;
    value.count = 42;

    job::json::JsonParser parser{R"({"count":[1,2,3]})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(value.count == 42);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser parses array object member", "[job_json][parser][array]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"id":7,"values":[10,20,30]})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.id == 7);
    REQUIRE(value.values == std::vector<int>{10, 20, 30});
}

TEST_CASE("JsonParser parses nested object and array structure", "[job_json][parser][array]")
{
    std::vector<job::json::tests::ParserRootObject> value;

    job::json::JsonParser parser{
        R"([
            {
                "id":1,
                "server":{"port":8001,"host":"one"},
                "values":[1,2,3]
            },
            {
                "id":2,
                "server":{"port":8002,"host":"two"},
                "values":[4,5,6]
            }
        ])"
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 2);

    REQUIRE(value[0].id == 1);
    REQUIRE(value[0].server.port == 8001);
    REQUIRE(value[0].server.host == "one");
    REQUIRE(value[0].values == std::vector<int>{1, 2, 3});

    REQUIRE(value[1].id == 2);
    REQUIRE(value[1].server.port == 8002);
    REQUIRE(value[1].server.host == "two");
    REQUIRE(value[1].values == std::vector<int>{4, 5, 6});
}

