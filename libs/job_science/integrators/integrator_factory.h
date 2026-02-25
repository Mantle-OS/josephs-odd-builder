#pragma once

#include <memory>

#include <job_thread_pool.h>

#include "engine/engine_config.h"

#include "iintegrator.h"



namespace job::science::integrators {
[[nodiscard]] std::unique_ptr<IIntegrator> create(const engine::EngineConfig &cfg);
}
