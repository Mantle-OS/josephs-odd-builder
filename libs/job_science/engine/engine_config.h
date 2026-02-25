#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

// job::threads
#include <sched/job_scheduler_types.h>
#include <utils/job_thread_options.h>

namespace job::science::engine {

enum class ThreadingAlgo : std::uint8_t {
    Auto = 0,
    DataParallelism,
    Graph,
    Pipeline,
    ActorModel
};

enum class IntegratorType : std::uint8_t {
    VelocityVerlet = 0,
    RungeKutta4,
    BarnesHut,
    FastMultipoleMethod,
    Euler                 // the “go away” mode
};

enum class SyncType : std::uint8_t {
    None  = 0,
    Socket,
    IO
};

enum class IoType : std::uint8_t {
    File   = 0,
    Uart,
    Memory
    // CarrierPigeon ---- backlog: "Homing" in on hardware support
};

/// Subtype of socket when SyncType == Socket.
enum class SocketType : std::uint8_t {
    Tcp  = 0,
    Udp,
    Unix
};

/// Threading configuration for Engine.
/// This does *not* own any threads; it just describes how to build a ThreadPool.
struct EngineThreadingConfig {
    ThreadingAlgo               algo{ThreadingAlgo::DataParallelism};
    job::threads::SchedulerType scheduler{job::threads::SchedulerType::Fifo};
    std::uint32_t               workerCount{0};  ///< 0 => use hardware_concurrency()

    job::threads::JobThreadOptions workerOptions{job::threads::JobThreadOptions::normal()};
    job::threads::JobThreadOptions driverOptions{job::threads::JobThreadOptions::normal()};
};


/// Frame IO configuration for Engine.
/// The engine will later use this to decide which IFrameSink/IFrameSource
/// to construct (File vs. Socket, etc.).
struct EngineIOConfig {
    SyncType   sync{SyncType::None};

    // When sync == SyncType::IO
    IoType     ioType{IoType::File};

    // When sync == SyncType::Socket
    SocketType socketType{SocketType::Tcp};

    // Optional wiring details; meaning depends on sync/ioType/socketType.
    // For File IO: path to directory or file.
    std::filesystem::path filepath{};

    // For sockets: JobUrl-style endpoint string, e.g. "tcp://127.0.0.1:9000".
    std::string           endpoint{};
};


/**
 * Configuration for the Barnes-Hut algorithm.
 */
struct BarnesHutConfig {
    float theta{0.5f};        // Opening criterion (0.0 = exact N^2, 1.0 = very fast/approx)
    float G{6.67430e-11f};    // Gravitational constant
    float epsilonSq{1e-6f};   // Softening factor squared
};

/**
 * Configuration for the Fast Multipole Method.
 */
struct FmmConfig {
    float theta{0.6f};        //  Multipole acceptance criterion
    uint32_t maxLeafSize{4}; // Max particles per leaf before stopping tree split
    // Note: P (Expansion Order) is currently fixed at 3 in the kernel templates,
};


/// Full engine configuration bundle.
/// Engine can take this in its ctor or via a setter, and use it to:
///   - build a ThreadPool
///   - choose an IIntegrator
///   - optionally wire frame sinks/sources
struct EngineConfig {
    EngineThreadingConfig threading{};
    EngineIOConfig        io{};
    IntegratorType        integrator{IntegratorType::VelocityVerlet};
    // Sub-configs for specific solvers
    BarnesHutConfig       barnesHut{};
    FmmConfig             fmm{};

    // Global physics overrides
    float fixedDeltaTime{0.01667f};
};


[[nodiscard]] inline EngineConfig DefaultEngineConfig(
    threads::SchedulerType scheduler = threads::SchedulerType::Fifo,
    IntegratorType         integrator = IntegratorType::VelocityVerlet
    )
{
    EngineConfig cfg;
    cfg.integrator          = integrator;
    cfg.threading.scheduler = scheduler;
    cfg.io.sync             = SyncType::IO;
    cfg.io.ioType           = IoType::File;
    cfg.barnesHut.theta     = 0.5f;
    cfg.fmm.theta           = 0.6f;
    cfg.fmm.maxLeafSize     = 16;
    return cfg;
}

} // namespace job::science::engine
