#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <job_json_concepts.h>

namespace {

enum class TestEnum : std::uint16_t
{
    Zero,
    One,
};

struct PlainObject
{
    int value{};
};

struct GoodSink
{
    void append(std::string_view)
    {
    }
};

struct BadSinkMissingAppend
{
};

struct BadSinkWrongArgument
{
    void append(int)
    {
    }
};

struct GoodDestination
{
    bool null()
    {
        return true;
    }

    bool boolean(bool)
    {
        return true;
    }

    bool number(std::string_view)
    {
        return true;
    }

    bool string(std::string_view)
    {
        return true;
    }

    bool stringOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginObject()
    {
        return true;
    }

    bool endObject()
    {
        return true;
    }

    bool beginArray()
    {
        return true;
    }

    bool endArray()
    {
        return true;
    }
};
struct GoodObjectDestination
{
    int &object();
};

struct BadObjectDestinationMissingObject
{
};
struct BadDestinationMissingNull
{
    bool boolean(bool)
    {
        return true;
    }

    bool number(std::string_view)
    {
        return true;
    }

    bool string(std::string_view)
    {
        return true;
    }

    bool stringOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginObject()
    {
        return true;
    }

    bool endObject()
    {
        return true;
    }

    bool beginArray()
    {
        return true;
    }

    bool endArray()
    {
        return true;
    }
};

struct BadDestinationMissingNumber
{
    bool null()
    {
        return true;
    }

    bool boolean(bool)
    {
        return true;
    }

    bool string(std::string_view)
    {
        return true;
    }

    bool stringOwned(std::string)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginObject()
    {
        return true;
    }

    bool endObject()
    {
        return true;
    }

    bool beginArray()
    {
        return true;
    }

    bool endArray()
    {
        return true;
    }
};

struct BadDestinationMissingOwnedString
{
    bool null()
    {
        return true;
    }

    bool boolean(bool)
    {
        return true;
    }

    bool number(std::string_view)
    {
        return true;
    }

    bool string(std::string_view)
    {
        return true;
    }

    bool key(std::string_view)
    {
        return true;
    }

    bool keyOwned(std::string)
    {
        return true;
    }

    bool beginObject()
    {
        return true;
    }

    bool endObject()
    {
        return true;
    }

    bool beginArray()
    {
        return true;
    }

    bool endArray()
    {
        return true;
    }
};

struct BadDestinationWrongReturn
{
    void null()
    {
    }

    void boolean(bool)
    {
    }

    void number(std::string_view)
    {
    }

    void string(std::string_view)
    {
    }

    void stringOwned(std::string)
    {
    }

    void key(std::string_view)
    {
    }

    void keyOwned(std::string)
    {
    }

    void beginObject()
    {
    }

    void endObject()
    {
    }

    void beginArray()
    {
    }

    void endArray()
    {
    }
};

} // namespace

TEST_CASE("JSON concepts classify string types", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonOwnedString<std::string>);
    STATIC_REQUIRE(job::json::JsonStringView<std::string_view>);

    STATIC_REQUIRE(job::json::JsonString<std::string>);
    STATIC_REQUIRE(job::json::JsonString<std::string_view>);

    STATIC_REQUIRE(job::json::JsonString<const std::string &>);
    STATIC_REQUIRE(job::json::JsonString<const std::string_view &>);

    STATIC_REQUIRE_FALSE(job::json::JsonString<const char *>);
    STATIC_REQUIRE_FALSE(job::json::JsonString<char *>);
    STATIC_REQUIRE_FALSE(job::json::JsonString<int>);
}

TEST_CASE("JSON concepts classify scalar types", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonBoolean<bool>);
    STATIC_REQUIRE_FALSE(job::json::JsonBoolean<int>);

    STATIC_REQUIRE(job::json::JsonSignedInteger<std::int8_t>);
    STATIC_REQUIRE(job::json::JsonSignedInteger<std::int16_t>);
    STATIC_REQUIRE(job::json::JsonSignedInteger<std::int32_t>);
    STATIC_REQUIRE(job::json::JsonSignedInteger<std::int64_t>);

    STATIC_REQUIRE(job::json::JsonUnsignedInteger<std::uint16_t>);
    STATIC_REQUIRE(job::json::JsonUnsignedInteger<std::uint32_t>);
    STATIC_REQUIRE(job::json::JsonUnsignedInteger<std::uint64_t>);

    STATIC_REQUIRE(job::json::JsonFloatingPoint<float>);
    STATIC_REQUIRE(job::json::JsonFloatingPoint<double>);

    STATIC_REQUIRE(job::json::JsonEnum<TestEnum>);
    STATIC_REQUIRE(job::json::JsonByte<std::byte>);

    STATIC_REQUIRE(job::json::JsonExtendedChar<wchar_t>);
    STATIC_REQUIRE(job::json::JsonExtendedChar<char8_t>);
    STATIC_REQUIRE(job::json::JsonExtendedChar<char16_t>);
    STATIC_REQUIRE(job::json::JsonExtendedChar<char32_t>);

    STATIC_REQUIRE(job::json::JsonScalar<bool>);
    STATIC_REQUIRE(job::json::JsonScalar<std::int32_t>);
    STATIC_REQUIRE(job::json::JsonScalar<std::uint64_t>);
    STATIC_REQUIRE(job::json::JsonScalar<float>);
    STATIC_REQUIRE(job::json::JsonScalar<double>);
    STATIC_REQUIRE(job::json::JsonScalar<TestEnum>);
    STATIC_REQUIRE(job::json::JsonScalar<std::byte>);
    STATIC_REQUIRE(job::json::JsonScalar<char32_t>);
    STATIC_REQUIRE(job::json::JsonScalar<std::string>);
    STATIC_REQUIRE(job::json::JsonScalar<std::string_view>);
}


TEST_CASE("JSON concepts keep character policy explicit", "[job_json][concepts]")
{
    STATIC_REQUIRE_FALSE(job::json::JsonSignedInteger<char>);

    STATIC_REQUIRE(job::json::JsonUnsignedInteger<unsigned char>);
    STATIC_REQUIRE(job::json::JsonUnsignedInteger<std::uint8_t>);

    STATIC_REQUIRE_FALSE(job::json::JsonInteger<bool>);
    STATIC_REQUIRE_FALSE(job::json::JsonInteger<char>);

    STATIC_REQUIRE(job::json::JsonInteger<unsigned char>);
    STATIC_REQUIRE(job::json::JsonInteger<std::uint8_t>);

    STATIC_REQUIRE_FALSE(job::json::JsonInteger<char8_t>);
    STATIC_REQUIRE_FALSE(job::json::JsonInteger<char16_t>);
    STATIC_REQUIRE_FALSE(job::json::JsonInteger<char32_t>);

    STATIC_REQUIRE_FALSE(job::json::JsonScalar<char>);

    STATIC_REQUIRE(job::json::JsonScalar<unsigned char>);
    STATIC_REQUIRE(job::json::JsonScalar<std::uint8_t>);
}

TEST_CASE("JSON concepts classify optional types", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonOptional<std::optional<int>>);
    STATIC_REQUIRE(job::json::JsonOptional<const std::optional<std::string> &>);

    STATIC_REQUIRE_FALSE(job::json::JsonOptional<int>);
    STATIC_REQUIRE_FALSE(job::json::JsonOptional<std::vector<int>>);

    STATIC_REQUIRE(std::same_as<
                   job::json::JsonOptionalValue<std::optional<std::uint32_t>>,
                   std::uint32_t>);
}

TEST_CASE("JSON concepts classify pointer ownership", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonSharedPointer<std::shared_ptr<PlainObject>>);
    STATIC_REQUIRE(job::json::JsonUniquePointer<std::unique_ptr<PlainObject>>);
    STATIC_REQUIRE(job::json::JsonWeakPointer<std::weak_ptr<PlainObject>>);

    STATIC_REQUIRE(job::json::JsonOwningPointer<std::shared_ptr<PlainObject>>);
    STATIC_REQUIRE(job::json::JsonOwningPointer<std::unique_ptr<PlainObject>>);

    STATIC_REQUIRE_FALSE(job::json::JsonOwningPointer<std::weak_ptr<PlainObject>>);
    STATIC_REQUIRE_FALSE(job::json::JsonOwningPointer<PlainObject *>);

    STATIC_REQUIRE(job::json::JsonUnsupportedPointer<std::weak_ptr<PlainObject>>);
    STATIC_REQUIRE(job::json::JsonUnsupportedPointer<PlainObject *>);

    STATIC_REQUIRE_FALSE(job::json::JsonUnsupportedPointer<std::shared_ptr<PlainObject>>);
    STATIC_REQUIRE_FALSE(job::json::JsonUnsupportedPointer<std::unique_ptr<PlainObject>>);

    STATIC_REQUIRE(std::same_as<
                   job::json::JsonSharedPointerElement<std::shared_ptr<PlainObject>>,
                   PlainObject>);

    STATIC_REQUIRE(std::same_as<
                   job::json::JsonUniquePointerElement<std::unique_ptr<PlainObject>>,
                   PlainObject>);
}

TEST_CASE("JSON concepts classify map containers", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonMap<std::map<std::string, int>>);
    STATIC_REQUIRE(job::json::JsonMap<std::unordered_map<std::string, PlainObject>>);

    STATIC_REQUIRE(job::json::JsonContainer<std::map<std::string, int>>);
    STATIC_REQUIRE(job::json::JsonContainer<std::unordered_map<int, std::string>>);

    STATIC_REQUIRE_FALSE(job::json::JsonSequence<std::map<std::string, int>>);
    STATIC_REQUIRE_FALSE(job::json::JsonSequence<std::unordered_map<std::string, int>>);
}

TEST_CASE("JSON concepts classify fixed sequences", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonFixedSequence<std::array<int, 4>>);
    STATIC_REQUIRE(job::json::JsonSequence<std::array<int, 4>>);
    STATIC_REQUIRE(job::json::JsonContainer<std::array<int, 4>>);

    STATIC_REQUIRE_FALSE(job::json::JsonPushBackSequence<std::array<int, 4>>);
    STATIC_REQUIRE_FALSE(job::json::JsonInsertSequence<std::array<int, 4>>);
}

TEST_CASE("JSON concepts classify push-back sequences", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonPushBackSequence<std::vector<int>>);
    STATIC_REQUIRE(job::json::JsonSequence<std::vector<int>>);
    STATIC_REQUIRE(job::json::JsonContainer<std::vector<int>>);

    STATIC_REQUIRE_FALSE(job::json::JsonFixedSequence<std::vector<int>>);
    STATIC_REQUIRE_FALSE(job::json::JsonInsertSequence<std::vector<int>>);
}

TEST_CASE("JSON concepts classify insert sequences", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonInsertSequence<std::set<int>>);
    STATIC_REQUIRE(job::json::JsonSequence<std::set<int>>);
    STATIC_REQUIRE(job::json::JsonContainer<std::set<int>>);

    STATIC_REQUIRE_FALSE(job::json::JsonFixedSequence<std::set<int>>);
    STATIC_REQUIRE_FALSE(job::json::JsonPushBackSequence<std::set<int>>);
}

TEST_CASE("JSON concepts do not classify strings as containers", "[job_json][concepts]")
{
    STATIC_REQUIRE_FALSE(job::json::JsonContainerBase<std::string>);
    STATIC_REQUIRE_FALSE(job::json::JsonContainerBase<std::string_view>);

    STATIC_REQUIRE_FALSE(job::json::JsonContainer<std::string>);
    STATIC_REQUIRE_FALSE(job::json::JsonContainer<std::string_view>);
}

TEST_CASE("JSON value concept accepts supported persistence shapes", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonValue<int>);
    STATIC_REQUIRE(job::json::JsonValue<double>);
    STATIC_REQUIRE(job::json::JsonValue<std::string>);
    STATIC_REQUIRE(job::json::JsonValue<TestEnum>);

    STATIC_REQUIRE(job::json::JsonValue<std::optional<int>>);
    STATIC_REQUIRE(job::json::JsonValue<std::shared_ptr<PlainObject>>);
    STATIC_REQUIRE(job::json::JsonValue<std::unique_ptr<PlainObject>>);

    STATIC_REQUIRE(job::json::JsonValue<std::vector<int>>);
    STATIC_REQUIRE(job::json::JsonValue<std::array<int, 4>>);
    STATIC_REQUIRE(job::json::JsonValue<std::set<int>>);
    STATIC_REQUIRE(job::json::JsonValue<std::map<std::string, int>>);

    STATIC_REQUIRE(job::json::JsonValue<PlainObject>);

    STATIC_REQUIRE_FALSE(job::json::JsonValue<PlainObject *>);
    STATIC_REQUIRE_FALSE(job::json::JsonValue<std::weak_ptr<PlainObject>>);
}

TEST_CASE("JSON output sink concept accepts valid sink", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonOutputSink<GoodSink>);

    STATIC_REQUIRE_FALSE(job::json::JsonOutputSink<BadSinkMissingAppend>);
    STATIC_REQUIRE_FALSE(job::json::JsonOutputSink<BadSinkWrongArgument>);
}

TEST_CASE("JSON output sink concept accepts std string", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonOutputSink<std::string>);
}

TEST_CASE("JSON value destination concept accepts complete destination", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonValueDestination<GoodDestination>);

    STATIC_REQUIRE_FALSE(job::json::JsonValueDestination<BadDestinationMissingNull>);
    STATIC_REQUIRE_FALSE(job::json::JsonValueDestination<BadDestinationMissingNumber>);
    STATIC_REQUIRE_FALSE(job::json::JsonValueDestination<BadDestinationMissingOwnedString>);
    STATIC_REQUIRE_FALSE(job::json::JsonValueDestination<BadDestinationWrongReturn>);
}

TEST_CASE("JSON object destination concept accepts object destination", "[job_json][concepts]")
{
    STATIC_REQUIRE(job::json::JsonObjectDestination<GoodObjectDestination>);
    STATIC_REQUIRE_FALSE(job::json::JsonObjectDestination<BadObjectDestinationMissingObject>);
}


