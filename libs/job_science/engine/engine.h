#pragma once

#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <filesystem>
#include <memory>

#include <real_type.h>
#include <random>
#include <cmath>

// job::threadings
#include <job_thread_pool.h>
#include <sched/job_scheduler_types.h> //      Fifo, RoundRobin, WorkStealing, Sporadic

// job::io
#include <file_io.h>

// job::science
#include "data/particle.h"
#include "data/disk.h"
#include "data/zones.h"
#include "data/composition.h"
#include "data/vec3f.h"

#include "frames/frame_serializer.h"
#include "frames/frame_deserializer.h"
// #include "frames/iframe_sink.h"
// #include "frames/iframe_source.h"

#include "frames/frame_sink_io.h"
#include "frames/frame_source_io.h"

#include "integrators/iintegrator.h"
#include "integrators/integrator_factory.h"

#include "engine/engine_config.h"
#include "engine/engine_threads.h"

namespace job::science::engine {
using namespace job::science::data;
using namespace job::science::frames;
using namespace job::threads;

class Engine final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Callbacks
    using EngineCallback = std::function<void()>;
    using ParticleCallback = std::function<void(const Particles&)>;



    explicit Engine(const data::DiskModel &diskModel,
                    const data::Zones &zones,
                    const EngineConfig &cfg,
                    size_t particleCount = 1024
        ):
        m_cfg(cfg),
        m_disk(diskModel),
        m_zones(zones)
    {
        // Default particle's
        m_particles.reserve(particleCount);
        initialize_particles(m_particles, m_disk, m_zones);

        // Threads
        updateWorkers();

        // Integrator
        updateIntegrator();

        // Frames
        updateFrameIo();
    }

    ~Engine()
    {
        kill();
    }
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    void start() noexcept
    {
        if (m_running.exchange(true))
            return;

        if (!m_worker->pool)
            updateWorkers();

        if (!m_integrator)
            updateIntegrator();

        if (!m_driver) {
            JobThreadOptions opts = JobThreadOptions::normal();
            // Give it a name so perf traces don’t look like a crime scene.
            std::snprintf(opts.name.data(), opts.name.size(), "job_engine");
            // heartbeat for JobThread::run()
            opts.heartbeat = 1;

            m_driver = std::make_shared<JobThread>(opts);
            m_driver->setRunFunction([this](std::stop_token token) {
                runLoop(token);
            });
        }

        (void)m_driver->start();
    }

    void stop() noexcept
    {
        if (!m_running.exchange(false, std::memory_order_acq_rel))
            return;

        m_paused.store(false, std::memory_order_release);

        if (m_driver) {
            m_driver->requestStop();
            (void)m_driver->join();
            // keep for restart
        }
    }

    void kill() noexcept
    {
        // make loop exit no matter what
        m_running.store(false, std::memory_order_release);
        m_paused.store(false, std::memory_order_release);

        // stop driver thread first
        if (m_driver) {
            m_driver->requestStop();
            (void)m_driver->join();
            m_driver.reset();
        }

        // tear down services that might use the pool
        m_integrator.reset();

        // flush/close frame IO (best-effort)
        if (m_writer)
            m_writer->flush();
        m_writer.reset();
        m_reader.reset();

        // now kill the pool + scheduler
        if (m_worker)
            m_worker->shutdown();
        m_worker.reset();
    }



    [[nodiscard]] bool running() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool paused() const noexcept
    {
        return m_paused.load(std::memory_order_acquire);
    }


    [[nodiscard]] bool pause() noexcept
    {
        if (!m_running.load(std::memory_order_acquire))
            return false;

        m_paused.store(true, std::memory_order_release);
        return true;
    }

    [[nodiscard]] bool play() noexcept
    {
        if (!m_running.load(std::memory_order_acquire))
            return false;

        m_paused.store(false, std::memory_order_release);
        return true;
    }



    [[nodiscard]] bool ffwd([[maybe_unused]]float multiplier) noexcept{return false;}
    [[nodiscard]] bool rwd([[maybe_unused]]float multiplier) noexcept{return false;}

    [[nodiscard]] FrameSerializer::Ptr writer() const noexcept { return m_writer; }
    [[nodiscard]] FrameDeserializer::Ptr reader() const noexcept { return m_reader; }
    [[nodiscard]] std::filesystem::path baseDir() const
    {
        return std::filesystem::temp_directory_path() / "job_science_output";
    }

    void reset()
    {
        stop();
        m_integratorPrimed = false;
        m_frameCounter.store(0, std::memory_order_relaxed);

        m_particles.clear();
        m_particles.reserve(m_particles.capacity());
        initialize_particles(m_particles, m_disk, m_zones);

        if (m_integrator)
            m_integrator->reset();
    }

    [[nodiscard]] const Particles &particles() const noexcept { return m_particles; }
    [[nodiscard]] const DiskModel &disk() const noexcept { return m_disk; }
    [[nodiscard]] const Zones &zones() const noexcept { return m_zones; }

    // Callback setters
    void setParticleCallback(ParticleCallback cb) { m_particleCallback = std::move(cb); }
    void setDiskCallback(EngineCallback cb)       { m_diskCallback = std::move(cb); }
    void setZonesCallback(EngineCallback cb)      { m_zonesCallback = std::move(cb); }

    // [[nodiscard]] float timeStep() const noexcept;
    // void setTimeStep(float dt_s) noexcept
    // {

    // }
    void step(float dt_s)
    {
        if (!(dt_s > 0.0f))
            return;

        if (!m_worker || !m_worker->pool) {
            updateWorkers();
            if (!m_worker || !m_worker->pool)
                return;
        }

        if (!m_integrator)
            updateIntegrator();

        if (!m_integrator)
            return;

        // Integrate
        integrators::IntegratorCtx ctx{
            m_worker->pool,
            m_disk,
            m_zones,
            m_particles,
            !m_integratorPrimed
        };

        if (ctx.prime) {
            m_integrator->prime(ctx);
            m_integratorPrimed = true;
        }

        m_integrator->step(ctx, dt_s);

        for (auto &p : m_particles) {
            const float r_au = DiskModelUtil::metersToAU(p.position.length());
            p.temperature    = DiskModelUtil::temperature(m_disk, r_au);
            p.zone           = ZonesUtil::classify(m_zones, p, m_disk);
        }

        // IO
        if (m_writer) {
            (void)m_writer->writeFrame(static_cast<std::uint32_t>(m_stepIndex), m_particles, m_disk, m_zones);
            m_writer->flush();
        }

        // Callbacks
        if (m_particleCallback)
            m_particleCallback(m_particles);

        if (m_diskCallback)
            m_diskCallback();

        if (m_zonesCallback)
            m_zonesCallback();

        ++m_stepIndex;
    }

    IntegratorType  currentIntegratorType()
    {
        return m_integrator->type();
    }

private:
    static void sleepNs(std::uint64_t ns) noexcept
    {
        timespec ts{};
        ts.tv_sec  = static_cast<time_t>(ns / 1'000'000'000ull);
        ts.tv_nsec = static_cast<long>(ns % 1'000'000'000ull);
        ::nanosleep(&ts, nullptr);
    }

    void runLoop(std::stop_token token) noexcept
    {
        // simple fixed-step loop for now:
        while (!token.stop_requested() && m_running.load(std::memory_order_relaxed)) {

            if (m_paused.load(std::memory_order_relaxed)) {
                sleepNs(5'000'000ull); // 5ms
                continue;
            }

            step(m_dt);
            sleepNs(1'000'000ull); // 1ms
        }
    }

    void updateWorkers()
    {
        const auto desiredType = m_cfg.threading.scheduler;
        const auto desiredN    = m_cfg.threading.workerCount;

        if (!m_worker) {
            m_worker = std::make_shared<EngineThreads>(desiredType, desiredN);
            return;
        }

        const bool typeChanged = (m_worker->schedulerType != desiredType);
        const bool nChanged    = (m_worker->threadCount != (desiredN == 0 ? m_worker->threadCount : desiredN));

        if (typeChanged || nChanged)
            m_worker->update(desiredType, desiredN);
    }

    void updateIntegrator()
    {
        if (!m_integrator) {
            m_integrator = integrators::create(m_cfg);
            m_integratorPrimed = false;
            return;
        }

        // Check if the type changed OR if we need a deep check on params
        if (m_cfg.integrator != m_integrator->type()) {
            auto next_integrator = integrators::create(m_cfg);
            m_integrator.swap(next_integrator);
            m_integratorPrimed = false;
        }
    }
    void updateFrameIo()
    {
        m_writer.reset();
        m_reader.reset();

        switch (m_cfg.io.sync) {
        case SyncType::None: {
            // No frame IO.
            return;
        }

        case SyncType::IO: {
            switch (m_cfg.io.ioType) {
            case IoType::File: {
                std::filesystem::path outPath;

                if (m_cfg.io.filepath.empty()) {
                    outPath = baseDir() / "frames.job";
                } else {
                    outPath = m_cfg.io.filepath;
                    if (std::filesystem::exists(outPath) && std::filesystem::is_directory(outPath)) {
                        outPath /= "frames.job";
                    } else if (!outPath.has_extension()) {
                        // treat as directory path
                        std::filesystem::create_directories(outPath);
                        outPath /= "frames.job";
                    }
                }

                // Ensure parent exists.
                if (outPath.has_parent_path())
                    std::filesystem::create_directories(outPath.parent_path());

                // Writer (append-many-frames to one file)
                {
                    auto ioWrite = std::make_shared<job::io::FileIO>(
                        outPath,
                        job::io::FileMode::RegularFile,
                        true // write mode
                        );

                    if (!ioWrite->openDevice()) {
                        JOB_LOG_ERROR("[Engine] Failed to open frame output file: {}", outPath.string());
                        return;
                    }

                    auto sink = std::make_shared<FrameSinkIO>(ioWrite);
                    m_writer  = std::make_shared<FrameSerializer>(sink);
                }

                {
                    auto ioRead = std::make_shared<job::io::FileIO>(
                        outPath,
                        job::io::FileMode::RegularFile,
                        false // read mode
                        );

                    auto source = std::make_shared<FrameSourceIO>(ioRead, true /*autoOpen*/);
                    m_reader    = std::make_shared<FrameDeserializer>(source);
                }

                JOB_LOG_INFO("[Engine] Frame IO enabled (File): {}", outPath.string());
                return;
            }

            case IoType::Uart:
            case IoType::Memory:
            default:
                JOB_LOG_WARN("[Engine] IO sync selected, but ioType not implemented yet");
                return;
            }
        }

        case SyncType::Socket: {
            JOB_LOG_WARN("[Engine] Socket sync selected, not wired yet (endpoint={})", m_cfg.io.endpoint);
            return;
        }

        default:
            return;
        }
    }



    // Worker and local threads
    EngineThreads::Ptr      m_worker;
    JobThread::Ptr          m_driver;

    // Config
    EngineConfig            m_cfg;
    EngineConfig            m_lastCfg = engine::DefaultEngineConfig();

    // Services
    FrameSerializer::Ptr     m_writer;
    FrameDeserializer::Ptr   m_reader;

    // The Intergrator/Adapter
    std::unique_ptr<integrators::IIntegrator>    m_integrator;


    // Data
    data::Particles         m_particles;
    std::vector<Vec3f>      m_prev_accel;
    DiskModel               m_disk;
    Zones                   m_zones;

    // State
    std::atomic<bool>       m_running{false};
    std::atomic<bool>       m_paused{false};
    float                   m_dt{1.0f}; // Timestep
    std::atomic<std::uint64_t> m_frameCounter{0};
    bool                       m_integratorPrimed{false};
    std::uint64_t m_stepIndex{0};


    // Calback
    ParticleCallback        m_particleCallback;
    EngineCallback          m_diskCallback;
    EngineCallback          m_zonesCallback;


    void initialize_particles(Particles &particles, const DiskModel &disk, const Zones &zones)
    {
        size_t count = particles.capacity();
        particles.resize(count);

        std::mt19937 rng{1337};

        float innerR = disk.innerRadius;
        float outerR = disk.outerRadius;

        std::uniform_real_distribution<float> radiusLog(std::log(innerR), std::log(outerR));
        std::uniform_real_distribution<float> angle(0.0f, 2.0f * job::core::piToTheFace);
        std::uniform_real_distribution<float> grainRadius(1e-6f, 1e-3f);

        for (uint64_t i = 0; i < count; i++) {
            auto &p = particles[i];
            float r_au = std::exp(radiusLog(rng));
            float theta = angle(rng);
            float radius = grainRadius(rng);

            float r_m = DiskModelUtil::auToMeters(r_au);
            float omega = DiskModelUtil::keplerianOmega(disk, r_au);
            float v_kep = omega * r_m; // Keplerian orbital speed

            p.id = i;
            p.radius = radius;
            p.composition = SciencePresets::rocky();
            p.mass = ParticleUtil::volume(p) * p.composition.density;
            p.temperature = DiskModelUtil::temperature(disk, r_au);

            p.position.x = r_m * std::cos(theta);
            p.position.y = r_m * std::sin(theta);
            p.position.z = 0.0f;

            p.velocity.x = -v_kep * std::sin(theta);
            p.velocity.y = v_kep * std::cos(theta);
            p.velocity.z = 0.0f;

            p.zone = ZonesUtil::classify(zones, p, disk);
        }
    }


};

}
