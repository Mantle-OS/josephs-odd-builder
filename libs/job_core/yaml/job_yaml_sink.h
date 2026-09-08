#pragma once

#include <string_view>

#include "job_yaml_concepts.h"
#include "job_yaml_object_reader.h"
#include "job_yaml_scalar_kernel.h"

namespace job::yaml {

class YamlSink
{
public:
    template <typename T>
    static constexpr bool scalar(T &destination, std::string_view value) noexcept
    {
        using DestinationType = YamlType<T>;

        if constexpr (YamlBool<DestinationType>) {
            return YamlScalarKernel::parseBool(value, destination);
        } else if constexpr (YamlInteger<DestinationType>) {
            return YamlScalarKernel::parseInteger(value, destination);
        } else if constexpr (YamlFloatingPoint<DestinationType>) {
            return YamlScalarKernel::parseFloat(value, destination);
        } else if constexpr (YamlOwnedString<DestinationType>) {
            destination.assign(value);
            return true;
        } else {
            return false;
        }
    }

    template <typename T>
    static constexpr bool member(T &destination,
                                 std::string_view key,
                                 std::string_view value) noexcept
    {
        return YamlObjectReader::readScalar(destination, key, value);
    }
};

} // namespace job::yaml