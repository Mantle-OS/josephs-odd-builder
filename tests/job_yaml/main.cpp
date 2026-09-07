#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include "../tests-fast-math-workaround.h"

static_assert(isSafePositiveInfinity(safeInfinity<float>()));
static_assert(isSafePositiveInfinity(safeInfinity<double>()));

static_assert(isSafeNegativeInfinity(safeNegativeInfinity<float>()));
static_assert(isSafeNegativeInfinity(safeNegativeInfinity<double>()));

static_assert(isSafeNaN(safeNaN<float>()));
static_assert(isSafeNaN(safeNaN<double>()));