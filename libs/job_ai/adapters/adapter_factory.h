#pragma once
#include <memory>

#include <job_logger.h>

#include "iadapter.h"
#include "adapter_types.h"

// machine learning style
#include "dense_adapter.h"
#include "flash_adapter.h"
#include "lowrank_adapter.h"

// physics style
#include "fmm_adapter.h"
#include "verlet_adapter.h"
#include "bh_adapter.h"
#include "stencil_adapter.h"
#include "rk4_adapter.h"
namespace job::ai::adapters {

[[nodiscard]] inline std::unique_ptr<IAdapter> makeAdapter(AdapterType adapterType)
{
    switch (adapterType) {
    case AdapterType::None:
        JOB_LOG_ERROR("[makeAdapter] Unknown AdapterType: None");
        return nullptr;
    case AdapterType::Dense:
        return DenseAdapter::unique();
    case AdapterType::Flash:
        return FlashAdapter::unique();
    case AdapterType::LowRank:
        return LowRankAdapter::unique();
    case AdapterType::FMM:
        return FmmAdapter::unique();
    case AdapterType::BarnesHut:
        return BhAdapter::unique();
    case AdapterType::Verlet:
        return VerletAdapter::unique();
    case AdapterType::Stencil:
        return StencilAdapter::unique();
    case AdapterType::RK4:
        return Rk4Adapter::unique();
    default:
        JOB_LOG_ERROR("[makeAdapter] Unknown AdapterType: {}", static_cast<int>(adapterType));
        return nullptr;
    }
    return nullptr;
}

}
