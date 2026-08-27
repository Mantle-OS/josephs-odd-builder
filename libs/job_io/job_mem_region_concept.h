#pragma once

#include <concepts>
#include <cstddef>

#include "job_mem_range.h"

namespace job::io {

template <typename T>
concept JobMemRegion = requires(const T &region)
{
    { region.range() } -> std::same_as<const JobMemRange &>;
    { region.size() } -> std::convertible_to<std::size_t>;
};

} // namespace job::io