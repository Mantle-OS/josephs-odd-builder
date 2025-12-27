#pragma once

#include <memory>

#include <job_thread_pool.h>

#include <data/particle.h>
#include <data/disk.h>
#include <data/zones.h>

#include "engine_config.h"

namespace job::science::engine {

using job::threads::ThreadPool;
using job::science::data::Particles;
using job::science::data::DiskModel;
using job::science::data::Zones;


/// Everything the integrator needs to know about "the world" in one bundle.
/// Engine builds this per step and hands it to the strategy.
struct IntegratorContext {
    ThreadPool  &pool;   ///< Threading substrate (work stealing / fifo / etc.)
    DiskModel   &disk;   ///< Central star + disk parameters
    Zones       &zones;  ///< Thermal / compositional zones
    Particles&  particles; ///< The actual N-body state we’re integrating
};

/// Optional: stats the engine can read back after a step.
/// (This is intentionally tiny; we can grow it later if useful.)
struct IntegratorStats {
    std::uint64_t stepIndex{0};
    float         lastStepSeconds{0.0f};  ///< Wall-clock, for profiling/GUI
};

/// Abstract “strategy” for advancing the particle system by one time step.
/// Concrete implementations will wrap:
///   - Velocity Verlet integrator
///   - RK4
///   - Barnes–Hut N-body
///   - FMM engine
///   - (yes, even Euler, for sins and regression tests)
class IIntegrator {
public:
    using Ptr = std::shared_ptr<IIntegrator>;

    virtual ~IIntegrator() = default;

    /// Which integrator this instance represents (for logging / UI).
    [[nodiscard]] virtual IntegratorType type() const noexcept = 0;

    /// Advance the system by dt_s *in-place*.
    ///
    /// Contract:
    ///   - ctx.particles, ctx.disk, ctx.zones are owned by Engine.
    ///   - Strategy may use ctx.pool however it likes (parallel_for, graphs, etc.).
    ///   - On return, particles should represent state at t + dt_s.
    virtual void step(IntegratorContext &ctx, float dt_s) = 0;

    /// Optional hook: called when Engine wants to reset timers, counters, etc.
    virtual void reset() noexcept { m_stats = {}; }

    /// Optional: surface stats back to Engine / GUI.
    [[nodiscard]] const IntegratorStats &stats() const noexcept { return m_stats; }

protected:
    IntegratorStats m_stats{};
};

/// Factory for a concrete strategy given engine configuration.
/// Engine will typically call this once in its ctor, or whenever the user
/// switches integrator type at runtime.
[[nodiscard]] IIntegrator::Ptr makeIntegratorStrategy(const EngineConfig &config, ThreadPool &pool);

} // namespace job::science::engine
