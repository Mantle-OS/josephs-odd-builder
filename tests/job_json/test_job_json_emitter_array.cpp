#include <catch2/catch_test_macros.hpp>

#include <array>
#include <set>
#include <string>
#include <vector>

#include <job_json_emitter.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonEmitter emits empty vector", "[job_json][emitter][array]")
{
    const std::vector<int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[]");
}

TEST_CASE("JsonEmitter emits integer vector", "[job_json][emitter][array]")
{
    const std::vector<int> value{1, 2, 3, 4};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,2,3,4]");
}

TEST_CASE("JsonEmitter emits string vector", "[job_json][emitter][array]")
{
    const std::vector<std::string> value{
        "JOB",
        "Cake Court",
        "JSON"
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(["JOB","Cake Court","JSON"])");
}

TEST_CASE("JsonEmitter escapes strings inside vector", "[job_json][emitter][array]")
{
    const std::vector<std::string> value{
        "Cake\nCourt",
        "JOB"
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"(["Cake\nCourt","JOB"])");
}

TEST_CASE("JsonEmitter emits optional elements inside vector", "[job_json][emitter][array]")
{
    const std::vector<std::optional<int>> value{
        1,
        std::nullopt,
        3
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,null,3]");
}

TEST_CASE("JsonEmitter emits nested vectors", "[job_json][emitter][array]")
{
    const std::vector<std::vector<int>> value{
        {1, 2},
        {3, 4},
        {}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[[1,2],[3,4],[]]");
}

TEST_CASE("JsonEmitter emits reflected objects inside vector", "[job_json][emitter][array]")
{
    std::vector<job::json::tests::ParserNestedObject> value(2);

    value[0].port = 8080;
    value[0].host = "localhost";

    value[1].port = 9090;
    value[1].host = "job";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"([{"port":8080,"host":"localhost"},{"port":9090,"host":"job"}])");
}

TEST_CASE("JsonEmitter emits fixed integer array", "[job_json][emitter][array]")
{
    const std::array<int, 4> value{
        10,
        20,
        30,
        40
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[10,20,30,40]");
}

TEST_CASE("JsonEmitter emits zero length fixed array", "[job_json][emitter][array]")
{
    const std::array<int, 0> value{};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[]");
}

TEST_CASE("JsonEmitter emits nested fixed arrays", "[job_json][emitter][array]")
{
    const std::array<std::array<int, 2>, 2> value{{
        {{1, 2}},
        {{3, 4}}
    }};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[[1,2],[3,4]]");
}

TEST_CASE("JsonEmitter emits insert sequence in iteration order", "[job_json][emitter][array]")
{
    const std::set<int> value{
        4,
        1,
        3,
        2
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,2,3,4]");
}

TEST_CASE("JsonEmitter emits empty insert sequence", "[job_json][emitter][array]")
{
    const std::set<int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[]");
}

TEST_CASE("JsonEmitter emits nested arrays and objects", "[job_json][emitter][array]")
{
    std::vector<std::vector<job::json::tests::ParserNestedObject>> value(2);

    value[0].resize(1);
    value[0][0].port = 8001;
    value[0][0].host = "one";

    value[1].resize(2);

    value[1][0].port = 8002;
    value[1][0].host = "two";

    value[1][1].port = 8003;
    value[1][1].host = "three";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(
        output ==
        R"([[{"port":8001,"host":"one"}],[{"port":8002,"host":"two"},{"port":8003,"host":"three"}]])");
}

TEST_CASE("JsonEmitter emits array object member", "[job_json][emitter][array]")
{
    job::json::tests::ParserRootObject value;

    value.id = 7;
    value.values = {10, 20, 30};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("values":[10,20,30])") != std::string::npos);
}

TEST_CASE("JsonEmitter emits empty array object member", "[job_json][emitter][array]")
{
    job::json::tests::ParserRootObject value;

    value.values.clear();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output.find(R"("values":[])") != std::string::npos);
}

TEST_CASE("JsonEmitter appends array to existing sink", "[job_json][emitter][array]")
{
    const std::vector<int> value{
        1,
        2,
        3
    };

    std::string output = "prefix:";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "prefix:[1,2,3]");
}

TEST_CASE("JsonEmitter emits array without extra whitespace", "[job_json][emitter][array]")
{
    const std::vector<int> value{
        1,
        2,
        3
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,2,3]");

    REQUIRE(output.find(' ') == std::string::npos);
    REQUIRE(output.find('\n') == std::string::npos);
    REQUIRE(output.find('\t') == std::string::npos);
}

TEST_CASE("JsonEmitter emits mixed nested structure", "[job_json][emitter][array]")
{
    std::vector<job::json::tests::ParserRootObject> value(2);

    value[0].id = 1;
    value[0].server.port = 8001;
    value[0].server.host = "one";
    value[0].values = {1, 2, 3};

    value[1].id = 2;
    value[1].server.port = 8002;
    value[1].server.host = "two";
    value[1].values = {4, 5, 6};

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));

    REQUIRE(output.find(R"("id":1)") != std::string::npos);
    REQUIRE(output.find(R"("server":{"port":8001,"host":"one"})") != std::string::npos);
    REQUIRE(output.find(R"("values":[1,2,3])") != std::string::npos);

    REQUIRE(output.find(R"("id":2)") != std::string::npos);
    REQUIRE(output.find(R"("server":{"port":8002,"host":"two"})") != std::string::npos);
    REQUIRE(output.find(R"("values":[4,5,6])") != std::string::npos);
}

