#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>

#include <job_json_parser_destination.h>


    TEST_CASE("JsonParserDestination assigns borrowed string to owned string", "[job_json][parser_destination][string]")
{
    std::string value = "before";

    job::json::JsonParserDestination<std::string> destination{value};

    constexpr std::string_view source = "Cake Court";

    REQUIRE(destination.string(source));
    REQUIRE(value == "Cake Court");
}

TEST_CASE("JsonParserDestination assigns borrowed string to string view", "[job_json][parser_destination][string]")
{
    std::string_view value;

    job::json::JsonParserDestination<std::string_view> destination{value};

    constexpr std::string_view source = "Cake Court";

    REQUIRE(destination.string(source));
    REQUIRE(value == source);
    REQUIRE(value.data() == source.data());
    REQUIRE(value.size() == source.size());
}

TEST_CASE("JsonParserDestination borrowed string view aliases source", "[job_json][parser_destination][string]")
{
    constexpr std::string_view source = "borrowed JSON string";

    std::string_view value;
    job::json::JsonParserDestination<std::string_view> destination{value};

    REQUIRE(destination.string(source));

    REQUIRE(value.data() == source.data());
    REQUIRE(value.size() == source.size());
}

TEST_CASE("JsonParserDestination assigns empty borrowed string to owned string", "[job_json][parser_destination][string]")
{
    std::string value = "not empty";

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(destination.string(""));
    REQUIRE(value.empty());
}

TEST_CASE("JsonParserDestination assigns empty borrowed string to string view", "[job_json][parser_destination][string]")
{
    std::string_view value = "not empty";

    job::json::JsonParserDestination<std::string_view> destination{value};

    constexpr std::string_view source;

    REQUIRE(destination.string(source));
    REQUIRE(value.empty());
    REQUIRE(value.data() == source.data());
}

static constexpr char data[] = {'c', 'a', 'k', 'e', '\0', 'c', 'o', 'u', 'r', 't'};

TEST_CASE("JsonParserDestination preserves raw UTF8 in owned string", "[job_json][parser_destination][string]")
{
    constexpr std::string_view source = "Grüße 世界 😀";

    std::string value;
    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(destination.string(source));
    REQUIRE(value == source);
}

TEST_CASE("JsonParserDestination preserves raw UTF8 in borrowed string view", "[job_json][parser_destination][string]")
{
    constexpr std::string_view source = "Grüße 世界 😀";

    std::string_view value;
    job::json::JsonParserDestination<std::string_view> destination{value};

    REQUIRE(destination.string(source));

    REQUIRE(value == source);
    REQUIRE(value.data() == source.data());
}

TEST_CASE("JsonParserDestination moves owned string into owned destination", "[job_json][parser_destination][string]")
{
    std::string value = "before";

    job::json::JsonParserDestination<std::string> destination{value};

    std::string source = "escaped Cake Court";

    REQUIRE(destination.stringOwned(std::move(source)));
    REQUIRE(value == "escaped Cake Court");
}

TEST_CASE("JsonParserDestination accepts empty owned string", "[job_json][parser_destination][string]")
{
    std::string value = "before";

    job::json::JsonParserDestination<std::string> destination{value};

    std::string source;

    REQUIRE(destination.stringOwned(std::move(source)));
    REQUIRE(value.empty());
}

TEST_CASE("JsonParserDestination preserves embedded null in owned string move", "[job_json][parser_destination][string]")
{
    std::string source{"cake\0court", 10};
    std::string value;

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(destination.stringOwned(std::move(source)));

    REQUIRE(value.size() == 10);
    REQUIRE(value[4] == '\0');
    REQUIRE(value == std::string{"cake\0court", 10});
}

TEST_CASE("JsonParserDestination preserves UTF8 in owned string move", "[job_json][parser_destination][string]")
{
    std::string source = "Grüße 世界 😀";
    std::string value;

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(destination.stringOwned(std::move(source)));
    REQUIRE(value == "Grüße 世界 😀");
}

TEST_CASE("JsonParserDestination rejects owned string for string view", "[job_json][parser_destination][string]")
{
    std::string backing = "existing";
    std::string_view value = backing;

    job::json::JsonParserDestination<std::string_view> destination{value};

    std::string source = "temporary decoded string";

    REQUIRE_FALSE(destination.stringOwned(std::move(source)));

    REQUIRE(value == "existing");
    REQUIRE(value.data() == backing.data());
}

TEST_CASE("JsonParserDestination rejection does not replace string view", "[job_json][parser_destination][string]")
{
    std::string backing = "stable backing storage";
    std::string_view value = backing;

    job::json::JsonParserDestination<std::string_view> destination{value};

    std::string owned = "would dangle";

    REQUIRE_FALSE(destination.stringOwned(std::move(owned)));

    REQUIRE(value.data() == backing.data());
    REQUIRE(value.size() == backing.size());
    REQUIRE(value == backing);
}

TEST_CASE("JsonParserDestination string path rejects integer destination", "[job_json][parser_destination][string]")
{
    int value = 42;

    job::json::JsonParserDestination<int> destination{value};

    REQUIRE_FALSE(destination.string("123"));
    REQUIRE(value == 42);
}

TEST_CASE("JsonParserDestination owned string path rejects integer destination", "[job_json][parser_destination][string]")
{
    int value = 42;

    job::json::JsonParserDestination<int> destination{value};

    std::string source = "123";

    REQUIRE_FALSE(destination.stringOwned(std::move(source)));
    REQUIRE(value == 42);
}

TEST_CASE("JsonParserDestination string path rejects boolean destination", "[job_json][parser_destination][string]")
{
    bool value = true;

    job::json::JsonParserDestination<bool> destination{value};

    REQUIRE_FALSE(destination.string("false"));
    REQUIRE(value);
}

TEST_CASE("JsonParserDestination owned string path rejects boolean destination", "[job_json][parser_destination][string]")
{
    bool value = true;

    job::json::JsonParserDestination<bool> destination{value};

    std::string source = "false";

    REQUIRE_FALSE(destination.stringOwned(std::move(source)));
    REQUIRE(value);
}

TEST_CASE("JsonParserDestination borrowed assignment replaces previous owned string", "[job_json][parser_destination][string]")
{
    std::string value = "old value";

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(destination.string("new value"));
    REQUIRE(value == "new value");
}

TEST_CASE("JsonParserDestination borrowed assignment replaces previous string view", "[job_json][parser_destination][string]")
{
    constexpr std::string_view first = "first";
    constexpr std::string_view second = "second";

    std::string_view value = first;

    job::json::JsonParserDestination<std::string_view> destination{value};

    REQUIRE(destination.string(second));

    REQUIRE(value == second);
    REQUIRE(value.data() == second.data());
}

TEST_CASE("JsonParserDestination owned assignment replaces previous owned string", "[job_json][parser_destination][string]")
{
    std::string value = "old value";

    job::json::JsonParserDestination<std::string> destination{value};

    std::string source = "new owned value";

    REQUIRE(destination.stringOwned(std::move(source)));
    REQUIRE(value == "new owned value");
}

TEST_CASE("JsonParserDestination value returns actual owned string", "[job_json][parser_destination][string]")
{
    std::string value = "Cake Court";

    job::json::JsonParserDestination<std::string> destination{value};

    REQUIRE(&destination.value() == &value);

    destination.value() = "JOB";

    REQUIRE(value == "JOB");
}

TEST_CASE("JsonParserDestination value returns actual string view", "[job_json][parser_destination][string]")
{
    constexpr std::string_view source = "Cake Court";

    std::string_view value = source;

    job::json::JsonParserDestination<std::string_view> destination{value};

    REQUIRE(&destination.value() == &value);
    REQUIRE(destination.value().data() == source.data());
}

