#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#include "job_schema.h"
#include "job_schema_types.h"

namespace job::schema {

// =============================================================================
// Schema Type
// =============================================================================

template <typename T>
concept SchemaType =
    std::is_class_v<std::remove_cvref_t<T>> &&
    std::derived_from<std::remove_cvref_t<T>, JobSchema> &&
    std::destructible<std::remove_cvref_t<T>>;

// =============================================================================
// Pointer Types
// =============================================================================

template <typename T>
concept RawPointerType = std::is_pointer_v<std::remove_cvref_t<T>>;

template <typename T>
struct IsSharedPtr : std::false_type {
};

template <typename T>
struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

template <typename T>
inline constexpr bool isSharedPointerTypeV = IsSharedPtr<std::remove_cvref_t<T>>::value;

template <typename T>
concept SharedPointerType = isSharedPointerTypeV<T>;

template <typename T>
struct IsUniquePtr : std::false_type {};

template <typename T, typename Deleter>
struct IsUniquePtr<std::unique_ptr<T, Deleter>> : std::true_type {};

template <typename T>
inline constexpr bool isUniquePointerTypeV =
    IsUniquePtr<std::remove_cvref_t<T>>::value;

template <typename T>
concept UniquePointerType = isUniquePointerTypeV<T>;

// =============================================================================
// Factory Construction
// =============================================================================

template <typename T>
concept SharedFactoryType =
    requires {
        { T::createShared() } -> std::same_as<std::shared_ptr<T>>;
    };

template <typename T>
concept UniqueFactoryType =
    requires {
        { T::createUniq() } -> std::same_as<std::unique_ptr<T>>;
    };

} // namespace job::schema