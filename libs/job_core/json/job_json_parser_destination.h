#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "job_json_concepts.h"
#include "job_json_key_dispatch.h"
#include "job_json_number_kernel.h"

    namespace job::json {

    template <typename T>
    class JsonParserDestination
    {
    public:
        using ValueType = JsonType<T>;

        explicit constexpr JsonParserDestination(T &value) noexcept :
            m_value(value)
        {
        }

        constexpr ~JsonParserDestination() = default;

        JsonParserDestination(const JsonParserDestination &) = delete;
        JsonParserDestination &operator=(const JsonParserDestination &) = delete;
        constexpr JsonParserDestination(JsonParserDestination &&) noexcept = default;
        JsonParserDestination &operator=(JsonParserDestination &&) = delete;

        [[nodiscard]] constexpr T &value() noexcept
        {
            return m_value;
        }

        [[nodiscard]] constexpr const T &value() const noexcept
        {
            return m_value;
        }

        template <typename Function>
        [[nodiscard]] bool withValue(Function &&function)
        {
            if constexpr (JsonOptional<T>) {
                using InnerType = JsonOptionalValue<T>;
                using DestinationType = JsonParserDestination<InnerType>;

                if (!m_value) {
                    if constexpr (std::default_initializable<InnerType>) {
                        m_value.emplace();
                    } else {
                        return false;
                    }
                }

                DestinationType destination{*m_value};
                return destination.withValue(std::forward<Function>(function));
            } else if constexpr (JsonSharedPointer<T>) {
                using ElementType = JsonSharedPointerElement<T>;
                using DestinationType = JsonParserDestination<ElementType>;

                if (!m_value) {
                    if constexpr (std::default_initializable<ElementType>) {
                        m_value = std::make_shared<ElementType>();
                    } else {
                        return false;
                    }
                }

                DestinationType destination{*m_value};
                return destination.withValue(std::forward<Function>(function));
            } else if constexpr (JsonUniquePointer<T>) {
                using ElementType = JsonUniquePointerElement<T>;
                using DestinationType = JsonParserDestination<ElementType>;

                if (!m_value) {
                    if constexpr (
                        std::default_initializable<ElementType> &&
                        requires {
                            ValueType{new ElementType{}};
                        }) {
                        m_value = ValueType{new ElementType{}};
                    } else {
                        return false;
                    }
                }

                DestinationType destination{*m_value};
                return destination.withValue(std::forward<Function>(function));
            } else {
                using DestinationType = JsonParserDestination<T>;

                static_assert(
                    std::convertible_to<
                        std::invoke_result_t<Function &, DestinationType &>,
                        bool>,
                    "JsonParserDestination::withValue callback must return a bool-convertible result");

                return std::invoke(std::forward<Function>(function), *this);
            }
        }


        // template <typename Function>
        // [[nodiscard]] bool withValue(Function &&function)
        // {
        //     if constexpr (JsonOptional<T>) {
        //         using InnerType = JsonOptionalValue<T>;
        //         using DestinationType = JsonParserDestination<InnerType>;

        //         static_assert(
        //             std::convertible_to<
        //                 std::invoke_result_t<Function &, DestinationType &>,
        //                 bool>,
        //             "JsonParserDestination::withValue callback must return a bool-convertible result");

        //         if (!m_value) {
        //             if constexpr (std::default_initializable<InnerType>) {
        //                 m_value.emplace();
        //             } else {
        //                 return false;
        //             }
        //         }

        //         // DestinationType destination{*m_value};
        //         // return std::invoke(std::forward<Function>(function), destination);

        //         DestinationType destination{*m_value};
        //         return destination.withValue(std::forward<Function>(function));

        //     } else if constexpr (JsonSharedPointer<T>) {
        //         using ElementType = JsonSharedPointerElement<T>;
        //         using DestinationType = JsonParserDestination<ElementType>;

        //         static_assert(
        //             std::convertible_to<
        //                 std::invoke_result_t<Function &, DestinationType &>,
        //                 bool>,
        //             "JsonParserDestination::withValue callback must return a bool-convertible result");

        //         if (!m_value) {
        //             if constexpr (std::default_initializable<ElementType>) {
        //                 m_value = std::make_shared<ElementType>();
        //             } else {
        //                 return false;
        //             }
        //         }

        //         DestinationType destination{*m_value};
        //         return std::invoke(std::forward<Function>(function), destination);
        //     } else if constexpr (JsonUniquePointer<T>) {
        //         using ElementType = JsonUniquePointerElement<T>;
        //         using DestinationType = JsonParserDestination<ElementType>;

        //         static_assert(
        //             std::convertible_to<
        //                 std::invoke_result_t<Function &, DestinationType &>,
        //                 bool>,
        //             "JsonParserDestination::withValue callback must return a bool-convertible result");

        //         if (!m_value) {
        //             if constexpr (
        //                 std::default_initializable<ElementType> &&
        //                 requires {
        //                     ValueType{new ElementType{}};
        //                 }) {
        //                 m_value = ValueType{new ElementType{}};
        //             } else {
        //                 return false;
        //             }
        //         }

        //         DestinationType destination{*m_value};
        //         return std::invoke(std::forward<Function>(function), destination);
        //     } else {
        //         using DestinationType = JsonParserDestination<T>;

        //         static_assert(
        //             std::convertible_to<
        //                 std::invoke_result_t<Function &, DestinationType &>,
        //                 bool>,
        //             "JsonParserDestination::withValue callback must return a bool-convertible result");

        //         return std::invoke(std::forward<Function>(function), *this);
        //     }
        // }

        [[nodiscard]] bool null()
        {
            if constexpr (JsonOptional<T>) {
                m_value.reset();
                return true;
            } else if constexpr (JsonSharedPointer<T> || JsonUniquePointer<T>) {
                m_value.reset();
                return true;
            } else {
                return false;
            }
        }

        [[nodiscard]] bool boolean(bool value)
        {
            if constexpr (JsonOptional<T> || JsonOwningPointer<T>) {
                return withValue([value](auto &destination) {
                    return destination.boolean(value);
                });
            } else if constexpr (JsonBoolean<T>) {
                m_value = value;
                return true;
            } else {
                return false;
            }
        }

        [[nodiscard]] bool number(std::string_view value)
        {
            if constexpr (JsonOptional<T> || JsonOwningPointer<T>) {
                return withValue([value](auto &destination) {
                    return destination.number(value);
                });
            } else if constexpr (JsonNumber<T>) {
                return JsonNumberKernel::parse(value, m_value);
            } else if constexpr (JsonEnum<T>) {
                using UnderlyingType = std::underlying_type_t<ValueType>;

                UnderlyingType parsed{};
                if (!JsonNumberKernel::parse(value, parsed))
                    return false;

                m_value = static_cast<ValueType>(parsed);
                return true;
            } else if constexpr (JsonByte<T>) {
                std::uint8_t parsed{};
                if (!JsonNumberKernel::parse(value, parsed))
                    return false;

                m_value = static_cast<std::byte>(parsed);
                return true;
            } else if constexpr (JsonExtendedChar<T>) {
                return parseExtendedChar(value);
            } else {
                return false;
            }
        }

        [[nodiscard]] bool string(std::string_view value)
        {
            if constexpr (JsonOptional<T> || JsonOwningPointer<T>) {
                return withValue([value](auto &destination) {
                    return destination.string(value);
                });
            } else if constexpr (JsonOwnedString<T>) {
                m_value.assign(value);
                return true;
            } else if constexpr (JsonStringView<T>) {
                m_value = value;
                return true;
            } else {
                return false;
            }
        }

        [[nodiscard]] bool stringOwned(std::string &&value)
        {
            if constexpr (JsonOptional<T> || JsonOwningPointer<T>) {
                return withValue([value = std::move(value)](auto &destination) mutable {
                    return destination.stringOwned(std::move(value));
                });
            } else if constexpr (JsonOwnedString<T>) {
                m_value = std::move(value);
                return true;
            } else {
                return false;
            }
        }

    private:
        [[nodiscard]] bool parseExtendedChar(std::string_view value)
            requires JsonExtendedChar<T>
        {
            if constexpr (std::numeric_limits<ValueType>::is_signed) {
                std::int64_t parsed{};

                if (!JsonNumberKernel::parse(value, parsed))
                    return false;

                if (parsed < static_cast<std::int64_t>(std::numeric_limits<ValueType>::min()) ||
                    parsed > static_cast<std::int64_t>(std::numeric_limits<ValueType>::max()))
                    return false;

                m_value = static_cast<ValueType>(parsed);
                return true;
            } else {
                std::uint64_t parsed{};

                if (!JsonNumberKernel::parse(value, parsed))
                    return false;

                if (parsed > static_cast<std::uint64_t>(std::numeric_limits<ValueType>::max()))
                    return false;

                m_value = static_cast<ValueType>(parsed);
                return true;
            }
        }

        T &m_value;
    };

    template <typename T>
    class JsonObjectParserDestination
    {
    public:
        using ObjectType = JsonType<T>;

        explicit constexpr JsonObjectParserDestination(T &object) noexcept :
            m_object(object)
        {
        }

        constexpr ~JsonObjectParserDestination() = default;

        JsonObjectParserDestination(const JsonObjectParserDestination &) = delete;
        JsonObjectParserDestination &operator=(const JsonObjectParserDestination &) = delete;
        constexpr JsonObjectParserDestination(JsonObjectParserDestination &&) noexcept = default;
        JsonObjectParserDestination &operator=(JsonObjectParserDestination &&) = delete;

        [[nodiscard]] constexpr T &object() noexcept
        {
            return m_object;
        }

        [[nodiscard]] constexpr const T &object() const noexcept
        {
            return m_object;
        }

        template <typename Function>
        [[nodiscard]] constexpr JsonKeyDispatchResult dispatch(std::string_view key, Function &&function)
        {
            return JsonKeyDispatch::dispatch(m_object, key, std::forward<Function>(function));
        }

        [[nodiscard]] JsonKeyDispatchResult null(std::string_view key)
        {
            return dispatchValue(key, [](auto &destination) {
                return destination.null();
            });
        }

        [[nodiscard]] JsonKeyDispatchResult boolean(std::string_view key, bool value)
        {
            return dispatchValue(key, [value](auto &destination) {
                return destination.boolean(value);
            });
        }

        [[nodiscard]] JsonKeyDispatchResult number(std::string_view key, std::string_view value)
        {
            return dispatchValue(key, [value](auto &destination) {
                return destination.number(value);
            });
        }

        [[nodiscard]] JsonKeyDispatchResult string(std::string_view key, std::string_view value)
        {
            return dispatchValue(key, [value](auto &destination) {
                return destination.string(value);
            });
        }

        [[nodiscard]] JsonKeyDispatchResult stringOwned(std::string_view key, std::string &&value)
        {
            return dispatchValue(key, [value = std::move(value)](auto &destination) mutable {
                return destination.stringOwned(std::move(value));
            });
        }

        template <typename Function>
        [[nodiscard]] JsonKeyDispatchResult withValue(std::string_view key, Function &&function)
        {
            return dispatchValue(key, [&](auto &destination) {
                return destination.withValue(std::forward<Function>(function));
            });
        }

    private:
        template <typename Function>
        [[nodiscard]] JsonKeyDispatchResult dispatchValue(std::string_view key, Function &&function)
        {
            return dispatch(key, [&](auto &member) {
                using MemberType = std::remove_cvref_t<decltype(member)>;
                using DestinationType = JsonParserDestination<MemberType>;

                DestinationType destination{member};
                return std::invoke(function, destination);
            });
        }

        T &m_object;
    };

} // namespace job::json

