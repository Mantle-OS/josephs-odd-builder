#include <catch2/catch_test_macros.hpp>

#include <string>
#include <map>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


    TEST_CASE("JsonParser diagnostic is clear after successful parse", "[job_json][parser][diagnostic]")
{
    int value{};

    job::json::JsonParser parser{"42"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(parser.diagnostic().hasError());
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::None);
}

TEST_CASE("JsonParser diagnostic reports unexpected end for empty input", "[job_json][parser][diagnostic]")
{
    int value{};

    job::json::JsonParser parser{""};

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 1);
}

TEST_CASE("JsonParser diagnostic reports unexpected end after whitespace", "[job_json][parser][diagnostic]")
{
    int value{};

    job::json::JsonParser parser{" \n  "};

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
    REQUIRE(diagnostic.offset() == parser.source().size());
    REQUIRE(diagnostic.line(parser.source()) == 2);
    REQUIRE(diagnostic.column(parser.source()) == 3);
}

TEST_CASE("JsonParser diagnostic points at invalid root token", "[job_json][parser][diagnostic]")
{
    int value{};

    job::json::JsonParser parser{"@"};

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::InvalidToken);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 1);
}

TEST_CASE("JsonParser diagnostic points at trailing root token", "[job_json][parser][diagnostic]")
{
    int value{};

    job::json::JsonParser parser{"42 true"};

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedToken);
    REQUIRE(diagnostic.offset() == 3);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 4);
}

TEST_CASE("JsonParser diagnostic points at unknown object member", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"missing":42})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnknownMember);
    REQUIRE(diagnostic.offset() == 1);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 2);
}

TEST_CASE("JsonParser diagnostic points at unknown multiline object member", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        "{\n"
        "  \"count\": 42,\n"
        "  \"missing\": 7\n"
        "}"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnknownMember);
    REQUIRE(diagnostic.line(parser.source()) == 3);
    REQUIRE(diagnostic.column(parser.source()) == 3);
}

TEST_CASE("JsonParser diagnostic points at missing object name separator", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count" 42})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedNameSeparator);
    REQUIRE(diagnostic.offset() == 9);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 10);
}

TEST_CASE("JsonParser diagnostic reports name separator at EOF", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count")"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedNameSeparator);
    REQUIRE(diagnostic.offset() == parser.source().size());
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == parser.source().size() + 1);
}

TEST_CASE("JsonParser diagnostic points at missing object value separator", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":42 "enabled":true})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
    REQUIRE(diagnostic.offset() == 12);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 13);
}

TEST_CASE("JsonParser diagnostic reports unexpected end for unterminated object", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":42)"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
    REQUIRE(diagnostic.offset() == parser.source().size());
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == parser.source().size() + 1);
}

TEST_CASE("JsonParser diagnostic points at wrong object member value type", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":"wrong"})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::TypeMismatch);
    REQUIRE(diagnostic.offset() == 9);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 10);
}

TEST_CASE("JsonParser diagnostic points at malformed array element", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[1,@,3]"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::InvalidToken);
    REQUIRE(diagnostic.offset() == 3);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 4);
}

TEST_CASE("JsonParser diagnostic points at missing array separator", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[1 2]"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
    REQUIRE(diagnostic.offset() == 3);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 4);
}

TEST_CASE("JsonParser diagnostic reports unexpected end for unterminated array", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[1,2"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
    REQUIRE(diagnostic.offset() == parser.source().size());
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == parser.source().size() + 1);
}

TEST_CASE("JsonParser diagnostic reports expected value for array trailing comma", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[1,2,]"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedValue);
    REQUIRE(diagnostic.offset() == 5);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 6);
}

TEST_CASE("JsonParser diagnostic reports map entry shape failure", "[job_json][parser][diagnostic]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"(["one",1])"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ContainerError);
    REQUIRE(diagnostic.offset() == 1);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 2);
}

TEST_CASE("JsonParser diagnostic reports map entry array end failure", "[job_json][parser][diagnostic]")
{
    std::map<std::string, int> value;

    job::json::JsonParser parser{
        R"([["one",1,2]])"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::ExpectedArrayEnd);
    REQUIRE(diagnostic.offset() == 9);
    REQUIRE(diagnostic.line(parser.source()) == 1);
    REQUIRE(diagnostic.column(parser.source()) == 10);
}

TEST_CASE("JsonParser diagnostic computes multiline array failure location", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[\n"
        "  1,\n"
        "  2,\n"
        "  @\n"
        "]"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::InvalidToken);
    REQUIRE(diagnostic.line(parser.source()) == 4);
    REQUIRE(diagnostic.column(parser.source()) == 3);
}

TEST_CASE("JsonParser diagnostic computes nested multiline object failure location", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        "{\n"
        "  \"id\": 7,\n"
        "  \"server\": {\n"
        "    \"port\": 8080,\n"
        "    \"host\": 42\n"
        "  }\n"
        "}"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::TypeMismatch);
    REQUIRE(diagnostic.line(parser.source()) == 5);
    REQUIRE(diagnostic.column(parser.source()) == 13);
}

TEST_CASE("JsonParser diagnostic preserves descriptive comment", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"missing":42})"
    };

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(parser.diagnostic().comment() == "Unknown JSON object member");
}

TEST_CASE("JsonParser diagnostic range identifies full offending token", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":"wrong"})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto range = parser.diagnostic().range();

    REQUIRE(range.view(parser.source()) == R"("wrong")");
}

TEST_CASE("JsonParser unknown member diagnostic range identifies full key token", "[job_json][parser][diagnostic]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"missing":42})"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto range = parser.diagnostic().range();

    REQUIRE(range.view(parser.source()) == R"("missing")");
}

TEST_CASE("JsonParser EOF diagnostic range is empty at source end", "[job_json][parser][diagnostic]")
{
    std::vector<int> value;

    job::json::JsonParser parser{
        "[1,2"
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto range = parser.diagnostic().range();

    REQUIRE(range.begin() == parser.source().size());
    REQUIRE(range.end() == parser.source().size());
    REQUIRE(range.view(parser.source()).empty());
}

