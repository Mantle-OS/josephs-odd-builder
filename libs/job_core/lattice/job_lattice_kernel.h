#pragma once

#include "job_lattice_transform_result.h"
#include "job_lattice_view.h"

namespace job::core::lattice {

template <typename T>
using LatticeKernelInput = LatticeView<const T>;

template <typename T>
using LatticeKernelOutput = LatticeView<T>;

template <typename Status>
using LatticeKernelResult = LatticeTransformResult<Status>;

} // namespace job::core::lattice