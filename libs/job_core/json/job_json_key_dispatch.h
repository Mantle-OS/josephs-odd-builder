#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <meta>
#include <string_view>
#include <type_traits>
#include <utility>

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
        template <typename T, typename Function>
        [[nodiscard]] static constexpr JsonKeyDispatchResult dispatch(
            T &object,
            std::string_view key,
            Function &&function)
        {
            using ObjectType = std::remove_cvref_t<T>;

            JsonKeyDispatchResult result = JsonKeyDispatchResult::NotFound;

            template for (constexpr auto member : std::define_static_array(
                              std::meta::nonstatic_data_members_of(
                                  ^^ObjectType,
                                  std::meta::access_context::current()))) {
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

            return result;
        }
    };

} // namespace job::json

