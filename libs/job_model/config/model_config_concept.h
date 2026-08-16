#pragma once

#include <concepts>
#include <string_view>
#include <cstdint>

namespace job::model {

// Concepts are like a contract, but without the boring paperwork.
// If your config object doesn't walk the walk and talk the talk,
// the compiler will yell at you before it even touches the linker.
template<typename T>
concept ModelConfigConcept = requires(const T &config) {
    { config.isValid() }          -> std::same_as<bool>;
    { config.architectureName() } -> std::convertible_to<std::string_view>;
};

// A specialized contract for transformer-based architectures.
// Because if you are missing layer counts or hidden dimensions,
// you aren't building a transformer—you're just building an expensive random number generator.
template<typename T>
concept TransformerConfigConcept = ModelConfigConcept<T> && requires(const T &config) {
    { config.blockCount() }       -> std::convertible_to<uint32_t>;
    { config.embeddingLength() }  -> std::convertible_to<uint32_t>;
    { config.vocabSize() }        -> std::convertible_to<uint32_t>;
    { config.contextLength() }    -> std::convertible_to<uint32_t>;
};

} // namespace job::model