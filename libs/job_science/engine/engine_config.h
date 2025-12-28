#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include <sched/job_scheduler_types.h>

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

/// Full engine configuration bundle.
/// Engine can take this in its ctor or via a setter, and use it to:
///   - build a ThreadPool
///   - choose an IIntegrator
///   - optionally wire frame sinks/sources
struct EngineConfig {
    EngineThreadingConfig threading{};
    EngineIOConfig        io{};
    IntegratorType        integrator{IntegratorType::VelocityVerlet};
};

} // namespace job::science::engine
