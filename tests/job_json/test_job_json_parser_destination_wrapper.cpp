#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>

#include <job_json_parser_destination.h>

#include "test_job_json_fixtures.h"

TEST_CASE("JsonParserDestination null resets optional", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value = 42;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE(destination.null());
    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("JsonParserDestination null keeps disengaged optional disengaged", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE(destination.null());
    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("JsonParserDestination number engages optional integer", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination number replaces engaged optional integer", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value = 7;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination boolean engages optional boolean", "[job_json][parser_destination][wrapper]")
{
    std::optional<bool> value;

    job::json::JsonParserDestination<std::optional<bool>> destination{value};

    REQUIRE(destination.boolean(true));

    REQUIRE(value.has_value());
    REQUIRE(*value);
}

TEST_CASE("JsonParserDestination string engages optional owned string", "[job_json][parser_destination][wrapper]")
{
    std::optional<std::string> value;

    job::json::JsonParserDestination<std::optional<std::string>> destination{value};

    REQUIRE(destination.string("Cake Court"));

    REQUIRE(value.has_value());
    REQUIRE(*value == "Cake Court");
}

TEST_CASE("JsonParserDestination owned string engages optional owned string", "[job_json][parser_destination][wrapper]")
{
    std::optional<std::string> value;

    job::json::JsonParserDestination<std::optional<std::string>> destination{value};

    std::string source = "decoded Cake Court";

    REQUIRE(destination.stringOwned(std::move(source)));

    REQUIRE(value.has_value());
    REQUIRE(*value == "decoded Cake Court");
}

TEST_CASE("JsonParserDestination failed scalar conversion still engages default constructible optional", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE_FALSE(destination.number("potato"));

    REQUIRE(value.has_value());
    REQUIRE(*value == 0);
}

TEST_CASE("JsonParserDestination null resets shared pointer", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<int> value = std::make_shared<int>(42);

    job::json::JsonParserDestination<std::shared_ptr<int>> destination{value};

    REQUIRE(destination.null());
    REQUIRE_FALSE(value);
}

TEST_CASE("JsonParserDestination number allocates shared pointer", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<int> value;

    job::json::JsonParserDestination<std::shared_ptr<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination number reuses existing shared pointer", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<int> value = std::make_shared<int>(7);
    int *original = value.get();

    job::json::JsonParserDestination<std::shared_ptr<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.get() == original);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination owned string allocates shared pointer string", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<std::string> value;

    job::json::JsonParserDestination<std::shared_ptr<std::string>> destination{value};

    std::string source = "JOB";

    REQUIRE(destination.stringOwned(std::move(source)));

    REQUIRE(value);
    REQUIRE(*value == "JOB");
}

TEST_CASE("JsonParserDestination null resets unique pointer", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<int> value = std::make_unique<int>(42);

    job::json::JsonParserDestination<std::unique_ptr<int>> destination{value};

    REQUIRE(destination.null());
    REQUIRE_FALSE(value);
}

TEST_CASE("JsonParserDestination number allocates unique pointer", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<int> value;

    job::json::JsonParserDestination<std::unique_ptr<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination number reuses existing unique pointer", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<int> value = std::make_unique<int>(7);
    int *original = value.get();

    job::json::JsonParserDestination<std::unique_ptr<int>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.get() == original);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination string allocates unique pointer string", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<std::string> value;

    job::json::JsonParserDestination<std::unique_ptr<std::string>> destination{value};

    REQUIRE(destination.string("Cake Court"));

    REQUIRE(value);
    REQUIRE(*value == "Cake Court");
}

TEST_CASE("JsonParserDestination peels nested optionals", "[job_json][parser_destination][wrapper]")
{
    std::optional<std::optional<int>> value;

    job::json::JsonParserDestination<std::optional<std::optional<int>>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.has_value());
    REQUIRE(value->has_value());
    REQUIRE(**value == 42);
}

TEST_CASE("JsonParserDestination peels optional shared pointer", "[job_json][parser_destination][wrapper]")
{
    std::optional<std::shared_ptr<int>> value;

    job::json::JsonParserDestination<std::optional<std::shared_ptr<int>>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value.has_value());
    REQUIRE(*value);
    REQUIRE(**value == 42);
}

TEST_CASE("JsonParserDestination peels shared pointer optional", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<std::optional<int>> value;

    job::json::JsonParserDestination<std::shared_ptr<std::optional<int>>> destination{value};

    REQUIRE(destination.number("42"));

    REQUIRE(value);
    REQUIRE(value->has_value());
    REQUIRE(**value == 42);
}

TEST_CASE("JsonParserDestination peels unique pointer optional string", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<std::optional<std::string>> value;

    job::json::JsonParserDestination<std::unique_ptr<std::optional<std::string>>> destination{value};

    std::string source = "Cake Court";

    REQUIRE(destination.stringOwned(std::move(source)));

    REQUIRE(value);
    REQUIRE(value->has_value());
    REQUIRE(**value == "Cake Court");
}

TEST_CASE("JsonParserDestination peels deeply nested wrappers", "[job_json][parser_destination][wrapper]")
{
    using ValueType =
        std::optional<
            std::shared_ptr<
                std::optional<int>>>;

    ValueType value;

    job::json::JsonParserDestination<ValueType> destination{value};

    REQUIRE(destination.number("8675309"));

    REQUIRE(value.has_value());
    REQUIRE(*value);
    REQUIRE((*value)->has_value());
    REQUIRE(***value == 8675309);
}

TEST_CASE("JsonParserDestination nested null resets outer optional", "[job_json][parser_destination][wrapper]")
{
    using ValueType = std::optional<std::shared_ptr<int>>;

    ValueType value = std::make_shared<int>(42);

    job::json::JsonParserDestination<ValueType> destination{value};

    REQUIRE(destination.null());

    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("JsonParserDestination withValue exposes direct scalar destination", "[job_json][parser_destination][wrapper]")
{
    int value = 7;

    job::json::JsonParserDestination<int> destination{value};

    bool called = false;

    REQUIRE(destination.withValue([&](auto &inner) {
        called = true;

        REQUIRE(&inner.value() == &value);
        return inner.number("42");
    }));

    REQUIRE(called);
    REQUIRE(value == 42);
}

TEST_CASE("JsonParserDestination withValue engages optional", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    bool called = false;

    REQUIRE(destination.withValue([&](auto &inner) {
        called = true;

        REQUIRE(value.has_value());
        REQUIRE(&inner.value() == &*value);

        inner.value() = 42;
        return true;
    }));

    REQUIRE(called);
    REQUIRE(value.has_value());
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination withValue allocates shared pointer", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<int> value;

    job::json::JsonParserDestination<std::shared_ptr<int>> destination{value};

    REQUIRE(destination.withValue([&](auto &inner) {
        REQUIRE(value);
        REQUIRE(&inner.value() == value.get());

        inner.value() = 42;
        return true;
    }));

    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination withValue allocates unique pointer", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<int> value;

    job::json::JsonParserDestination<std::unique_ptr<int>> destination{value};

    REQUIRE(destination.withValue([&](auto &inner) {
        REQUIRE(value);
        REQUIRE(&inner.value() == value.get());

        inner.value() = 42;
        return true;
    }));

    REQUIRE(value);
    REQUIRE(*value == 42);
}

TEST_CASE("JsonParserDestination withValue preserves callback rejection", "[job_json][parser_destination][wrapper]")
{
    std::optional<int> value = 42;

    job::json::JsonParserDestination<std::optional<int>> destination{value};

    REQUIRE_FALSE(destination.withValue([](auto &inner) {
        inner.value() = 7;
        return false;
    }));

    REQUIRE(value.has_value());
    REQUIRE(*value == 7);
}

TEST_CASE("JsonParserDestination cannot materialize disengaged optional non default constructible value", "[job_json][parser_destination][wrapper]")
{
    std::optional<job::json::tests::NonDefaultConstructible> value;

    job::json::JsonParserDestination<
        std::optional<job::json::tests::NonDefaultConstructible>> destination{value};

    bool called = false;

    REQUIRE_FALSE(destination.withValue([&](auto &) {
        called = true;
        return true;
    }));

    REQUIRE_FALSE(called);
    REQUIRE_FALSE(value.has_value());
}

TEST_CASE("JsonParserDestination can expose engaged optional non default constructible value", "[job_json][parser_destination][wrapper]")
{
    std::optional<job::json::tests::NonDefaultConstructible> value{
        std::in_place,
        42
    };

    job::json::JsonParserDestination<
        std::optional<job::json::tests::NonDefaultConstructible>> destination{value};

    REQUIRE(destination.withValue([](auto &inner) {
        REQUIRE(inner.value().value == 42);
        inner.value().value = 84;
        return true;
    }));

    REQUIRE(value.has_value());
    REQUIRE(value->value == 84);
}

TEST_CASE("JsonParserDestination cannot materialize empty shared pointer non default constructible value", "[job_json][parser_destination][wrapper]")
{
    std::shared_ptr<job::json::tests::NonDefaultConstructible> value;

    job::json::JsonParserDestination<
        std::shared_ptr<job::json::tests::NonDefaultConstructible>> destination{value};

    bool called = false;

    REQUIRE_FALSE(destination.withValue([&](auto &) {
        called = true;
        return true;
    }));

    REQUIRE_FALSE(called);
    REQUIRE_FALSE(value);
}

TEST_CASE("JsonParserDestination can expose existing shared pointer non default constructible value", "[job_json][parser_destination][wrapper]")
{
    auto value = std::make_shared<job::json::tests::NonDefaultConstructible>(42);

    job::json::JsonParserDestination<
        std::shared_ptr<job::json::tests::NonDefaultConstructible>> destination{value};

    REQUIRE(destination.withValue([](auto &inner) {
        REQUIRE(inner.value().value == 42);
        inner.value().value = 84;
        return true;
    }));

    REQUIRE(value);
    REQUIRE(value->value == 84);
}

TEST_CASE("JsonParserDestination cannot materialize empty unique pointer non default constructible value", "[job_json][parser_destination][wrapper]")
{
    std::unique_ptr<job::json::tests::NonDefaultConstructible> value;

    job::json::JsonParserDestination<
        std::unique_ptr<job::json::tests::NonDefaultConstructible>> destination{value};

    bool called = false;

    REQUIRE_FALSE(destination.withValue([&](auto &) {
        called = true;
        return true;
    }));

    REQUIRE_FALSE(called);
    REQUIRE_FALSE(value);
}

TEST_CASE("JsonParserDestination can expose existing unique pointer non default constructible value", "[job_json][parser_destination][wrapper]")
{
    auto value = std::make_unique<job::json::tests::NonDefaultConstructible>(42);

    job::json::JsonParserDestination<
        std::unique_ptr<job::json::tests::NonDefaultConstructible>> destination{value};

    REQUIRE(destination.withValue([](auto &inner) {
        REQUIRE(inner.value().value == 42);
        inner.value().value = 84;
        return true;
    }));

    REQUIRE(value);
    REQUIRE(value->value == 84);
}

