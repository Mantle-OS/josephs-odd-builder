#pragma once
#include "adapter_types.h"
namespace job::ai::router {
// VERY ALPHA Shit is getting odd bro .... whos on 1st or 2nd ?
struct RouterExpertConfig {
    int                         id{0};                                  // expert index
    adapters::AdapterType       adapter{adapters::AdapterType::None};   // Dense / Flash / FMM / BH
    // FIXME bounds, region center, etc. for spatial routing
};
} // namespace job::ai::router
