#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <meta>

#include "job_yaml_concepts.h"
#include "job_yaml_key_dispatch.h"
#include "job_yaml_scalar_kernel.h"
namespace job::yaml {

class YamlValidateSink
{
public:
    template <typename T>
    static constexpr bool scalar(std::string_view value) noexcept
    {
        using DestinationType = YamlType<T>;

        if constexpr (YamlBool<DestinationType>) {
            bool destination{};
            return YamlScalarKernel::parseBool(value, destination);
        } else if constexpr (YamlInteger<DestinationType>) {
            DestinationType destination{};
            return YamlScalarKernel::parseInteger(value, destination);
        } else if constexpr (YamlFloatingPoint<DestinationType>) {
            DestinationType destination{};
            return YamlScalarKernel::parseFloat(value, destination);
        } else if constexpr (YamlOwnedString<DestinationType>) {
            return true;
        } else {
            return false;
        }
    }

    template <typename T>
    static constexpr bool member(std::string_view key, std::string_view value) noexcept
    {
        bool valid = false;

        const bool matched = YamlKeyDispatch::dispatch<T>(key, [&]<auto member> {
            using MemberType = YamlType<typename[:std::meta::type_of(member):]>;
            valid = scalar<MemberType>(value);
        });

        return matched && valid;
    }
};

} // namespace job::yaml