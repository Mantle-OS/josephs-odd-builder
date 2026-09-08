#pragma once

namespace job::json::tests {

// START KEY_DISPATCH
struct DispatchFixture
{
    int         count{};
    float       ratio{};
    bool        enabled{};
    std::string name{};
};
// END


// START PARSER_DESTINATION_(and friends)SCALAR
enum class DispatchMode : std::uint8_t
{
    Zero = 0,
    One = 1,
    Max = 255
};

struct NonDefaultConstructible
{
    explicit NonDefaultConstructible(int value) :
        value(value)
    {
    }

    int value{};
};
// END



// START PARSER_DESTINATION_OBJECT
struct ObjectDestinationFixture
{
    int count{};
    bool enabled{};
    std::string name{};
    std::optional<int> optionalCount{};
    std::shared_ptr<int> sharedCount{};
};
struct NestedDestinationFixture
{
    int port{};
    std::string host{};
};

struct ObjectDestinationNestedFixture
{
    NestedDestinationFixture server{};
    std::optional<NestedDestinationFixture> optionalServer{};
};
// END




// START PARSER (Scalar)
struct ParserScalarObject
{
    int count{};
    float ratio{};
    bool enabled{};
    std::string name{};
    std::optional<int> optionalCount{};
};

struct ParserNestedObject
{
    int port{};
    std::string host{};
};

struct ParserRootObject
{
    int id{};
    ParserNestedObject server{};
    std::optional<ParserNestedObject> optionalServer{};
    std::vector<int> values{};
};
// END

// START PARSER (Go deeper)
struct ParserLeaf
{
    int value{};
};

struct ParserMiddle
{
    ParserLeaf leaf{};
};

struct ParserOuter
{
    ParserMiddle middle{};
};
// END


// START EMITTERS
struct JsonRoundTripFixture
{
    int id{};
    double ratio{};
    bool enabled{};
    std::string name{};
    std::optional<int> optionalCount{};
    std::vector<int> values{};
    ParserNestedObject nested{};
};
// END




} // namespace job::json::tests
