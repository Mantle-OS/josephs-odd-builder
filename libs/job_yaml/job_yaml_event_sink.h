#pragma once

#include <string>
#include <string_view>
#include <utility>

#include "job_yaml_concepts.h"

namespace job::yaml {

class YamlEventSink
{
public:
    template <YamlParseDestination Sink>
    static constexpr bool scalar(Sink &sink, std::string_view value) noexcept(noexcept(sink.scalar(value)))
    {
        return sink.scalar(value);
    }

    template <YamlParseDestination Sink>
    static constexpr bool scalarOwned(Sink &sink, std::string value) noexcept(noexcept(sink.scalarOwned(std::move(value))))
    {
        return sink.scalarOwned(std::move(value));
    }

    template <YamlParseDestination Sink>
    static constexpr bool key(Sink &sink, std::string_view value) noexcept(noexcept(sink.key(value)))
    {
        return sink.key(value);
    }

    template <YamlParseDestination Sink>
    static constexpr bool keyOwned(Sink &sink, std::string value) noexcept(noexcept(sink.keyOwned(std::move(value))))
    {
        return sink.keyOwned(std::move(value));
    }

    template <YamlParseDestination Sink>
    static constexpr bool beginMapping(Sink &sink) noexcept(noexcept(sink.beginMapping()))
    {
        return sink.beginMapping();
    }

    template <YamlParseDestination Sink>
    static constexpr bool endMapping(Sink &sink) noexcept(noexcept(sink.endMapping()))
    {
        return sink.endMapping();
    }

    template <YamlParseDestination Sink>
    static constexpr bool beginSequence(Sink &sink) noexcept(noexcept(sink.beginSequence()))
    {
        return sink.beginSequence();
    }

    template <YamlParseDestination Sink>
    static constexpr bool endSequence(Sink &sink) noexcept(noexcept(sink.endSequence()))
    {
        return sink.endSequence();
    }
};

} // namespace job::yaml