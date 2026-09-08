#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <optional>
#include <string>

#include <job_json_parser_destination.h>

#include "test_job_json_fixtures.h"

TEST_CASE("JsonObjectParserDestination exposes backing object", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(&destination.object() == &object);

    destination.object().count = 42;

    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination dispatches matching member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    int *memberAddress = nullptr;

    const auto result = destination.dispatch("count", [&](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, int>) {
            memberAddress = &member;
            member = 42;
            return true;
        } else {
            return false;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(memberAddress == &object.count);
    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination dispatch returns not found for unknown member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    bool called = false;

    const auto result = destination.dispatch("missing", [&](auto &) {
        called = true;
        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::NotFound);
    REQUIRE_FALSE(called);
}

TEST_CASE("JsonObjectParserDestination dispatch preserves callback rejection", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    const auto result = destination.dispatch("count", [](auto &) {
        return false;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Rejected);
}

TEST_CASE("JsonObjectParserDestination assigns number by key", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.number("count", "42") == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination rejects number for wrong member type", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.name = "before";

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.number("name", "42") == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.name == "before");
}

TEST_CASE("JsonObjectParserDestination reports not found for number unknown key", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.number("missing", "42") == job::json::JsonKeyDispatchResult::NotFound);
}

TEST_CASE("JsonObjectParserDestination assigns boolean by key", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.boolean("enabled", true) == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.enabled);
}

TEST_CASE("JsonObjectParserDestination rejects boolean for wrong member type", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.count = 7;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.boolean("count", true) == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.count == 7);
}

TEST_CASE("JsonObjectParserDestination assigns borrowed string by key", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.string("name", "Cake Court") == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.name == "Cake Court");
}

TEST_CASE("JsonObjectParserDestination rejects borrowed string for wrong member type", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.count = 42;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.string("count", "Cake Court") == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination assigns owned string by key", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    std::string value = "decoded Cake Court";

    REQUIRE(destination.stringOwned("name", std::move(value)) == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.name == "decoded Cake Court");
}

TEST_CASE("JsonObjectParserDestination rejects owned string for wrong member type", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.count = 42;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    std::string value = "decoded Cake Court";

    REQUIRE(destination.stringOwned("count", std::move(value)) == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination null resets optional member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.optionalCount = 42;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.null("optionalCount") == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE_FALSE(object.optionalCount.has_value());
}

TEST_CASE("JsonObjectParserDestination null resets shared pointer member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.sharedCount = std::make_shared<int>(42);

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.null("sharedCount") == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE_FALSE(object.sharedCount);
}

TEST_CASE("JsonObjectParserDestination null rejects scalar member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;
    object.count = 42;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.null("count") == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.count == 42);
}

TEST_CASE("JsonObjectParserDestination null reports unknown member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.null("missing") == job::json::JsonKeyDispatchResult::NotFound);
}

TEST_CASE("JsonObjectParserDestination number engages optional member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.number("optionalCount", "42") == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(object.optionalCount.has_value());
    REQUIRE(*object.optionalCount == 42);
}

TEST_CASE("JsonObjectParserDestination number allocates shared pointer member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    REQUIRE(destination.number("sharedCount", "42") == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(object.sharedCount);
    REQUIRE(*object.sharedCount == 42);
}

TEST_CASE("JsonObjectParserDestination withValue exposes direct nested object", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationNestedFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationNestedFixture> destination{object};

    const auto result = destination.withValue("server", [&](auto &inner) {
        REQUIRE(&inner.value() == &object.server);

        inner.value().port = 8080;
        inner.value().host = "localhost";

        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.server.port == 8080);
    REQUIRE(object.server.host == "localhost");
}

TEST_CASE("JsonObjectParserDestination withValue materializes optional nested object", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationNestedFixture object;

    REQUIRE_FALSE(object.optionalServer.has_value());

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationNestedFixture> destination{object};

    const auto result = destination.withValue("optionalServer", [&](auto &inner) {
        REQUIRE(object.optionalServer.has_value());
        REQUIRE(&inner.value() == &*object.optionalServer);

        inner.value().port = 9090;
        inner.value().host = "job";

        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(object.optionalServer.has_value());
    REQUIRE(object.optionalServer->port == 9090);
    REQUIRE(object.optionalServer->host == "job");
}

TEST_CASE("JsonObjectParserDestination withValue reports unknown nested member", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationNestedFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationNestedFixture> destination{object};

    bool called = false;

    const auto result = destination.withValue("missing", [&](auto &) {
        called = true;
        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::NotFound);
    REQUIRE_FALSE(called);
}

TEST_CASE("JsonObjectParserDestination withValue preserves nested callback rejection", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationNestedFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationNestedFixture> destination{object};

    const auto result = destination.withValue("server", [&](auto &inner) {
        inner.value().port = 8080;
        return false;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(object.server.port == 8080);
}

TEST_CASE("JsonObjectParserDestination distinguishes accepted rejected and not found", "[job_json][object_parser_destination]")
{
    job::json::tests::ObjectDestinationFixture object;

    job::json::JsonObjectParserDestination<
        job::json::tests::ObjectDestinationFixture> destination{object};

    const auto accepted = destination.number("count", "42");
    const auto rejected = destination.number("name", "42");
    const auto notFound = destination.number("missing", "42");

    REQUIRE(accepted == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(rejected == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(notFound == job::json::JsonKeyDispatchResult::NotFound);

    REQUIRE(accepted != rejected);
    REQUIRE(accepted != notFound);
    REQUIRE(rejected != notFound);
}

