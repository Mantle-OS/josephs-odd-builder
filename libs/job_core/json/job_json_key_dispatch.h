#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

#include "job_obj_annotation.h"
#include "job_obj_concept.h"

namespace job::json {

enum class JsonKeyDispatchResult : std::uint8_t
{
    NotFound = 0,
    Accepted,
    Rejected,
};

class JsonKeyDispatch
{
public:
    JsonKeyDispatch() = delete;
    ~JsonKeyDispatch() = delete;

    JsonKeyDispatch(const JsonKeyDispatch &) = delete;
    JsonKeyDispatch &operator=(const JsonKeyDispatch &) = delete;
    JsonKeyDispatch(JsonKeyDispatch &&) = delete;
    JsonKeyDispatch &operator=(JsonKeyDispatch &&) = delete;

    template <typename T, typename Function>
    [[nodiscard]] static constexpr JsonKeyDispatchResult dispatch(T &object, std::string_view key, Function &&function)
    {
        using ObjectType = std::remove_cvref_t<T>;

        JsonKeyDispatchResult result = JsonKeyDispatchResult::NotFound;

        template for (constexpr auto member : job::core::reflectedDataMembersV<ObjectType>) {
            if constexpr (!job::core::isSerializableMember<member>()) {
                continue;
            } else {
                if (result != JsonKeyDispatchResult::NotFound)
                    continue;

                constexpr std::string_view name = std::meta::identifier_of(member);

                if (key != name)
                    continue;

                using MemberReference = decltype((object.[:member:]));
                using ResultType = std::invoke_result_t<Function &, MemberReference>;

                if constexpr (std::convertible_to<ResultType, bool>) {
                    result = std::invoke(function, object.[:member:]) ?
                                 JsonKeyDispatchResult::Accepted :
                                 JsonKeyDispatchResult::Rejected;
                } else {
                    std::invoke(function, object.[:member:]);
                    result = JsonKeyDispatchResult::Accepted;
                }
            }
        }

        return result;
    }
};

} // namespace job::json