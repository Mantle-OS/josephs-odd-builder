#pragma once

#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <filesystem>

#include <real_type.h>

#include <job_thread_pool.h>

#include <sched/job_scheduler_types.h> //      Fifo, RoundRobin, WorkStealing, Sporadic


#include "data/particle.h"
#include "data/disk.h"
#include "data/zones.h"
#include "data/composition.h"
#include "data/vec3f.h"

#include "frames/frame_serializer.h"
#include "frames/frame_deserializer.h"



namespace job::science::engine {
using namespace job::science::data;
using namespace job::science::frames;
using namespace job::threads;

enum class ThreadingAlgo : std::uint8_t {
    DataParallelism,      // parallel_for
    Reductions,           // parallel_reduce
    Graph,                // JobThreadGraph / DAG-style job system
    Pipeline,             // JobPipeline + JobPipelineStage + sinks
    Barnes_N_Hut,         // Barnes–Hut N-body
    FastMultipoleMethod,  // Here is where the math dragon lives
    WorkStealing,         // JobWorkStealingScheduler
    MapReduce,            // map + parallel_reduce
    ProducerConsumer,     // JobTaskQueue / JobBoundedMPMCQueue
    FuturesPromises,      // ThreadPool::submit + std::future
    AsyncIO,              // AsyncEventLoop / JobIoAsyncThread (epoll/eventfd)
    Bfs,                  // level-synchronous parallel_bfs
    Dijkstra,             // parallelDijkstra (delta-stepping style)
    BranchAndBound,       // parallel_branch_and_bound
    StencilGrid2D,        // JobStencilGrid2D
    StencilGrid3D,        // JobStencilGrid3D
    CellularAutomata,     // CA on top of stencil (Game of Life, etc.)
    VelocityVerlet,       // KDK/DKD
    RungeKutta4,          // JobRK4Integrator
    ReaderWriterLock,     // std::shared_mutex usage
    ForkJoin,             // fork-join via ThreadPool + parallel_* helpers
    ActorModel,           // JobThreadActor
    Dataflow,             // pipeline / actor network, Kahn-ish
    Euler,                 // go away
    MonteCarloTreeSearch // executor ready, API "here be dragons"
};


// do we force the users into picking out the scheduler or use the new ctx/[job_fifo_ctx.h,  job_rr_ctx.h,  job_sporadic_ctx.h,  job_stealing_ctx.h]
// inline constexpr kThreadSchedMap std::hash<ThreadingAlgo, SchedulerType> = {};

enum class SyncType : std::uint8_t {
    Socket = 0,
    IO     = 1
};

// Sub sync
enum class IoTypes : std::uint8_t {
    File,
    Uart,
    Memory
    // CarrierPigeon ---- backlog: Homing in on hardware support
};

// Sub sync
enum class SocketTypes : std::uint8_t {
    Tcp,
    Udp,
    Unix
};


class Engine final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Callbacks
    using EngineCallback = std::function<void()>;
    using ParticleCallback = std::function<void(const Particles&)>;

    explicit Engine(const DiskModel &diskModel,
                    const Zones &zones,
                    size_t particleCount = 1024);

    ~Engine();

    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void start() noexcept;
    void stop() noexcept;
    void kill() noexcept;
    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] bool pause() noexcept;
    [[nodiscard]] bool play() noexcept;
    [[nodiscard]] bool ffwd(core::real_t multiplier) noexcept;
    [[nodiscard]] bool rwd(core::real_t multiplier) noexcept;

    [[nodiscard]] FrameSerializer::Ptr writer() const noexcept;
    [[nodiscard]] FrameDeserializer::Ptr reader() const noexcept;
    [[nodiscard]] std::filesystem::path baseDir() const;
    void reset();

    [[nodiscard]] const Particles &particles() const noexcept;
    [[nodiscard]] const DiskModel &disk() const noexcept;
    [[nodiscard]] const Zones &zones() const noexcept;

    // Callback setters
    void setParticleCallback(ParticleCallback cb);
    void setDiskCallback(EngineCallback cb);
    void setZonesCallback(EngineCallback cb);

    [[nodiscard]] core::real_t timeStep() const noexcept;
    void setTimeStep(core::real_t dt_s) noexcept;

    void step(core::real_t dt_s);
private:
    void calculateForces(Particle &p);
    void runLoop();

    // Services
    ThreadPool::Ptr         m_pool;
    FrameSerializer::Ptr    m_writer;
    FrameDeserializer::Ptr  m_reader;
    Particles               m_particles;
    std::vector<Vec3f>      m_prev_accel;
    DiskModel               m_disk;
    Zones                   m_zones;
    std::atomic<bool>       m_running{false};
    std::atomic<bool>       m_paused{false};
    core::real_t            m_dt{core::real_t(1.0)}; // Timestep
    ParticleCallback        m_particleCallback;
    EngineCallback          m_diskCallback;
    EngineCallback          m_zonesCallback;
};

}
