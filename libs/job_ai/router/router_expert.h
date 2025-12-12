#pragma once
#include "adapter_types.h"
namespace job::ai::router {
// VERY ALPHA
struct RouterExpertConfig {
    int                         id{0};                                  // expert index
    adapters::AdapterType       adapter{adapters::AdapterType::None};   // Dense / Flash / FMM / BH
    // FIXME bounds, region center, etc. for spatial routing later if even needed
};
} // namespace job::ai::router
