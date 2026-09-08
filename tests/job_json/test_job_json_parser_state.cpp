
#include <catch2/catch_test_macros.hpp>
#include <job_json_parser_state.h>

TEST_CASE("JsonParserFrame default constructs", "[job_json][parser_state]")
{
    constexpr job::json::JsonParserFrame frame;

    STATIC_REQUIRE(frame.type() == job::json::JsonParserFrameType::Object);
    STATIC_REQUIRE(frame.state() == job::json::JsonParserState::ExpectValue);
}

TEST_CASE("JsonParserFrame stores object frame state", "[job_json][parser_state]")
{
    constexpr job::json::JsonParserFrame frame{
        job::json::JsonParserFrameType::Object,
        job::json::JsonParserState::ExpectObjectKeyOrEnd
    };

    STATIC_REQUIRE(frame.type() == job::json::JsonParserFrameType::Object);
    STATIC_REQUIRE(frame.state() == job::json::JsonParserState::ExpectObjectKeyOrEnd);
}

TEST_CASE("JsonParserFrame stores array frame state", "[job_json][parser_state]")
{
    constexpr job::json::JsonParserFrame frame{
        job::json::JsonParserFrameType::Array,
        job::json::JsonParserState::ExpectArrayValueOrEnd
    };

    STATIC_REQUIRE(frame.type() == job::json::JsonParserFrameType::Array);
    STATIC_REQUIRE(frame.state() == job::json::JsonParserState::ExpectArrayValueOrEnd);
}

TEST_CASE("JsonParserFrame state can be updated", "[job_json][parser_state]")
{
    job::json::JsonParserFrame frame{
        job::json::JsonParserFrameType::Object,
        job::json::JsonParserState::ExpectObjectKeyOrEnd
    };

    REQUIRE(frame.state() == job::json::JsonParserState::ExpectObjectKeyOrEnd);

    frame.setState(job::json::JsonParserState::ExpectNameSeparator);

    REQUIRE(frame.state() == job::json::JsonParserState::ExpectNameSeparator);

    frame.setState(job::json::JsonParserState::ExpectObjectValue);

    REQUIRE(frame.state() == job::json::JsonParserState::ExpectObjectValue);

    frame.setState(job::json::JsonParserState::ExpectObjectSeparatorOrEnd);

    REQUIRE(frame.state() == job::json::JsonParserState::ExpectObjectSeparatorOrEnd);
}

TEST_CASE("JsonParserFrame type remains stable when state changes", "[job_json][parser_state]")
{
    job::json::JsonParserFrame frame{
        job::json::JsonParserFrameType::Array,
        job::json::JsonParserState::ExpectArrayValueOrEnd
    };

    frame.setState(job::json::JsonParserState::ExpectArraySeparatorOrEnd);

    REQUIRE(frame.type() == job::json::JsonParserFrameType::Array);
    REQUIRE(frame.state() == job::json::JsonParserState::ExpectArraySeparatorOrEnd);
}

TEST_CASE("JsonParserFrame copy preserves state", "[job_json][parser_state]")
{
    constexpr job::json::JsonParserFrame original{
        job::json::JsonParserFrameType::Object,
        job::json::JsonParserState::ExpectNameSeparator
    };

    constexpr job::json::JsonParserFrame copied = original;

    STATIC_REQUIRE(copied.type() == original.type());
    STATIC_REQUIRE(copied.state() == original.state());
}

TEST_CASE("JsonParserFrame move preserves state", "[job_json][parser_state]")
{
    constexpr job::json::JsonParserFrame moved = [] {
        job::json::JsonParserFrame frame{
            job::json::JsonParserFrameType::Array,
            job::json::JsonParserState::ExpectArraySeparatorOrEnd
        };

        return job::json::JsonParserFrame{std::move(frame)};
    }();

    STATIC_REQUIRE(moved.type() == job::json::JsonParserFrameType::Array);
    STATIC_REQUIRE(moved.state() == job::json::JsonParserState::ExpectArraySeparatorOrEnd);
}

TEST_CASE("JsonParserState values are distinct", "[job_json][parser_state]")
{
    STATIC_REQUIRE(job::json::JsonParserState::ExpectValue !=
                   job::json::JsonParserState::ExpectObjectKeyOrEnd);

    STATIC_REQUIRE(job::json::JsonParserState::ExpectObjectKeyOrEnd !=
                   job::json::JsonParserState::ExpectNameSeparator);

    STATIC_REQUIRE(job::json::JsonParserState::ExpectNameSeparator !=
                   job::json::JsonParserState::ExpectObjectValue);

    STATIC_REQUIRE(job::json::JsonParserState::ExpectObjectValue !=
                   job::json::JsonParserState::ExpectObjectSeparatorOrEnd);

    STATIC_REQUIRE(job::json::JsonParserState::ExpectArrayValueOrEnd !=
                   job::json::JsonParserState::ExpectArraySeparatorOrEnd);

    STATIC_REQUIRE(job::json::JsonParserState::Done !=
                   job::json::JsonParserState::Error);
}

TEST_CASE("JsonParserFrameType values are distinct", "[job_json][parser_state]")
{
    STATIC_REQUIRE(job::json::JsonParserFrameType::Object !=
                   job::json::JsonParserFrameType::Array);
}

TEST_CASE("JsonParserState exposes root state", "[job_json][parser_state]")
{
    constexpr auto state = job::json::JsonParserState::ExpectValue;

    STATIC_REQUIRE(state == job::json::JsonParserState::ExpectValue);
}

TEST_CASE("JsonParserState exposes object states", "[job_json][parser_state]")
{
    constexpr auto keyOrEnd = job::json::JsonParserState::ExpectObjectKeyOrEnd;
    constexpr auto nameSeparator = job::json::JsonParserState::ExpectNameSeparator;
    constexpr auto value = job::json::JsonParserState::ExpectObjectValue;
    constexpr auto separatorOrEnd = job::json::JsonParserState::ExpectObjectSeparatorOrEnd;

    STATIC_REQUIRE(keyOrEnd == job::json::JsonParserState::ExpectObjectKeyOrEnd);
    STATIC_REQUIRE(nameSeparator == job::json::JsonParserState::ExpectNameSeparator);
    STATIC_REQUIRE(value == job::json::JsonParserState::ExpectObjectValue);
    STATIC_REQUIRE(separatorOrEnd == job::json::JsonParserState::ExpectObjectSeparatorOrEnd);
}

TEST_CASE("JsonParserState exposes array states", "[job_json][parser_state]")
{
    constexpr auto valueOrEnd = job::json::JsonParserState::ExpectArrayValueOrEnd;
    constexpr auto separatorOrEnd = job::json::JsonParserState::ExpectArraySeparatorOrEnd;

    STATIC_REQUIRE(valueOrEnd == job::json::JsonParserState::ExpectArrayValueOrEnd);
    STATIC_REQUIRE(separatorOrEnd == job::json::JsonParserState::ExpectArraySeparatorOrEnd);
}

TEST_CASE("JsonParserState exposes terminal states", "[job_json][parser_state]")
{
    constexpr auto done = job::json::JsonParserState::Done;
    constexpr auto error = job::json::JsonParserState::Error;

    STATIC_REQUIRE(done == job::json::JsonParserState::Done);
    STATIC_REQUIRE(error == job::json::JsonParserState::Error);
    STATIC_REQUIRE(done != error);
}

