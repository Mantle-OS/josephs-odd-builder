#pragma once

#include <concepts>
#include <cstdint>

#include "job_input_tv.h"
#include "job_input_traits.h"

namespace job::input {
template <typename T>
concept JobInputConcept = requires(const T &event) {
    { event.time() }  -> std::convertible_to<JobTimeValue>;
    { event.type() }  -> std::convertible_to<JobInputType>;
    { event.code() }  -> std::convertible_to<JobInputCode>;
    { event.value() } -> std::convertible_to<JobInputValue>;
};

} // namespace job::input