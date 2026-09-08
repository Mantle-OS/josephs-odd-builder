#pragma once

#include <string_view>
#include <utility>

#include "job_yaml_concepts.h"

namespace job::yaml {

class YamlNodeSink
{
public:
    YamlNodeSink() = delete;
    ~YamlNodeSink() = delete;

    YamlNodeSink(const YamlNodeSink &) = delete;
    YamlNodeSink &operator=(const YamlNodeSink &) = delete;
    YamlNodeSink(YamlNodeSink &&) = delete;
    YamlNodeSink &operator=(YamlNodeSink &&) = delete;

    template <YamlNodeDestination Node>
    static constexpr bool null(Node &destination)
        noexcept(noexcept(destination.setNull()))
    {
        destination.setNull();
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool scalar(Node &destination, std::string_view value)
        noexcept(noexcept(destination.setScalar(value)))
    {
        destination.setScalar(value);
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool scalarView(Node &destination, std::string_view value)
        noexcept(noexcept(destination.setScalarView(value)))
    {
        destination.setScalarView(value);
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool mapping(Node &destination)
        noexcept(noexcept(destination.setMapping()))
    {
        destination.setMapping();
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool sequence(Node &destination)
        noexcept(noexcept(destination.setSequence()))
    {
        destination.setSequence();
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool member(Node &destination, std::string_view key, Node value)
        noexcept(noexcept(destination.setMember(key, std::move(value))))
    {
        destination.setMember(key, std::move(value));
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool member(Node &destination, std::string_view key, std::string_view value)
        noexcept(noexcept(destination.setMemberScalar(key, value)))
    {
        destination.setMemberScalar(key, value);
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool memberView(Node &destination, std::string_view key, std::string_view value)
        noexcept(noexcept(destination.setMemberScalarView(key, value)))
    {
        destination.setMemberScalarView(key, value);
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool append(Node &destination, Node value)
        noexcept(noexcept(destination.append(std::move(value))))
    {
        destination.append(std::move(value));
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool append(Node &destination, std::string_view value)
        noexcept(noexcept(destination.appendScalar(value)))
    {
        destination.appendScalar(value);
        return true;
    }

    template <YamlNodeDestination Node>
    static constexpr bool appendView(Node &destination, std::string_view value)
        noexcept(noexcept(destination.appendScalarView(value)))
    {
        destination.appendScalarView(value);
        return true;
    }
};

} // namespace job::yaml