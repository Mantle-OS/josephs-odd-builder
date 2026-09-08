#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

#include <job_json_emitter.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonEmitter emits empty reflected object", "[job_json][emitter][object]")
{
    struct EmptyObject
    {
    };

    const EmptyObject value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "{}");
}

TEST_CASE("JsonEmitter emits scalar reflected object", "[job_json][emitter][object]")
{
    job::json::tests::ParserScalarObject value;

    value.count = 42;
    value.ratio = 3.5f;
    value.enabled = true;
    value.name = "Cake Court";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"count":42,"ratio":3.5,"enabled":true,"name":"Cake Court","optionalCount":null})");
}

TEST_CASE("JsonEmitter emits reflected members in declaration order", "[job_json][emitter][object]")
{
    struct OrderedObject
    {
        int first{};
        int second{};
        int third{};
    };

    OrderedObject value;
    value.first = 1;
    value.second = 2;
    value.third = 3;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"first":1,"second":2,"third":3})");
}

TEST_CASE("JsonEmitter emits engaged optional object member", "[job_json][emitter][object]")
{
    job::json::tests::ParserScalarObject value;

    value.optionalCount = 42;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("optionalCount":42)") != std::string::npos);
}

TEST_CASE("JsonEmitter emits disengaged optional object member as null", "[job_json][emitter][object]")
{
    job::json::tests::ParserScalarObject value;

    value.optionalCount.reset();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("optionalCount":null)") != std::string::npos);
}

TEST_CASE("JsonEmitter emits escaped reflected string member", "[job_json][emitter][object]")
{
    job::json::tests::ParserScalarObject value;

    value.name = "Cake\n\"Court\"";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("name":"Cake\n\"Court\"")") != std::string::npos);
}

TEST_CASE("JsonEmitter emits nested reflected object", "[job_json][emitter][object]")
{
    job::json::tests::ParserRootObject value;

    value.id = 7;
    value.server.port = 8080;
    value.server.host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output.find(R"("id":7)") != std::string::npos);
    REQUIRE(output.find(R"("server":{"port":8080,"host":"localhost"})") != std::string::npos);
}

TEST_CASE("JsonEmitter emits optional nested reflected object", "[job_json][emitter][object]")
{
    job::json::tests::ParserRootObject value;

    value.optionalServer.emplace();
    value.optionalServer->port = 9090;
    value.optionalServer->host = "job";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("optionalServer":{"port":9090,"host":"job"})") != std::string::npos);
}

TEST_CASE("JsonEmitter emits null optional nested reflected object", "[job_json][emitter][object]")
{
    job::json::tests::ParserRootObject value;

    value.optionalServer.reset();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("optionalServer":null)") != std::string::npos);
}

TEST_CASE("JsonEmitter emits vector object member", "[job_json][emitter][object]")
{
    job::json::tests::ParserRootObject value;

    value.values = {1, 2, 3};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("values":[1,2,3])") != std::string::npos);
}

TEST_CASE("JsonEmitter emits empty vector object member", "[job_json][emitter][object]")
{
    job::json::tests::ParserRootObject value;

    value.values.clear();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("values":[])") != std::string::npos);
}

TEST_CASE("JsonEmitter emits deeply nested reflected objects", "[job_json][emitter][object]")
{
    job::json::tests::ParserOuter value;

    value.middle.leaf.value = 8675309;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"middle":{"leaf":{"value":8675309}}})");
}

TEST_CASE("JsonEmitter emits round trip fixture", "[job_json][emitter][object]")
{
    job::json::tests::JsonRoundTripFixture value;

    value.id = 42;
    value.ratio = 1.25;
    value.enabled = true;
    value.name = "JOB";
    value.optionalCount = 7;
    value.values = {10, 20, 30};
    value.nested.port = 8080;
    value.nested.host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output == R"({"id":42,"ratio":1.25,"enabled":true,"name":"JOB","optionalCount":7,"values":[10,20,30],"nested":{"port":8080,"host":"localhost"}})");
}

TEST_CASE("JsonEmitter emits null optional in round trip fixture", "[job_json][emitter][object]")
{
    job::json::tests::JsonRoundTripFixture value;

    value.id = 42;
    value.ratio = 1.25;
    value.enabled = false;
    value.name = "JOB";
    value.optionalCount.reset();
    value.values = {};
    value.nested.port = 9000;
    value.nested.host = "job";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output == R"({"id":42,"ratio":1.25,"enabled":false,"name":"JOB","optionalCount":null,"values":[],"nested":{"port":9000,"host":"job"}})");
}

TEST_CASE("JsonEmitter appends reflected object to existing sink", "[job_json][emitter][object]")
{
    job::json::tests::ParserNestedObject value;

    value.port = 8080;
    value.host = "localhost";

    std::string output = "prefix:";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(prefix:{"port":8080,"host":"localhost"})");
}

TEST_CASE("JsonEmitter emits object without extra whitespace", "[job_json][emitter][object]")
{
    job::json::tests::ParserNestedObject value;

    value.port = 8080;
    value.host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"port":8080,"host":"localhost"})");

    REQUIRE(output.find(' ') == std::string::npos);
    REQUIRE(output.find('\n') == std::string::npos);
    REQUIRE(output.find('\t') == std::string::npos);
}

