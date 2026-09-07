#include <job_yaml.h>

#include "test_job_yaml_fixtures.h"

#include <optional>
#include <string_view>

using namespace std::string_view_literals;

namespace job::yaml::tests {

consteval bool constexprEmitInteger()
{
    FixedStringSink sink;

    if (!JobYaml::emit(42, sink))
        return false;

    return sink.view() == "42";
}

consteval bool constexprEmitBool()
{
    FixedStringSink sink;

    if (!JobYaml::emit(true, sink))
        return false;

    return sink.view() == "true";
}

consteval bool constexprEmitString()
{
    FixedStringSink sink;

    if (!JobYaml::emit("JOB YAML"sv, sink))
        return false;

    return sink.view() == "\"JOB YAML\"";
}

consteval bool constexprEmitEmptyOptional()
{
    std::optional<int> value;
    FixedStringSink sink;

    if (!JobYaml::emit(value, sink))
        return false;

    return sink.view() == "null";
}

consteval bool constexprEmitOptional()
{
    std::optional<int> value{42};
    FixedStringSink sink;

    if (!JobYaml::emit(value, sink))
        return false;

    return sink.view() == "42";
}

struct ConstexprEmitterObject
{
    int count{};
    bool enabled{};
    std::string_view name{};
};

consteval bool constexprEmitObject()
{
    constexpr ConstexprEmitterObject object{
        .count = 42,
        .enabled = true,
        .name = "JOB",
    };

    FixedStringSink sink;

    if (!JobYaml::emit(object, sink))
        return false;

    return sink.view() ==
           "count: 42\n"
           "enabled: true\n"
           "name: \"JOB\"\n";
}

static_assert(constexprEmitInteger());
static_assert(constexprEmitBool());
static_assert(constexprEmitString());
static_assert(constexprEmitEmptyOptional());
static_assert(constexprEmitOptional());
static_assert(constexprEmitObject());

} // namespace job::yaml::tests