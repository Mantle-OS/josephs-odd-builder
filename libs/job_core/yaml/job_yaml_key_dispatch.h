#pragma once

#include <meta>
#include <string_view>
#include <utility>
#include <type_traits>

#include "job_obj_annotation.h"
#include "job_obj_concept.h"

namespace job::yaml {

class YamlKeyDispatch
{
public:
    template <typename T, typename F>
    static constexpr bool dispatch(std::string_view key, F &&function)
    {
        using ObjectType = std::remove_cvref_t<T>;

        template for (constexpr auto member : job::core::reflectedDataMembersV<ObjectType>) {
            using MemberType = typename[:std::meta::type_of(member):];

            if constexpr (job::core::SignalType<MemberType> ||
                          job::core::hasNoSerializeAnnotation(member))
                continue;

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