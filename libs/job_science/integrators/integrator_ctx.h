#pragma once
#include <memory>
#include <job_thread_pool.h>
#include "data/particle.h"
#include "data/disk.h"
#include "data/zones.h"
namespace job::science::integrators {
// Everything the integrator needs to know about "the world" in one bundle.
// Engine builds this per step and hands it to the strategy.
struct IntegratorCtx {
    threads::ThreadPool::Ptr    pool;          // Threading substrate (work stealing / fifo / etc.)
    data::DiskModel             &disk;          // Central star + disk parameters
    data::Zones                 &zones;         // Thermal / compositional zones
    data::Particles             &particles;     // The actual N-body state we’re integrating -> std::vector<Particle>;
    bool                        prime;          // prime the loop on construcxtions
};
}
