#pragma once
#include "adapter_types.h"
namespace job::ai::router {
struct RouterExpertConfig {
    int id;                                    // expert index
    adapters::AdapterType adapter;             // Dense / Flash / FMM / BH
    // FIXME bounds, region center, etc. for spatial routing
};
} // namespace job::ai::router
