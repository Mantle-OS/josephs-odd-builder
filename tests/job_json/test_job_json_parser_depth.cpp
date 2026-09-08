#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <vector>

#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonParser depth limit does not affect root scalar", "[job_json][parser][depth]")
{
    int value{};

    job::json::JsonParser parser{"42", 0};

    REQUIRE(parser.parse(value));
    REQUIRE(value == 42);
    REQUIRE_FALSE(parser.diagnostic().hasError());
}

TEST_CASE("JsonParser depth zero rejects root object", "[job_json][parser][depth]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{"{}", 0};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth zero rejects root array", "[job_json][parser][depth]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[]", 0};

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth one accepts root object", "[job_json][parser][depth]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{R"({"count":42})", 1};

    REQUIRE(parser.parse(value));
    REQUIRE(value.count == 42);
}

TEST_CASE("JsonParser depth one accepts root array", "[job_json][parser][depth]")
{
    std::vector<int> value;

    job::json::JsonParser parser{"[1,2,3]", 1};

    REQUIRE(parser.parse(value));
    REQUIRE(value == std::vector<int>{1, 2, 3});
}

TEST_CASE("JsonParser depth one rejects nested object", "[job_json][parser][depth]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"server":{"port":8080,"host":"localhost"}})",
        1
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth two accepts nested object", "[job_json][parser][depth]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"server":{"port":8080,"host":"localhost"}})",
        2
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.server.port == 8080);
    REQUIRE(value.server.host == "localhost");
}

TEST_CASE("JsonParser depth one rejects nested array", "[job_json][parser][depth]")
{
    std::vector<std::vector<int>> value;

    job::json::JsonParser parser{
        "[[1,2]]",
        1
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth two accepts nested array", "[job_json][parser][depth]")
{
    std::vector<std::vector<int>> value;

    job::json::JsonParser parser{
        "[[1,2],[3,4]]",
        2
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 2);
    REQUIRE(value[0] == std::vector<int>{1, 2});
    REQUIRE(value[1] == std::vector<int>{3, 4});
}

TEST_CASE("JsonParser depth counts object containing array", "[job_json][parser][depth]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"values":[1,2,3]})",
        1
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth two accepts object containing array", "[job_json][parser][depth]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"values":[1,2,3]})",
        2
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.values == std::vector<int>{1, 2, 3});
}

TEST_CASE("JsonParser depth counts array containing objects", "[job_json][parser][depth]")
{
    std::vector<job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"([{"port":8080,"host":"localhost"}])",
        1
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth two accepts array containing objects", "[job_json][parser][depth]")
{
    std::vector<job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"([{"port":8080,"host":"localhost"}])",
        2
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.size() == 1);
    REQUIRE(value[0].port == 8080);
    REQUIRE(value[0].host == "localhost");
}

TEST_CASE("JsonParser depth three accepts three reflected object levels", "[job_json][parser][depth]")
{
    job::json::tests::ParserOuter value;

    job::json::JsonParser parser{
        R"({"middle":{"leaf":{"value":42}}})",
        3
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.middle.leaf.value == 42);
}

TEST_CASE("JsonParser depth two rejects three reflected object levels", "[job_json][parser][depth]")
{
    job::json::tests::ParserOuter value;

    job::json::JsonParser parser{
        R"({"middle":{"leaf":{"value":42}}})",
        2
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser optional scalar wrapper does not consume depth", "[job_json][parser][depth]")
{
    std::optional<int> value;

    job::json::JsonParser parser{"42", 0};

    REQUIRE(parser.parse(value));
    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParser shared pointer scalar wrapper does not consume depth", "[job_json][parser][depth]")
{
    std::shared_ptr<int> value;

    job::json::JsonParser parser{"42", 0};

    REQUIRE(parser.parse(value));
    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParser unique pointer scalar wrapper does not consume depth", "[job_json][parser][depth]")
{
    std::unique_ptr<int> value;

    job::json::JsonParser parser{"42", 0};

    REQUIRE(parser.parse(value));
    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParser optional object wrapper does not add structural depth", "[job_json][parser][depth]")
{
    std::optional<job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"({"port":8080,"host":"localhost"})",
        1
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value.has_value());
    REQUIRE(value->port == 8080);
    REQUIRE(value->host == "localhost");
}

TEST_CASE("JsonParser shared pointer object wrapper does not add structural depth", "[job_json][parser][depth]")
{
    std::shared_ptr<job::json::tests::ParserNestedObject> value;

    job::json::JsonParser parser{
        R"({"port":8080,"host":"localhost"})",
        1
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value);
    REQUIRE(value->port == 8080);
    REQUIRE(value->host == "localhost");
}

TEST_CASE("JsonParser nested wrappers do not add structural depth", "[job_json][parser][depth]")
{
    std::optional<std::shared_ptr<std::optional<job::json::tests::ParserNestedObject>>> value;

    job::json::JsonParser parser{
        R"({"port":8080,"host":"localhost"})",
        1
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.has_value());
    REQUIRE(*value);
    REQUIRE((*value)->has_value());

    REQUIRE((*value)->value().port == 8080);
    REQUIRE((*value)->value().host == "localhost");
}

TEST_CASE("JsonParser nested wrappers cannot bypass structural depth limit", "[job_json][parser][depth]")
{
    std::optional<std::shared_ptr<std::optional<job::json::tests::ParserNestedObject>>> value;

    job::json::JsonParser parser{
        R"({"port":8080,"host":"localhost"})",
        0
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser depth diagnostic points at rejected root object", "[job_json][parser][depth]")
{
    job::json::tests::ParserScalarObject value;

    job::json::JsonParser parser{"{}", 0};

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
    REQUIRE(diagnostic.offset() == 0);
    REQUIRE(diagnostic.range().view(parser.source()) == "{");
}

TEST_CASE("JsonParser depth diagnostic points at rejected nested object", "[job_json][parser][depth]")
{
    job::json::tests::ParserRootObject value;

    job::json::JsonParser parser{
        R"({"server":{"port":8080,"host":"localhost"}})",
        1
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
    REQUIRE(diagnostic.range().view(parser.source()) == "{");
    REQUIRE(diagnostic.offset() == 10);
}

TEST_CASE("JsonParser depth diagnostic points at rejected nested array", "[job_json][parser][depth]")
{
    std::vector<std::vector<int>> value;

    job::json::JsonParser parser{
        "[[1,2]]",
        1
    };

    REQUIRE_FALSE(parser.parse(value));

    const auto &diagnostic = parser.diagnostic();

    REQUIRE(diagnostic.code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
    REQUIRE(diagnostic.offset() == 1);
    REQUIRE(diagnostic.range().view(parser.source()) == "[");
}

TEST_CASE("JsonParser depth limit applies equally through mixed structures", "[job_json][parser][depth]")
{
    std::vector<std::vector<job::json::tests::ParserNestedObject>> value;

    job::json::JsonParser parser{
        R"([[{"port":8080,"host":"localhost"}]])",
        2
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

TEST_CASE("JsonParser sufficient depth accepts mixed structures", "[job_json][parser][depth]")
{
    std::vector<std::vector<job::json::tests::ParserNestedObject>> value;

    job::json::JsonParser parser{
        R"([[{"port":8080,"host":"localhost"}]])",
        3
    };

    REQUIRE(parser.parse(value));

    REQUIRE(value.size() == 1);
    REQUIRE(value[0].size() == 1);
    REQUIRE(value[0][0].port == 8080);
    REQUIRE(value[0][0].host == "localhost");
}

TEST_CASE("JsonParser maximum depth boundary accepts exactly allowed nesting", "[job_json][parser][depth]")
{
    std::vector<std::vector<std::vector<int>>> value;

    job::json::JsonParser parser{
        "[[[42]]]",
        3
    };

    REQUIRE(parser.parse(value));
    REQUIRE(value[0][0][0] == 42);
}

TEST_CASE("JsonParser maximum depth boundary rejects one level beyond allowance", "[job_json][parser][depth]")
{
    std::vector<std::vector<std::vector<int>>> value;

    job::json::JsonParser parser{
        "[[[42]]]",
        2
    };

    REQUIRE_FALSE(parser.parse(value));
    REQUIRE(parser.diagnostic().code() == job::json::JsonDiagnosticCode::MaxDepthExceeded);
}

