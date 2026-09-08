#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_source_range.h>

        TEST_CASE("JsonSourceRange default constructs empty", "[job_json][source_range]")
{
    constexpr job::json::JsonSourceRange range;

    STATIC_REQUIRE(range.begin() == 0);
    STATIC_REQUIRE(range.end() == 0);
    STATIC_REQUIRE(range.size() == 0);
    STATIC_REQUIRE(range.empty());
    STATIC_REQUIRE(range.valid());
}

TEST_CASE("JsonSourceRange stores begin and end offsets", "[job_json][source_range]")
{
    constexpr job::json::JsonSourceRange range{3, 8};

    STATIC_REQUIRE(range.begin() == 3);
    STATIC_REQUIRE(range.end() == 8);
    STATIC_REQUIRE(range.size() == 5);
    STATIC_REQUIRE_FALSE(range.empty());
    STATIC_REQUIRE(range.valid());
}

TEST_CASE("JsonSourceRange detects invalid ordering", "[job_json][source_range]")
{
    constexpr job::json::JsonSourceRange range{8, 3};

    STATIC_REQUIRE_FALSE(range.valid());
}

TEST_CASE("JsonSourceRange validates against source", "[job_json][source_range]")
{
    constexpr std::string_view source = R"({"value":42})";

    constexpr job::json::JsonSourceRange whole{0, source.size()};
    constexpr job::json::JsonSourceRange middle{2, 7};
    constexpr job::json::JsonSourceRange atEnd{source.size(), source.size()};
    constexpr job::json::JsonSourceRange pastEnd{0, source.size() + 1};
    constexpr job::json::JsonSourceRange reversed{5, 3};

    STATIC_REQUIRE(whole.validFor(source));
    STATIC_REQUIRE(middle.validFor(source));
    STATIC_REQUIRE(atEnd.validFor(source));

    STATIC_REQUIRE_FALSE(pastEnd.validFor(source));
    STATIC_REQUIRE_FALSE(reversed.validFor(source));
}

TEST_CASE("JsonSourceRange returns borrowed source view", "[job_json][source_range]")
{
    constexpr std::string_view source = R"({"name":"cake"})";

    constexpr job::json::JsonSourceRange name{2, 6};
    constexpr job::json::JsonSourceRange cake{9, 13};

    STATIC_REQUIRE(name.view(source) == "name");
    STATIC_REQUIRE(cake.view(source) == "cake");
}

TEST_CASE("JsonSourceRange supports empty range at source end", "[job_json][source_range]")
{
    constexpr std::string_view source = "json";
    constexpr job::json::JsonSourceRange range{source.size(), source.size()};

    STATIC_REQUIRE(range.validFor(source));
    STATIC_REQUIRE(range.empty());
    STATIC_REQUIRE(range.view(source).empty());
}

TEST_CASE("JsonSourceRange copy and move preserve offsets", "[job_json][source_range]")
{
    constexpr job::json::JsonSourceRange original{4, 9};
    constexpr job::json::JsonSourceRange copied = original;

    STATIC_REQUIRE(copied.begin() == 4);
    STATIC_REQUIRE(copied.end() == 9);

    constexpr job::json::JsonSourceRange moved = [] {
        job::json::JsonSourceRange source{7, 12};
        return job::json::JsonSourceRange{std::move(source)};
    }();

    STATIC_REQUIRE(moved.begin() == 7);
    STATIC_REQUIRE(moved.end() == 12);
}

