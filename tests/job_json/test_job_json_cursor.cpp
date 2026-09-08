#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include <job_json_cursor.h>

TEST_CASE("JsonCursor starts at beginning of source", "[job_json][cursor]")
{
    constexpr std::string_view source = R"({"value":42})";
    job::json::JsonCursor cursor{source};

    REQUIRE(cursor.source() == source);
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.size() == source.size());
    REQUIRE(cursor.remaining() == source.size());
    REQUIRE_FALSE(cursor.atEnd());
}

TEST_CASE("JsonCursor peeks without advancing", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"json"};

    REQUIRE(cursor.current() == 'j');
    REQUIRE(cursor.peek()  == 'j');
    REQUIRE(cursor.peek(1) == 's');
    REQUIRE(cursor.peek(2) == 'o');
    REQUIRE(cursor.peek(3) == 'n');

    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.remaining() == 4);
}

TEST_CASE("JsonCursor advances through source", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"json"};

    cursor.advance();
    REQUIRE(cursor.offset() == 1);
    REQUIRE(cursor.current() == 's');
    REQUIRE(cursor.remaining() == 3);

    cursor.advance(2);
    REQUIRE(cursor.offset() == 3);
    REQUIRE(cursor.current() == 'n');
    REQUIRE(cursor.remaining() == 1);

    cursor.advance();
    REQUIRE(cursor.offset() == 4);
    REQUIRE(cursor.remaining() == 0);
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JsonCursor has reports available source", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"cake"};

    REQUIRE(cursor.has());
    REQUIRE(cursor.has(1));
    REQUIRE(cursor.has(4));
    REQUIRE_FALSE(cursor.has(5));
    cursor.advance(2);

    REQUIRE(cursor.has());
    REQUIRE(cursor.has(2));
    REQUIRE_FALSE(cursor.has(3));
    cursor.advance(2);

    REQUIRE_FALSE(cursor.has());
    REQUIRE(cursor.has(0));
    REQUIRE(cursor.atEnd());
}

TEST_CASE("JsonCursor conditionally consumes expected character", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"{}"};

    REQUIRE(cursor.consume('{'));
    REQUIRE(cursor.offset() == 1);
    REQUIRE(cursor.current() == '}');

    REQUIRE_FALSE(cursor.consume('{'));
    REQUIRE(cursor.offset() == 1);

    REQUIRE(cursor.consume('}'));
    REQUIRE(cursor.atEnd());

    REQUIRE_FALSE(cursor.consume('}'));
    REQUIRE(cursor.offset() == 2);
}

TEST_CASE("JsonCursor resets to beginning", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"json"};

    cursor.advance(3);
    REQUIRE(cursor.offset() == 3);

    cursor.reset();
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'j');
    REQUIRE(cursor.remaining() == 4);
}

TEST_CASE("JsonCursor seeks to valid source offset", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{"abcdef"};

    cursor.seek(3);
    REQUIRE(cursor.offset() == 3);
    REQUIRE(cursor.current() == 'd');
    REQUIRE(cursor.remaining() == 3);

    cursor.seek(6);
    REQUIRE(cursor.offset() == 6);
    REQUIRE(cursor.atEnd());

    cursor.seek(0);
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.current() == 'a');
}

TEST_CASE("JsonCursor creates range from previous offset", "[job_json][cursor]")
{
    constexpr std::string_view source = R"("cake")";
    job::json::JsonCursor cursor{source};

    cursor.advance();
    const std::size_t begin = cursor.offset();

    cursor.advance(4);

    const job::json::JsonSourceRange range = cursor.rangeFrom(begin);

    REQUIRE(range.begin() == 1);
    REQUIRE(range.end()   == 5);
    REQUIRE(range.size()  == 4);
    REQUIRE(range.view(source) == "cake");
}

TEST_CASE("JsonCursor creates explicit source range", "[job_json][cursor]")
{
    constexpr std::string_view source = R"({"cake":true})";
    job::json::JsonCursor cursor{source};

    const job::json::JsonSourceRange range = cursor.range(2, 6);

    REQUIRE(range.begin() == 2);
    REQUIRE(range.end()   == 6);
    REQUIRE(range.view(source) == "cake");
}

TEST_CASE("JsonCursor supports empty source", "[job_json][cursor]")
{
    job::json::JsonCursor cursor{""};
    REQUIRE(cursor.source().empty());
    REQUIRE(cursor.size() == 0);
    REQUIRE(cursor.offset() == 0);
    REQUIRE(cursor.remaining() == 0);
    REQUIRE(cursor.atEnd());
    REQUIRE_FALSE(cursor.has());
    REQUIRE(cursor.has(0));
}

TEST_CASE("JsonCursor preserves borrowed source", "[job_json][cursor]")
{
    constexpr std::string_view source = "borrowed-json";
    job::json::JsonCursor cursor{source};
    REQUIRE(cursor.source().data() == source.data());
    REQUIRE(cursor.source().size() == source.size());
}

TEST_CASE("JsonCursor move construction preserves state", "[job_json][cursor]")
{
    job::json::JsonCursor source{"abcdef"};
    source.advance(4);

    job::json::JsonCursor moved{std::move(source)};
    REQUIRE(moved.source() == "abcdef");
    REQUIRE(moved.offset() == 4);
    REQUIRE(moved.current() == 'e');
    REQUIRE(moved.remaining() == 2);
}

