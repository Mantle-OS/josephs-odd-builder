#pragma once

#include <concepts>
#include <string>
#include <string_view>
#include <type_traits>

#include "job_yaml_key_dispatch.h"
#include "job_yaml_scalar_kernel.h"

namespace job::yaml {

class YamlObjectReader
{
public:
    YamlObjectReader() = delete;
    ~YamlObjectReader() = delete;

    YamlObjectReader(const YamlObjectReader &) = delete;
    YamlObjectReader &operator=(const YamlObjectReader &) = delete;
    YamlObjectReader(YamlObjectReader &&) = delete;
    YamlObjectReader &operator=(YamlObjectReader &&) = delete;

    template <typename T>
    [[nodiscard]] static constexpr bool readScalar(T &object, std::string_view key, std::string_view value) noexcept
    {
        bool converted = false;

        const bool matched = YamlKeyDispatch::dispatch<T>(key, [&]<auto member> {
            auto &destination = object.[:member:];
            using MemberType = std::remove_cvref_t<decltype(destination)>;

            if constexpr (std::same_as<MemberType, std::string>) {
                destination.assign(value);
                converted = true;
            } else if constexpr (std::same_as<MemberType, std::string_view>) {
                destination = value;
                converted = true;
            } else {
                converted = YamlScalarKernel::parse(value, destination);
            }
        });

        return matched && converted;
    }
};

} // namespace job::yaml