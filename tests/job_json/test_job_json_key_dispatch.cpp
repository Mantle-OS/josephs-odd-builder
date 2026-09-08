#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

#include <job_json_key_dispatch.h>

#include "test_job_json_fixtures.h"


TEST_CASE("JsonKeyDispatch dispatches matching integer member", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "count", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, int>) {
            member = 42;
            return true;
        } else {
            return false;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(fixture.count == 42);
}

TEST_CASE("JsonKeyDispatch dispatches matching floating point member", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "ratio", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, float>) {
            member = 3.14f;
            return true;
        } else {
            return false;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(fixture.ratio == 3.14f);
}

TEST_CASE("JsonKeyDispatch dispatches matching boolean member", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "enabled", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, bool>) {
            member = true;
            return true;
        } else {
            return false;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(fixture.enabled);
}

TEST_CASE("JsonKeyDispatch dispatches matching string member", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "name", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, std::string>) {
            member = "Cake Court";
            return true;
        } else {
            return false;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(fixture.name == "Cake Court");
}

TEST_CASE("JsonKeyDispatch runtime key selects compile time member type", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    std::string_view selectedType;

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "count", [&](auto &member) {
            using MemberType = std::remove_cvref_t<decltype(member)>;

            if constexpr (std::same_as<MemberType, int>) {
                selectedType = "int";
            } else if constexpr (std::same_as<MemberType, float>) {
                selectedType = "float";
            } else if constexpr (std::same_as<MemberType, bool>) {
                selectedType = "bool";
            } else if constexpr (std::same_as<MemberType, std::string>) {
                selectedType = "string";
            }

            return true;
        }) == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(selectedType == "int");

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "ratio", [&](auto &member) {
            using MemberType = std::remove_cvref_t<decltype(member)>;

            if constexpr (std::same_as<MemberType, int>) {
                selectedType = "int";
            } else if constexpr (std::same_as<MemberType, float>) {
                selectedType = "float";
            } else if constexpr (std::same_as<MemberType, bool>) {
                selectedType = "bool";
            } else if constexpr (std::same_as<MemberType, std::string>) {
                selectedType = "string";
            }

            return true;
        }) == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(selectedType == "float");

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "enabled", [&](auto &member) {
            using MemberType = std::remove_cvref_t<decltype(member)>;

            if constexpr (std::same_as<MemberType, int>) {
                selectedType = "int";
            } else if constexpr (std::same_as<MemberType, float>) {
                selectedType = "float";
            } else if constexpr (std::same_as<MemberType, bool>) {
                selectedType = "bool";
            } else if constexpr (std::same_as<MemberType, std::string>) {
                selectedType = "string";
            }

            return true;
        }) == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(selectedType == "bool");

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "name", [&](auto &member) {
            using MemberType = std::remove_cvref_t<decltype(member)>;

            if constexpr (std::same_as<MemberType, int>) {
                selectedType = "int";
            } else if constexpr (std::same_as<MemberType, float>) {
                selectedType = "float";
            } else if constexpr (std::same_as<MemberType, bool>) {
                selectedType = "bool";
            } else if constexpr (std::same_as<MemberType, std::string>) {
                selectedType = "string";
            }

            return true;
        }) == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(selectedType == "string");
}

TEST_CASE("JsonKeyDispatch reports rejected when matching callback rejects", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "count", [](auto &) {
        return false;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Rejected);
    REQUIRE(fixture.count == 0);
}

TEST_CASE("JsonKeyDispatch reports not found for unknown key", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    bool called = false;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "missing", [&](auto &) {
        called = true;
        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::NotFound);
    REQUIRE_FALSE(called);
}

TEST_CASE("JsonKeyDispatch invokes callback exactly once", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    std::size_t calls = 0;
    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "ratio", [&](auto &) {
        ++calls;
        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(calls == 1);
}

TEST_CASE("JsonKeyDispatch supports void callback", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "enabled", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, bool>) {
            member = true;
        }
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(fixture.enabled);
}

TEST_CASE("JsonKeyDispatch passes actual member by reference", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    int *memberAddress = nullptr;

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "count", [&](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, int>) {
            memberAddress = &member;
        }

        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(memberAddress == &fixture.count);
}

TEST_CASE("JsonKeyDispatch preserves unrelated members", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture {
        .count = 7,
        .ratio = 1.5f,
        .enabled = false,
        .name = "before"
    };

    const auto result = job::json::JsonKeyDispatch::dispatch(fixture, "name", [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, std::string>) {
            member = "after";
        }

        return true;
    });

    REQUIRE(result == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(fixture.count == 7);
    REQUIRE(fixture.ratio == 1.5f);
    REQUIRE_FALSE(fixture.enabled);
    REQUIRE(fixture.name == "after");
}

TEST_CASE("JsonKeyDispatch key matching is exact", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "Count", [](auto &) {
            return true;
        }) == job::json::JsonKeyDispatchResult::NotFound);

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "count ", [](auto &) {
            return true;
        }) == job::json::JsonKeyDispatchResult::NotFound);

    REQUIRE(
        job::json::JsonKeyDispatch::dispatch(fixture, "coun", [](auto &) {
            return true;
        }) == job::json::JsonKeyDispatchResult::NotFound);
}

TEST_CASE("JsonKeyDispatch can mutate multiple member types through one generic callback", "[job_json][key_dispatch]")
{
    job::json::tests::DispatchFixture fixture;

    const auto assign = [](auto &member) {
        using MemberType = std::remove_cvref_t<decltype(member)>;

        if constexpr (std::same_as<MemberType, int>) {
            member = 64;
        } else if constexpr (std::same_as<MemberType, float>) {
            member = 2.5f;
        } else if constexpr (std::same_as<MemberType, bool>) {
            member = true;
        } else if constexpr (std::same_as<MemberType, std::string>) {
            member = "JOB";
        }

        return true;
    };

    REQUIRE(job::json::JsonKeyDispatch::dispatch(fixture, "count", assign)   == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(job::json::JsonKeyDispatch::dispatch(fixture, "ratio", assign)   == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(job::json::JsonKeyDispatch::dispatch(fixture, "enabled", assign) == job::json::JsonKeyDispatchResult::Accepted);
    REQUIRE(job::json::JsonKeyDispatch::dispatch(fixture, "name", assign)    == job::json::JsonKeyDispatchResult::Accepted);

    REQUIRE(fixture.count == 64);
    REQUIRE(fixture.ratio == 2.5f);
    REQUIRE(fixture.enabled);
    REQUIRE(fixture.name == "JOB");
}

