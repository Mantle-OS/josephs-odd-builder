#include "engine.h"
#include <random>
#include <future>
#include <cmath>
#include <utility>

#ifdef ENABLE_LOGGING
#include <job_logger.h>
#endif
#include <job_thread_pool.h>

#include <data/forces.h>

// WARNING BAD CODE
// Put togeather real quick for juat a demo and to get stuff on the screen
namespace job::science::engine {
using namespace job::science::data;
using namespace job::threads;

void initialize_particles(Particles &particles, const DiskModel &disk, const Zones &zones)
{
    size_t count = particles.capacity();
    particles.resize(count);

    std::mt19937 rng{1337};

    real_t innerR = disk.innerRadius;
    real_t outerR = disk.outerRadius;

    std::uniform_real_distribution<real_t> radiusLog(std::log(innerR), std::log(outerR));
    std::uniform_real_distribution<real_t> angle(0.0f, 2.0f * static_cast<real_t>(piToTheFace));
    std::uniform_real_distribution<real_t> grainRadius(1e-6f, 1e-3f);

    for (uint64_t i = 0; i < count; i++) {
        auto &p = particles[i];
        real_t r_au = std::exp(radiusLog(rng));
        real_t theta = angle(rng);
        real_t radius = grainRadius(rng);

        real_t r_m = DiskModelUtil::auToMeters(r_au);
        real_t omega = DiskModelUtil::keplerianOmega(disk, r_au);
        real_t v_kep = omega * r_m; // Keplerian orbital speed

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
void Engine::runLoop()
{
    while (m_running.load()) {
        if (m_paused.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        step(m_dt);
    }
}

Engine::Engine(const DiskModel &diskModel,
               const Zones &zones,
               size_t particleCount) :
    m_pool(ThreadPool::create()),
    m_prev_accel(particleCount, Vec3f{}),
    m_disk(diskModel),
    m_zones(zones)
{

    m_particles.reserve(particleCount);
    initialize_particles(m_particles, m_disk, m_zones);
    m_writer = std::make_shared<FrameSerializer>(m_pool);
    m_reader = std::make_shared<FrameDeserializer>();
}

Engine::~Engine()
{
    stop();
}


void Engine::start() noexcept
{
    if (m_running.exchange(true))
        return;

#ifdef ENABLE_LOGGING
    JOB_LOG_INFO("[Engine] Starting simulation loop..");
#endif

    m_pool->submit([this]() {
        runLoop();
    });
}

void Engine::stop() noexcept
{
    if (!m_running.exchange(false))
        return;

#ifdef ENABLE_LOGGING
    JOB_LOG_INFO("[Engine] Stopping simulation...");
#endif

    if (m_pool)
        m_pool->shutdown(); // Gracefully shut down the pool
}

void Engine::kill() noexcept
{
    m_running.store(false);
}

bool Engine::running() const noexcept
{
    return m_running.load();
}


bool Engine::pause() noexcept
{
    if (m_running.load()) {
        m_paused.store(true);
        return true;
    }
    return false;
}

bool Engine::play() noexcept
{
    if (m_running.load()) {
        m_paused.store(false);
        return true;
    }
    return false;
}

bool Engine::ffwd(real_t multiplier) noexcept
{
    if (multiplier > static_cast<real_t>(0.0f)) {
        // Here, we would update the m_dt for the simulation.
        // For now, we will leave the timestep constant and update the UI pacing externally.
        // If we wanted to adjust simulation speed, we would adjust m_dt.
        return true;
    }
    return false;
}

bool Engine::rwd([[maybe_unused]] real_t multiplier) noexcept
{
    // HACK: Reversing simulation time is complex and requires full state history.for now, this is a stub.
    return false;
}


std::shared_ptr<FrameSerializer> Engine::writer() const noexcept
{
    return m_writer;
}

std::shared_ptr<FrameDeserializer> Engine::reader() const noexcept
{
    return m_reader;
}

std::filesystem::path Engine::baseDir() const
{
    return std::filesystem::temp_directory_path() / "pps_output";
}

void Engine::reset()
{
    m_particles.clear();
    m_prev_accel.clear();
    initialize_particles(m_particles, m_disk, m_zones);
}

const Particles &Engine::particles() const noexcept
{
    return m_particles;
}

const DiskModel &Engine::disk() const noexcept
{
    return m_disk;
}

const Zones &Engine::zones() const noexcept
{
    return m_zones;
}

void Engine::setParticleCallback(ParticleCallback cb)
{
    m_particleCallback = std::move(cb);
}

void Engine::setDiskCallback(EngineCallback cb)
{
    m_diskCallback = std::move(cb);
}

void Engine::setZonesCallback(EngineCallback cb)
{
    m_zonesCallback = std::move(cb);
}

real_t Engine::timeStep() const noexcept
{
    return m_dt;
}

void Engine::setTimeStep(real_t dt_s) noexcept
{
    m_dt = dt_s;
}

void Engine::calculateForces(Particle &p)
{
    p.acceleration = Forces::netAcceleration(p, m_disk, p.composition);
}


void Engine::step(real_t dt_s)
{
    std::vector<std::future<void>> futures;
    futures.reserve(m_particles.size());

    for (size_t i = 0; i < m_particles.size(); ++i) {
        Particle &p = m_particles[i];

        futures.push_back(m_pool->submit(
            0,
            [&p, this, i, dt_s]() {
                p.velocity = p.velocity + m_prev_accel[i] * (dt_s * static_cast<real_t>(0.5f));
                calculateForces(p);
            }
            ));
    }

    for (auto& future : futures)
        future.get();


    for (size_t i = 0; i < m_particles.size(); ++i) {
        Particle &p = m_particles[i];

        p.position = p.position + p.velocity * dt_s;
        p.velocity = p.velocity + p.acceleration * (dt_s * static_cast<real_t>(0.5f));
        m_prev_accel[i] = p.acceleration;
        real_t r_au = DiskModelUtil::metersToAU(p.position.length());
        p.temperature = DiskModelUtil::temperature(m_disk, r_au);
        p.zone = ZonesUtil::classify(m_zones, p, m_disk);
    }
    if (m_particleCallback)
        m_particleCallback(m_particles);
}


// void Engine::integrateParticle(Particle &p, real_t dt_s)
// {
//     // This function is kept as a stub since all integration is now performed in step()
//     // due to the index-based Velocity Verlet requirement.
// }


}
