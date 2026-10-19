
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>
#include <utility>

#include "test_job_object_fixtures.h"

namespace job::core::tests {

// =============================================================================
// Block One: Usage / Examples
// =============================================================================

TEST_CASE("JSON object macros serialize and deserialize typed JOB objects",
          "[core][object][json][usage]")
{
    SECTION("Read-write member round-trips a typed object")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture input;

        input.set_city("Chicago");
        input.set_limit(10);
        input.set_enabled(true);

        REQUIRE(fixture.write_jsonRw(input));

        const str &json = fixture.get_jsonRw();

        REQUIRE_FALSE(json.empty());

        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRw(output));
        REQUIRE(output.get_city() == "Chicago");
        REQUIRE(output.get_limit() == 10);
        REQUIRE(output.get_enabled());
    }

    SECTION("Required read-write member round-trips a typed object")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture input;

        input.set_city("New York");
        input.set_limit(25);
        input.set_enabled(false);

        REQUIRE(fixture.write_jsonRwRequired(input));

        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRwRequired(output));
        REQUIRE(output.get_city() == "New York");
        REQUIRE(output.get_limit() == 25);
        REQUIRE_FALSE(output.get_enabled());
    }

    SECTION("Read-only member exposes typed deserialization")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRo(output));
        REQUIRE(output.get_city().empty());
        REQUIRE(output.get_limit() == 0);
        REQUIRE_FALSE(output.get_enabled());
    }

    SECTION("Required read-only member exposes typed deserialization")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRoRequired(output));
        REQUIRE(output.get_city().empty());
        REQUIRE(output.get_limit() == 0);
        REQUIRE_FALSE(output.get_enabled());
    }

    SECTION("Const member exposes typed deserialization")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonConst(output));
        REQUIRE(output.get_city().empty());
        REQUIRE(output.get_limit() == 0);
        REQUIRE_FALSE(output.get_enabled());
    }

    SECTION("Required const member exposes typed deserialization")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonConstRequired(output));
        REQUIRE(output.get_city().empty());
        REQUIRE(output.get_limit() == 0);
        REQUIRE_FALSE(output.get_enabled());
    }
}

TEST_CASE("JSON object macros preserve ordinary member access",
          "[core][object][json][usage]")
{
    SECTION("Read-write member accepts raw JSON")
    {
        MacroJsonMembersFixture fixture;

        const str json = R"({"m_city":"Chicago","m_limit":10,"m_enabled":true})";

        fixture.set_jsonRw(json);

        REQUIRE(fixture.get_jsonRw() == json);

        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRw(output));
        REQUIRE(output.get_city() == "Chicago");
        REQUIRE(output.get_limit() == 10);
        REQUIRE(output.get_enabled());
    }

    SECTION("Required read-write member accepts raw JSON")
    {
        MacroJsonMembersFixture fixture;

        const str json = R"({"m_city":"Boston","m_limit":5,"m_enabled":false})";

        fixture.set_jsonRwRequired(json);

        REQUIRE(fixture.get_jsonRwRequired() == json);

        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRwRequired(output));
        REQUIRE(output.get_city() == "Boston");
        REQUIRE(output.get_limit() == 5);
        REQUIRE_FALSE(output.get_enabled());
    }

    SECTION("All six members expose their initial JSON")
    {
        MacroJsonMembersFixture fixture;

        REQUIRE(fixture.get_jsonRw() == "{}");
        REQUIRE(fixture.get_jsonRwRequired() == "{}");
        REQUIRE(fixture.get_jsonRo() == "{}");
        REQUIRE(fixture.get_jsonRoRequired() == "{}");
        REQUIRE(fixture.get_jsonConst() == "{}");
        REQUIRE(fixture.get_jsonConstRequired() == "{}");
    }
}

TEST_CASE("JSON object macros support nested JOB objects",
          "[core][object][json][usage]")
{
    MacroJsonMembersFixture fixture;
    MacroJsonNestedFixture input;

    input.set_name("weather");

    auto arguments = MacroJsonPayloadFixture::createUniq();

    REQUIRE(arguments != nullptr);

    arguments->set_city("Chicago");
    arguments->set_limit(10);
    arguments->set_enabled(true);

    input.set_arguments(std::move(arguments));

    REQUIRE(fixture.write_jsonRw(input));

    MacroJsonNestedFixture output;

    REQUIRE(fixture.read_jsonRw(output));
    REQUIRE(output.get_name() == "weather");
    REQUIRE(output.get_arguments() != nullptr);
    REQUIRE(output.get_arguments()->get_city() == "Chicago");
    REQUIRE(output.get_arguments()->get_limit() == 10);
    REQUIRE(output.get_arguments()->get_enabled());
}

// =============================================================================
// Block Two: Edge Cases / Breakage
// =============================================================================

TEST_CASE("JSON object macros reset to their declared defaults",
          "[core][object][json][edge]")
{
    SECTION("Read-write member resets after modification")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw(R"({"city":"Chicago"})");

        REQUIRE(fixture.get_jsonRw() != "{}");

        fixture.reset_jsonRw();

        REQUIRE(fixture.get_jsonRw() == "{}");
    }

    SECTION("Required read-write member resets after modification")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRwRequired(R"({"city":"Boston"})");

        REQUIRE(fixture.get_jsonRwRequired() != "{}");

        fixture.reset_jsonRwRequired();

        REQUIRE(fixture.get_jsonRwRequired() == "{}");
    }
}

TEST_CASE("JSON object macros reject invalid JSON without changing stored data",
          "[core][object][json][edge]")
{
    SECTION("Malformed JSON fails typed deserialization")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw(R"({"city":)");

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
        REQUIRE(fixture.get_jsonRw() == R"({"city":)");
    }

    SECTION("Unknown object members are rejected by the current parser")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw(R"({"unknown":42})");

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
    }

    SECTION("Type mismatch fails typed deserialization")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw(R"({"limit":"not-a-number"})");

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
    }

    SECTION("Failed typed deserialization does not change the stored JSON")
    {
        MacroJsonMembersFixture fixture;

        const str json = R"({"limit":"invalid"})";

        fixture.set_jsonRw(json);

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
        REQUIRE(fixture.get_jsonRw() == json);
    }
}

TEST_CASE("JSON object macros handle empty and null JSON values",
          "[core][object][json][edge]")
{
    SECTION("Empty JSON string is not a valid document")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw("");

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
    }

    SECTION("JSON null cannot deserialize into a concrete JOB object")
    {
        MacroJsonMembersFixture fixture;

        fixture.set_jsonRw("null");

        MacroJsonPayloadFixture output;

        REQUIRE_FALSE(fixture.read_jsonRw(output));
    }

    SECTION("Empty JSON object is accepted")
    {
        MacroJsonMembersFixture fixture;
        MacroJsonPayloadFixture output;

        REQUIRE(fixture.read_jsonRw(output));
        REQUIRE(output.get_city().empty());
        REQUIRE(output.get_limit() == 0);
        REQUIRE_FALSE(output.get_enabled());
    }
}

TEST_CASE("JSON object macros preserve previous values across successive writes",
          "[core][object][json][edge]")
{
    MacroJsonMembersFixture fixture;

    MacroJsonPayloadFixture first;
    first.set_city("Chicago");
    first.set_limit(10);

    REQUIRE(fixture.write_jsonRw(first));

    const str firstJson = fixture.get_jsonRw();

    MacroJsonPayloadFixture second;
    second.set_city("Boston");
    second.set_limit(20);

    REQUIRE(fixture.write_jsonRw(second));

    REQUIRE(fixture.get_jsonRw() != firstJson);

    MacroJsonPayloadFixture output;

    REQUIRE(fixture.read_jsonRw(output));
    REQUIRE(output.get_city() == "Boston");
    REQUIRE(output.get_limit() == 20);
}

} // namespace job::core::tests
