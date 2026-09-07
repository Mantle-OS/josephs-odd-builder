#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <contracts>
#include <memory>
#include <optional>

#include <job_yaml_contracts.h>
#include <job_yaml_concepts.h>

#include <job_yaml_parser_destination.h>
#include <job_yaml_object_parser_destination.h>

namespace job::yaml::tests {

// START CONTRACTS TEST
struct ContractFixture
{
    inline static std::size_t preBodyCalls{};
    inline static std::size_t postBodyCalls{};
    inline static std::size_t assertBodyCalls{};

    static void reset() noexcept
    {
        preBodyCalls = 0;
        postBodyCalls = 0;
        assertBodyCalls = 0;
    }

    static void requirePositive(int value) noexcept
        pre(value > 0)
    {
        ++preBodyCalls;
    }

    [[nodiscard]] static int producePositive(bool valid) noexcept
        post(result: result > 0)
    {
        ++postBodyCalls;
        return valid ? 1 : 0;
    }

    static void requireNonZero(int value) noexcept
    {
        ++assertBodyCalls;
        contract_assert(value != 0);
    }
};
// END CONTRACTS TEST


// START SCALAR_KERNEL TEST
template <typename T>
struct ScalarCase
{
    std::string_view text;
    T expected;
    bool valid;
};
// END SCALAR_KERNEL TEST

// START ESCAPE_KERNEL TEST
struct EscapeCase
{
    std::string_view text;
    char32_t codepoint;
    std::uint8_t consumed;
    bool valid;
};
// END ESCAPE_KERNEL TEST

// START KEY_DISPATCH TEST
struct KeyDispatchObject
{
    int count{};
    bool enabled{};
    float scale{};
};
// END KEY_DISPATCH TEST

// START OBJECT_READER TEST
enum class ObjectReaderMode : std::uint8_t {
    Zero = 0,
    One = 1,
    Two = 2
};

struct ObjectReaderObject
{
    int count{};
    std::uint32_t size{};
    bool enabled{};
    float scale{};
    double ratio{};
    std::string name{};
    std::string_view view{};
    ObjectReaderMode mode{ObjectReaderMode::Zero};
};

struct ObjectReaderUnsupportedObject
{
    int count{};
    std::vector<int> values{};
};
// END OBJECT_READER TEST

// START EMITTER TEST
struct EmitterObject
{
    int count{};
    std::uint32_t size{};
    bool enabled{};
    float scale{};
    double ratio{};
    std::string name{};
};

struct EmitterNullableObject
{
    std::optional<int> optionalValue{};
    std::shared_ptr<int> sharedValue{};
    std::unique_ptr<int> uniqueValue{};
};

struct FixedStringSink
{
    char buffer[256]{};
    std::size_t used{};

    constexpr void append(std::string_view text) noexcept
    {
        for (char c : text)
            buffer[used++] = c;
    }

    constexpr void push_back(char c) noexcept
    {
        buffer[used++] = c;
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept
    {
        return {buffer, used};
    }
};
// END EMITTER TEST

// START NODE_SINK TEST
struct NodeSinkFixture
{
    enum class Operation
    {
        None,
        Null,
        Scalar,
        Mapping,
        Sequence,
        Member,
        MemberNode,
        Append,
        AppendNode
    };

    Operation operation{Operation::None};
    std::string key{};
    std::string value{};

    void setNull() noexcept
    {
        operation = Operation::Null;
        key.clear();
        value.clear();
    }

    void setScalar(std::string_view scalar)
    {
        operation = Operation::Scalar;
        key.clear();
        value.assign(scalar);
    }

    void setMapping() noexcept
    {
        operation = Operation::Mapping;
        key.clear();
        value.clear();
    }

    void setSequence() noexcept
    {
        operation = Operation::Sequence;
        key.clear();
        value.clear();
    }

    void setMember(std::string_view memberKey, NodeSinkFixture node)
    {
        operation = Operation::MemberNode;
        key.assign(memberKey);
        value = std::move(node.value);
    }

    void setMemberScalar(std::string_view memberKey, std::string_view scalar)
    {
        operation = Operation::Member;
        key.assign(memberKey);
        value.assign(scalar);
    }

    void append(NodeSinkFixture node)
    {
        operation = Operation::AppendNode;
        key.clear();
        value = std::move(node.value);
    }

    void appendScalar(std::string_view scalar)
    {
        operation = Operation::Append;
        key.clear();
        value.assign(scalar);
    }
};

static_assert(YamlNodeDestination<NodeSinkFixture>);
// END NODE_SINK TEST

// START VALIDATE_SINK TEST
struct ValidateUnsupportedObject
{
    int count{};
    std::vector<int> values{};
};
// END VALIDATE_SINK TEST


// START EVENT_SINK TEST
struct EventSinkFixture
{
    enum class Operation
    {
        None,
        Null,
        Scalar,
        ScalarOwned,
        Key,
        KeyOwned,
        BeginMapping,
        EndMapping,
        BeginSequence,
        EndSequence
    };

    Operation operation{Operation::None};
    std::string keyValue{};
    std::string value{};

    bool null() noexcept
    {
        operation = Operation::Null;
        keyValue.clear();
        value.clear();
        return true;
    }

    bool scalar(std::string_view scalarValue) noexcept
    {
        operation = Operation::Scalar;
        keyValue.clear();
        value.assign(scalarValue);
        return true;
    }

    bool scalarOwned(std::string scalarValue) noexcept
    {
        operation = Operation::ScalarOwned;
        keyValue.clear();
        value = std::move(scalarValue);
        return true;
    }

    bool key(std::string_view mappingKey) noexcept
    {
        operation = Operation::Key;
        keyValue.assign(mappingKey);
        value.clear();
        return true;
    }

    bool keyOwned(std::string mappingKey) noexcept
    {
        operation = Operation::KeyOwned;
        keyValue = std::move(mappingKey);
        value.clear();
        return true;
    }

    bool beginMapping() noexcept
    {
        operation = Operation::BeginMapping;
        keyValue.clear();
        value.clear();
        return true;
    }

    bool endMapping() noexcept
    {
        operation = Operation::EndMapping;
        keyValue.clear();
        value.clear();
        return true;
    }

    bool beginSequence() noexcept
    {
        operation = Operation::BeginSequence;
        keyValue.clear();
        value.clear();
        return true;
    }

    bool endSequence() noexcept
    {
        operation = Operation::EndSequence;
        keyValue.clear();
        value.clear();
        return true;
    }
};

struct ThrowingEventSinkFixture
{
    bool null()
    {
        return true;
    }

    bool scalar(std::string_view)
    {
        return true;
    }

    bool scalarOwned(std::string)
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

    bool beginMapping()
    {
        return true;
    }

    bool endMapping()
    {
        return true;
    }

    bool beginSequence()
    {
        return true;
    }

    bool endSequence()
    {
        return true;
    }
};

static_assert(YamlParseDestination<EventSinkFixture>);
static_assert(YamlParseDestination<ThrowingEventSinkFixture>);
// END EVENT_SINK TEST

// START OBJECT_PARSE_DESTINATION
struct ObjectDestinationAddressFixture
{
    std::string city;
    int zip{};
};

struct ObjectDestinationProfileFixture
{
    std::string displayName;
    ObjectDestinationAddressFixture address;
};

struct ObjectDestinationFixture
{
    std::string name;
    int count{};
    bool enabled{};
    double ratio{};
    ObjectDestinationAddressFixture address;
    ObjectDestinationProfileFixture profile;
};

static_assert(YamlParseDestination<YamlObjectParserDestination<ObjectDestinationFixture>>);  // here
// END OBJECT_PARSE_DESTINATION TEST


// START PARSER TEST
struct ParserAddressFixture
{
    std::string city;
    int zip{};
};

struct ParserProfileFixture
{
    std::string displayName;
    ParserAddressFixture address;
};

struct ParserDestinationFixture
{
    std::string name;
    int count{};
    bool enabled{};
    double ratio{};
    ParserAddressFixture address;
    ParserProfileFixture profile;
};

static_assert(YamlParseDestination<YamlObjectParserDestination<ParserDestinationFixture>>); // here
// END PARSER TEST


} // namespace job::yaml::tests