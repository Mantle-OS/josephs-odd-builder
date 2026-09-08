#include <catch2/catch_test_macros.hpp>

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <job_json_emitter.h>
#include <job_json_parser.h>

#include "test_job_json_fixtures.h"


TEST_CASE("Json round trip preserves scalar object", "[job_json][roundtrip]")
{
    job::json::tests::ParserScalarObject source;

    source.count = 42;
    source.ratio = 3.5f;
    source.enabled = true;
    source.name = "Cake Court";
    source.optionalCount = 7;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserScalarObject result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.count == source.count);
    REQUIRE(result.ratio == source.ratio);
    REQUIRE(result.enabled == source.enabled);
    REQUIRE(result.name == source.name);
    REQUIRE(result.optionalCount == source.optionalCount);
}

TEST_CASE("Json round trip preserves null optional object member", "[job_json][roundtrip]")
{
    job::json::tests::ParserScalarObject source;

    source.count = 42;
    source.ratio = 1.25f;
    source.enabled = false;
    source.name = "JOB";
    source.optionalCount.reset();

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserScalarObject result;
    result.optionalCount = 99;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.count == source.count);
    REQUIRE(result.ratio == source.ratio);
    REQUIRE(result.enabled == source.enabled);
    REQUIRE(result.name == source.name);
    REQUIRE_FALSE(result.optionalCount.has_value());
}

TEST_CASE("Json round trip preserves escaped string", "[job_json][roundtrip]")
{
    job::json::tests::ParserScalarObject source;

    source.name = "Cake\n\"Court\"\\JOB";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserScalarObject result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result.name == source.name);
}

TEST_CASE("Json round trip preserves UTF8 string", "[job_json][roundtrip]")
{
    job::json::tests::ParserScalarObject source;

    source.name = "Grüße 世界 😀";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserScalarObject result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result.name == source.name);
}

TEST_CASE("Json round trip preserves nested reflected object", "[job_json][roundtrip]")
{
    job::json::tests::ParserRootObject source;

    source.id = 7;
    source.server.port = 8080;
    source.server.host = "localhost";
    source.optionalServer.emplace();
    source.optionalServer->port = 9090;
    source.optionalServer->host = "job";
    source.values = {1, 2, 3, 4};

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserRootObject result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.id == source.id);

    REQUIRE(result.server.port == source.server.port);
    REQUIRE(result.server.host == source.server.host);

    REQUIRE(result.optionalServer.has_value());
    REQUIRE(result.optionalServer->port == source.optionalServer->port);
    REQUIRE(result.optionalServer->host == source.optionalServer->host);

    REQUIRE(result.values == source.values);
}

TEST_CASE("Json round trip preserves deeply nested reflected object", "[job_json][roundtrip]")
{
    job::json::tests::ParserOuter source;

    source.middle.leaf.value = 8675309;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::ParserOuter result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result.middle.leaf.value == source.middle.leaf.value);
}

TEST_CASE("Json round trip preserves vector", "[job_json][roundtrip]")
{
    const std::vector<int> source{
        1,
        2,
        3,
        4,
        5
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::vector<int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves nested vector", "[job_json][roundtrip]")
{
    const std::vector<std::vector<int>> source{
        {1, 2},
        {3, 4},
        {},
        {5, 6, 7}
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::vector<std::vector<int>> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves fixed array", "[job_json][roundtrip]")
{
    const std::array<int, 4> source{
        10,
        20,
        30,
        40
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::array<int, 4> result{};

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves vector of reflected objects", "[job_json][roundtrip]")
{
    std::vector<job::json::tests::ParserNestedObject> source(2);

    source[0].port = 8001;
    source[0].host = "one";

    source[1].port = 8002;
    source[1].host = "two";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::vector<job::json::tests::ParserNestedObject> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.size() == source.size());

    REQUIRE(result[0].port == source[0].port);
    REQUIRE(result[0].host == source[0].host);

    REQUIRE(result[1].port == source[1].port);
    REQUIRE(result[1].host == source[1].host);
}

TEST_CASE("Json round trip preserves string integer map", "[job_json][roundtrip]")
{
    const std::map<std::string, int> source{
        {"one", 1},
        {"two", 2},
        {"three", 3}
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::map<std::string, int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves integer string map", "[job_json][roundtrip]")
{
    const std::map<int, std::string> source{
        {1, "one"},
        {2, "two"},
        {3, "three"}
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::map<int, std::string> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves nested map", "[job_json][roundtrip]")
{
    const std::map<std::string, std::map<std::string, int>> source{
        {
            "outer",
            {
                {"one", 1},
                {"two", 2}
            }
        }
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::map<std::string, std::map<std::string, int>> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves unordered map semantically", "[job_json][roundtrip]")
{
    const std::unordered_map<std::string, int> source{
        {"one", 1},
        {"two", 2},
        {"three", 3}
    };

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::unordered_map<std::string, int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves optional scalar", "[job_json][roundtrip]")
{
    const std::optional<int> source = 42;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::optional<int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result == source);
}

TEST_CASE("Json round trip preserves disengaged optional", "[job_json][roundtrip]")
{
    const std::optional<int> source;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::optional<int> result = 42;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("Json round trip preserves shared pointer scalar value", "[job_json][roundtrip]")
{
    const auto source = std::make_shared<int>(42);

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::shared_ptr<int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result);
    REQUIRE(*result == *source);
}

TEST_CASE("Json round trip preserves null shared pointer", "[job_json][roundtrip]")
{
    const std::shared_ptr<int> source;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    auto result = std::make_shared<int>(42);

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE_FALSE(result);
}

TEST_CASE("Json round trip preserves unique pointer scalar value", "[job_json][roundtrip]")
{
    const auto source = std::make_unique<int>(42);

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::unique_ptr<int> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE(result);
    REQUIRE(*result == *source);
}

TEST_CASE("Json round trip preserves null unique pointer", "[job_json][roundtrip]")
{
    const std::unique_ptr<int> source;

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    auto result = std::make_unique<int>(42);

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));
    REQUIRE_FALSE(result);
}

TEST_CASE("Json round trip preserves deeply nested wrappers", "[job_json][roundtrip]")
{
    std::optional<
        std::shared_ptr<
            std::optional<
                job::json::tests::ParserNestedObject>>> source;

    source.emplace(
        std::make_shared<
            std::optional<
                job::json::tests::ParserNestedObject>>());

    (*source)->emplace();
    (*source)->value().port = 8080;
    (*source)->value().host = "localhost";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    std::optional<
        std::shared_ptr<
            std::optional<
                job::json::tests::ParserNestedObject>>> result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.has_value());
    REQUIRE(*result);
    REQUIRE((*result)->has_value());

    REQUIRE((*result)->value().port == 8080);
    REQUIRE((*result)->value().host == "localhost");
}

TEST_CASE("Json round trip preserves round trip fixture", "[job_json][roundtrip]")
{
    job::json::tests::JsonRoundTripFixture source;

    source.id = 42;
    source.ratio = 1.25;
    source.enabled = true;
    source.name = "Cake\nCourt 世界 😀";
    source.optionalCount = 7;
    source.values = {10, 20, 30, 40};
    source.nested.port = 8080;
    source.nested.host = "localhost";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::JsonRoundTripFixture result;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.id == source.id);
    REQUIRE(result.ratio == source.ratio);
    REQUIRE(result.enabled == source.enabled);
    REQUIRE(result.name == source.name);
    REQUIRE(result.optionalCount == source.optionalCount);
    REQUIRE(result.values == source.values);
    REQUIRE(result.nested.port == source.nested.port);
    REQUIRE(result.nested.host == source.nested.host);
}

TEST_CASE("Json round trip preserves null optional in round trip fixture", "[job_json][roundtrip]")
{
    job::json::tests::JsonRoundTripFixture source;

    source.id = 7;
    source.ratio = 3.5;
    source.enabled = false;
    source.name = "JOB";
    source.optionalCount.reset();
    source.values = {};
    source.nested.port = 9000;
    source.nested.host = "job";

    std::string json;

    REQUIRE(job::json::JsonEmitter::emit(source, json));

    job::json::tests::JsonRoundTripFixture result;
    result.optionalCount = 42;

    job::json::JsonParser parser{json};

    REQUIRE(parser.parse(result));

    REQUIRE(result.id == source.id);
    REQUIRE(result.ratio == source.ratio);
    REQUIRE(result.enabled == source.enabled);
    REQUIRE(result.name == source.name);
    REQUIRE_FALSE(result.optionalCount.has_value());
    REQUIRE(result.values.empty());
    REQUIRE(result.nested.port == source.nested.port);
    REQUIRE(result.nested.host == source.nested.host);
}

TEST_CASE("Json repeated round trip produces stable emitted representation", "[job_json][roundtrip]")
{
    job::json::tests::JsonRoundTripFixture source;

    source.id = 42;
    source.ratio = 1.25;
    source.enabled = true;
    source.name = "JOB";
    source.optionalCount = 7;
    source.values = {1, 2, 3};
    source.nested.port = 8080;
    source.nested.host = "localhost";

    std::string firstJson;

    REQUIRE(job::json::JsonEmitter::emit(source, firstJson));

    job::json::tests::JsonRoundTripFixture middle;

    job::json::JsonParser parser{firstJson};

    REQUIRE(parser.parse(middle));

    std::string secondJson;

    REQUIRE(job::json::JsonEmitter::emit(middle, secondJson));

    REQUIRE(secondJson == firstJson);
}

