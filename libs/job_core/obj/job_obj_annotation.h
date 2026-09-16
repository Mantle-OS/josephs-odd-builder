#pragma once

#include <meta>

namespace job::core {

// =============================================================================
// Object Annotations
// =============================================================================

struct NoSerialize {};
struct NoReset {};
struct Required {};

// =============================================================================
// Annotation Helpers
// =============================================================================

template <typename Annotation>
[[nodiscard]] consteval bool hasAnnotation(std::meta::info member)
{
    return !std::meta::annotations_of_with_type(member, ^^Annotation).empty();
}

[[nodiscard]] consteval bool hasNoSerializeAnnotation(std::meta::info member)
{
    return hasAnnotation<NoSerialize>(member);
}

[[nodiscard]] consteval bool hasNoResetAnnotation(std::meta::info member)
{
    return hasAnnotation<NoReset>(member);
}

[[nodiscard]] consteval bool hasRequiredAnnotation(std::meta::info member)
{
    return hasAnnotation<Required>(member);
}

} // namespace job::core