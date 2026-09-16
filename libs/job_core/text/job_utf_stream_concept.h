#pragma once

#include <concepts>
#include <cstdint>

#include "job_lattice_codec.h"
#include "job_unicode_scalar.h"

namespace job::core::text {

template <typename T>
concept UtfStreamCodec =
    lattice::LatticeCodec<T> &&
    std::same_as<typename T::ScalarInput::value_type, UnicodeScalar> &&
    std::same_as<typename T::ScalarOutput::value_type, UnicodeScalar> &&
    (std::same_as<typename T::UnitInput::value_type, std::uint8_t> ||
     std::same_as<typename T::UnitInput::value_type, char16_t> ||
     std::same_as<typename T::UnitInput::value_type, char32_t>) &&
    std::same_as<typename T::UnitInput::value_type,
                 typename T::UnitOutput::value_type>;

} // namespace job::core::text