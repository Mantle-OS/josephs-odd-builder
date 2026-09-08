#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


        TEST_CASE("JsonParser parses empty object", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{"{}"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser parses scalar object members", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":42,"ratio":3.5,"enabled":true,"name":"Cake Court"})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.count == 42);
    REQUIRE(value.ratio == 3.5f);
    REQUIRE(value.enabled);
    REQUIRE(value.name == "Cake Court");
}

TEST_CASE("JsonParser parses object members with whitespace", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"(
        {
            "count"   : 42,
            "ratio"   : 1.5,
            "enabled" : true,
            "name"    : "JOB"
        }
        )"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.count == 42);
    REQUIRE(value.ratio == 1.5f);
    REQUIRE(value.enabled);
    REQUIRE(value.name == "JOB");
}

TEST_CASE("JsonParser parses object members independent of declaration order", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"name":"JOB","enabled":true,"ratio":2.5,"count":84})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.count == 84);
    REQUIRE(value.ratio == 2.5f);
    REQUIRE(value.enabled);
    REQUIRE(value.name == "JOB");
}

TEST_CASE("JsonParser parses optional object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"optionalCount":42})"};

    REQUIRE(parser.parse(value));

    REQUIRE(value.optionalCount.has_value());
    REQUIRE(*value.optionalCount == 42);
}

TEST_CASE("JsonParser parses null optional object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.optionalCount = 42;

    job::json::JsonParser parser{R"({"optionalCount":null})"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(value.optionalCount.has_value());
}

TEST_CASE("JsonParser decodes escaped object string value", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"name":"Cake\nCourt"})"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.name == "Cake\nCourt");
}

TEST_CASE("JsonParser decodes Unicode object string value", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"name":"\u004A\u004F\u0042"})"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.name == "JOB");
}

TEST_CASE("JsonParser parses nested object", "[job_json][parser][object]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"id":7,"server":{"port":8080,"host":"localhost"}})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.id == 7);
    REQUIRE(value.server.port == 8080);
    REQUIRE(value.server.host == "localhost");
}

TEST_CASE("JsonParser parses optional nested object", "[job_json][parser][object]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"optionalServer":{"port":9090,"host":"job"}})"
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.optionalServer.has_value());
    REQUIRE(value.optionalServer->port == 9090);
    REQUIRE(value.optionalServer->host == "job");
}

TEST_CASE("JsonParser resets optional nested object with null", "[job_json][parser][object]")
{
    job::json::tests::ParserRootObject value;
    value.optionalServer.emplace();
    value.optionalServer->port = 9090;
    value.optionalServer->host = "before";

    job::json::JsonParser parser{R"({"optionalServer":null})"};

    REQUIRE(parser.parse(value));
    REQUIRE_FALSE(value.optionalServer.has_value());
}

TEST_CASE("JsonParser parses deeply nested reflected objects", "[job_json][parser][object]")
{
    job::json::tests::ParserOuter value;

    job::json::JsonParser parser{
        R"({"middle":{"leaf":{"value":8675309}}})"
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.middle.leaf.value == 8675309);
}

TEST_CASE("JsonParser rejects unknown object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"missing":42})"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnknownMember);
    REQUIRE(parser.diagnostic().comment() == "Unknown JSON object member");
}

TEST_CASE("JsonParser reports unknown escaped object key after decoding", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"\u006Dissing":42})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnknownMember);
}

TEST_CASE("JsonParser matches escaped object key after decoding", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"\u0063ount":42})"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.count == 42);
}

TEST_CASE("JsonParser rejects number for string object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.name = "before";

    job::json::JsonParser parser{R"({"name":42})"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.name == "before");
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects string for integer object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.count = 7;

    job::json::JsonParser parser{R"({"count":"42"})"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.count == 7);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects boolean for integer object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.count = 7;

    job::json::JsonParser parser{R"({"count":true})"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.count == 7);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::TypeMismatch);
}

TEST_CASE("JsonParser rejects null for scalar object member", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.count = 7;

    job::json::JsonParser parser{R"({"count":null})"};

    REQUIRE_FALSE(parser.parse(value));

    REQUIRE(value.count == 7);
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidNull);
}

TEST_CASE("JsonParser rejects missing object key", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({:42})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedObjectKey);
}

TEST_CASE("JsonParser rejects non string object key", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({42:1})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedObjectKey);
}

TEST_CASE("JsonParser rejects missing name separator", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count" 42})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedNameSeparator);
}

TEST_CASE("JsonParser rejects missing value separator between object members", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{
        R"({"count":42 "enabled":true})"
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedValueSeparator);
}

TEST_CASE("JsonParser rejects trailing comma in object", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":42,})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedObjectKey);
}

TEST_CASE("JsonParser rejects object missing closing brace", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":42)"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects object ending after key", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count")"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::ExpectedNameSeparator);
}

TEST_CASE("JsonParser rejects object ending after name separator", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":)"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::UnexpectedEnd);
}

TEST_CASE("JsonParser rejects malformed object key escape", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"\q":42})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser rejects malformed object value token", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":0x10})"};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::InvalidToken);
}

TEST_CASE("JsonParser preserves members not present in payload", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;
    value.count = 7;
    value.ratio = 1.5f;
    value.enabled = false;
    value.name = "before";
    value.optionalCount = 99;

    job::json::JsonParser parser{R"({"count":42})"};

    REQUIRE(parser.parse(value));

    REQUIRE(value.count == 42);
    REQUIRE(value.ratio == 1.5f);
    REQUIRE_FALSE(value.enabled);
    REQUIRE(value.name == "before");
    REQUIRE(value.optionalCount.has_value());
    REQUIRE(*value.optionalCount == 99);
}

TEST_CASE("JsonParser later duplicate object member overwrites earlier value", "[job_json][parser][object]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":7,"count":42})"};

    REQUIRE(parser.parse(value));
    REQUIRE(value.count == 42);
}

TEST_CASE("JsonParser parses empty nested object", "[job_json][parser][object]")
{
    job::json::tests::ParserRootObject value;
    value.server.port = 42;
    value.server.host = "before";

    job::json::JsonParser parser{R"({"server":{}})"};

    REQUIRE(parser.parse(value));

    REQUIRE(value.server.port == 42);
    REQUIRE(value.server.host == "before");
}

