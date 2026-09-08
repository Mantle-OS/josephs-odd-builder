#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <job_json_emitter.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonEmitter emits disengaged optional as null", "[job_json][emitter][wrapper]")
{
    const std::optional<int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits engaged optional scalar", "[job_json][emitter][wrapper]")
{
    const std::optional<int> value = 42;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits engaged optional boolean", "[job_json][emitter][wrapper]")
{
    const std::optional<bool> value = true;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "true");
}

TEST_CASE("JsonEmitter emits engaged optional string", "[job_json][emitter][wrapper]")
{
    const std::optional<std::string> value = "Cake Court";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake Court")");
}

TEST_CASE("JsonEmitter escapes engaged optional string", "[job_json][emitter][wrapper]")
{
    const std::optional<std::string> value = "Cake\nCourt";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake\nCourt")");
}

TEST_CASE("JsonEmitter emits null shared pointer as null", "[job_json][emitter][wrapper]")
{
    const std::shared_ptr<int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits shared pointer scalar", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_shared<int>(42);

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits shared pointer string", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_shared<std::string>("JOB");

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("JOB")");
}

TEST_CASE("JsonEmitter emits null unique pointer as null", "[job_json][emitter][wrapper]")
{
    const std::unique_ptr<int> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits unique pointer scalar", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_unique<int>(42);

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits unique pointer string", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_unique<std::string>("JOB");

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("JOB")");
}

TEST_CASE("JsonEmitter emits optional reflected object", "[job_json][emitter][wrapper]")
{
    std::optional<job::json::tests::ParserNestedObject> value;

    value.emplace();
    value->port = 8080;
    value->host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"port":8080,"host":"localhost"})");
}

TEST_CASE("JsonEmitter emits shared pointer reflected object", "[job_json][emitter][wrapper]")
{
    auto value = std::make_shared<job::json::tests::ParserNestedObject>();

    value->port = 8080;
    value->host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"port":8080,"host":"localhost"})");
}

TEST_CASE("JsonEmitter emits unique pointer reflected object", "[job_json][emitter][wrapper]")
{
    auto value = std::make_unique<job::json::tests::ParserNestedObject>();

    value->port = 8080;
    value->host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"port":8080,"host":"localhost"})");
}

TEST_CASE("JsonEmitter emits nested optionals", "[job_json][emitter][wrapper]")
{
    std::optional<std::optional<int>> value;

    value.emplace(42);

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits engaged outer optional with disengaged inner optional as null", "[job_json][emitter][wrapper]")
{
    std::optional<std::optional<int>> value;

    value.emplace();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits optional shared pointer scalar", "[job_json][emitter][wrapper]")
{
    std::optional<std::shared_ptr<int>> value;

    value = std::make_shared<int>(42);

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits engaged optional containing null shared pointer as null", "[job_json][emitter][wrapper]")
{
    std::optional<std::shared_ptr<int>> value;

    value.emplace();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits shared pointer optional scalar", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_shared<std::optional<int>>(42);

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "42");
}

TEST_CASE("JsonEmitter emits shared pointer disengaged optional as null", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_shared<std::optional<int>>();

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits unique pointer optional string", "[job_json][emitter][wrapper]")
{
    auto value = std::make_unique<std::optional<std::string>>();

    *value = "Cake Court";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"("Cake Court")");
}

TEST_CASE("JsonEmitter recursively peels deeply nested wrappers", "[job_json][emitter][wrapper]")
{
    std::optional<
        std::shared_ptr<
            std::optional<
                job::json::tests::ParserNestedObject>>> value;

    value.emplace(
        std::make_shared<
            std::optional<
                job::json::tests::ParserNestedObject>>());

    (*value)->emplace();
    (*value)->value().port = 8080;
    (*value)->value().host = "localhost";

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == R"({"port":8080,"host":"localhost"})");
}

TEST_CASE("JsonEmitter deeply nested wrapper null propagates to JSON null", "[job_json][emitter][wrapper]")
{
    std::optional<
        std::shared_ptr<
            std::optional<int>>> value;

    value.emplace(std::make_shared<std::optional<int>>());

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter null outer wrapper stops recursive peeling", "[job_json][emitter][wrapper]")
{
    const std::optional<
        std::shared_ptr<
            std::optional<int>>> value;

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "null");
}

TEST_CASE("JsonEmitter emits optional vector", "[job_json][emitter][wrapper]")
{
    const std::optional<std::vector<int>> value{
        std::in_place,
        std::initializer_list<int>{1, 2, 3}
    };

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,2,3]");
}

TEST_CASE("JsonEmitter emits shared pointer vector", "[job_json][emitter][wrapper]")
{
    const auto value = std::make_shared<std::vector<int>>(
        std::initializer_list<int>{1, 2, 3});

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,2,3]");
}

TEST_CASE("JsonEmitter emits vector of optional values", "[job_json][emitter][wrapper]")
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

TEST_CASE("JsonEmitter emits vector of shared pointer values", "[job_json][emitter][wrapper]")
{
    std::vector<std::shared_ptr<int>> value;

    value.push_back(std::make_shared<int>(1));
    value.push_back(nullptr);
    value.push_back(std::make_shared<int>(3));

    std::string output;

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "[1,null,3]");
}

TEST_CASE("JsonEmitter appends wrapped value to existing sink", "[job_json][emitter][wrapper]")
{
    const std::optional<int> value = 42;

    std::string output = "prefix:";

    REQUIRE(job::json::JsonEmitter::emit(value, output));
    REQUIRE(output == "prefix:42");
}

