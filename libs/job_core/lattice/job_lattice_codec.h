#pragma once

#include <concepts>

namespace job::core::lattice {

template <typename T>
concept LatticeCodec = requires {
    typename T::Result;
    typename T::UnitInput;
    typename T::UnitOutput;
    typename T::ScalarInput;
    typename T::ScalarOutput;
} && requires(typename T::UnitInput unitInput,
                       typename T::UnitOutput unitOutput,
                       typename T::ScalarInput scalarInput,
                       typename T::ScalarOutput scalarOutput) {
    { T::decode(unitInput, scalarOutput) } -> std::same_as<typename T::Result>;
    { T::encode(scalarInput, unitOutput) } -> std::same_as<typename T::Result>;
    { T::validate(unitInput) } -> std::same_as<typename T::Result>;
};

} // namespace job::core::lattice