#pragma once

#include <meta>
#include <string_view>
#include <utility>

namespace job::yaml {

class YamlKeyDispatch
{
public:
    template <typename T, typename F>
    static constexpr bool dispatch(std::string_view key, F &&function)
    {
        template for (constexpr auto member :
                      std::define_static_array(
                          std::meta::nonstatic_data_members_of(
                              ^^T,
                              std::meta::access_context::current()))) {
            constexpr std::string_view name = std::meta::identifier_of(member);

            if (key == name) {
                std::forward<F>(function).template operator()<member>();
                return true;
            }
        }

        return false;
    }
};

}