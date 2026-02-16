#include "frame_sink_io.h"
#include "frame_source_io.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>

#include <real_type.h>

#include <file_io.h>

#include <ctx/job_stealing_ctx.h>

#include <frames/frame_serializer.h>
#include <frames/frame_deserializer.h>

#include <data/particle.h>
#include <data/disk.h>
#include <data/zones.h>
#include <data/composition.h>

using namespace job::science::data;
using namespace job::science::frames;
using namespace job::threads;

Particle create_particle(uint64_t id, float r_au, float radius, const Composition& comp)
{
    Particle p;
    const float r_m = r_au * DiskModelUtil::auToMeters(1.0f);
    const float r_m_comp = r_m / std::sqrt(2.0f);

    p.id = id;
    p.position.x = r_m_comp;
    p.position.y = r_m_comp;
    p.position.z = -r_m_comp;

    p.velocity = {10.0f, -5.0f, 1.0f};
    p.acceleration = {0.0f, 0.0f, 0.0f};
    p.radius = radius;
    p.temperature = 300.0f;
    p.composition = comp;

    p.mass = ParticleUtil::volume(p) * p.composition.density;
    p.zone = DiskZone::Mid_Disk;

    return p;
}

void check_particle_fidelity(const Particle& original, const Particle& loaded)
{
    REQUIRE(loaded.id == original.id);
    REQUIRE(loaded.zone == original.zone);

    REQUIRE(loaded.position.x == Catch::Approx(original.position.x));
    REQUIRE(loaded.velocity.y == Catch::Approx(original.velocity.y));
    REQUIRE(loaded.radius == Catch::Approx(original.radius));
    REQUIRE(loaded.mass == Catch::Approx(original.mass));

    REQUIRE(loaded.composition.density == Catch::Approx(original.composition.density));
    REQUIRE(loaded.composition.heatCapacity == Catch::Approx(original.composition.heatCapacity));
    REQUIRE(loaded.composition.type == original.composition.type);
    REQUIRE(loaded.composition.silicate == Catch::Approx(original.composition.silicate));
}



TEST_CASE("Frames E2E: Write/Read Fidelity of Full PDL State", "[frames][e2e]")
{
    const std::filesystem::path temp_file = "test_frame_out.job";

    DiskModel original_disk  = SciencePresets::tTauriDisk();
    Zones     original_zones = SciencePresets::solarSystemLike();

    std::vector<Particle> original_particles;
    original_particles.push_back(create_particle(100, 1.5f, 1e-4f, SciencePresets::rocky()));
    original_particles.push_back(create_particle(101, 5.0f, 1e-6f, SciencePresets::icy()));

    // WRITE
    auto io_write = std::make_shared<job::io::FileIO>(temp_file, job::io::FileMode::RegularFile, true);
    REQUIRE(io_write->openDevice()); // critical: serializer checks isReady() first

    auto sink = std::make_shared<FrameSinkIO>(io_write);
    FrameSerializer serializer(sink);

    REQUIRE(serializer.writeFrame(1, original_particles, original_disk, original_zones));

    serializer.flush();
    io_write->flush();
    io_write->closeDevice();

    // READ
    auto io_read = std::make_shared<job::io::FileIO>(temp_file, job::io::FileMode::RegularFile, false);
    auto source  = std::make_shared<FrameSourceIO>(io_read, true); // autoOpen

    FrameDeserializer reader(source);

    DiskModel loaded_disk{};
    Zones     loaded_zones{};
    Particles loaded_particles;

    REQUIRE(reader.readFrame(loaded_disk, loaded_zones, loaded_particles));

    SECTION("File Existence and Count Fidelity")
    {
        REQUIRE(std::filesystem::exists(temp_file));
        REQUIRE(loaded_particles.size() == original_particles.size());
    }

    SECTION("Configuration Block Fidelity (DiskModel and Zones)")
    {
        REQUIRE(std::memcmp(&original_disk,  &loaded_disk,  sizeof(DiskModel)) == 0);
        REQUIRE(std::memcmp(&original_zones, &loaded_zones, sizeof(Zones)) == 0);
    }

    SECTION("Particle Data Fidelity (Specific Member Checks)")
    {
        check_particle_fidelity(original_particles[0], loaded_particles[0]);
        check_particle_fidelity(original_particles[1], loaded_particles[1]);
    }

    std::filesystem::remove(temp_file);
}

TEST_CASE("Frames: Source autoOpen ready state", "[frames][source]")
{
    const std::filesystem::path temp_file = "test_frame_async.pps";

    std::ofstream out(temp_file, std::ios::binary);
    out.write("1234567890", 10);
    out.close();

    auto io_read = std::make_shared<job::io::FileIO>(temp_file, job::io::FileMode::RegularFile, false);
    auto source  = std::make_shared<FrameSourceIO>(io_read, true); // autoOpen

    REQUIRE(source->isReady()); // it should open the file

    FrameDeserializer reader(source);

    DiskModel d{};
    Zones z{};
    Particles p{};

    REQUIRE_FALSE(reader.readFrame(d, z, p)); // invalid header is expected

    std::filesystem::remove(temp_file);
}


#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#ifdef JOB_TEST_BENCHMARKS

namespace {

struct TempFileGuard final {
    std::filesystem::path path;

    explicit TempFileGuard(std::filesystem::path p) :
        path(std::move(p))
    {
    }

    ~TempFileGuard()
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
};

static void writeManyFramesToFile(const std::filesystem::path &path,
                                  std::size_t                  frameCount,
                                  const Particles             &particles,
                                  const DiskModel             &disk,
                                  const Zones                 &zones)
{
    auto io_write = std::make_shared<job::io::FileIO>(path, job::io::FileMode::RegularFile, true);

    // Current behavior: serializer checks sink->isReady() first.
    // FrameSinkIO::isReady() returns m_device->isOpen(), so open explicitly.
    REQUIRE(io_write->openDevice());

    auto sink = std::make_shared<FrameSinkIO>(io_write);
    FrameSerializer serializer(sink);

    for (std::size_t i = 0; i < frameCount; ++i) {
        const std::uint64_t frameId = static_cast<std::uint64_t>(i + 1);
        REQUIRE(serializer.writeFrame(frameId, particles, disk, zones));
    }

    serializer.flush();
    io_write->flush();
    io_write->closeDevice();
}

static std::size_t readAllFramesOnce(const std::filesystem::path &path,
                                     std::size_t                  frameCountExpected)
{
    auto io_read = std::make_shared<job::io::FileIO>(path, job::io::FileMode::RegularFile, false);
    auto source  = std::make_shared<FrameSourceIO>(io_read, true); // autoOpen
    FrameDeserializer reader(source);

    DiskModel disk{};
    Zones zones{};
    Particles particles{};

    std::size_t count = 0;
    while (count < frameCountExpected) {
        if (!reader.readFrame(disk, zones, particles))
            break;

        ++count;
    }

    return count;
}

static bool readSingleFrameOnce(const std::filesystem::path &path)
{
    auto io_read = std::make_shared<job::io::FileIO>(path, job::io::FileMode::RegularFile, false);
    auto source  = std::make_shared<FrameSourceIO>(io_read, true); // autoOpen
    FrameDeserializer reader(source);

    DiskModel disk{};
    Zones zones{};
    Particles particles{};

    return reader.readFrame(disk, zones, particles);
}

} // namespace

TEST_CASE("Frames: FrameDeserializer benchmarks (file)", "[frames][bench][deserializer]")
{
    // Keep sizes reasonable; benchmarks can be bumped locally.
    constexpr std::size_t kFramesSmall = 64;
    constexpr std::size_t kFramesBig   = 256;

    constexpr std::size_t kN1 = 1'024;
    constexpr std::size_t kN2 = 8'192;

    const DiskModel disk  = SciencePresets::tTauriDisk();
    const Zones     zones = SciencePresets::solarSystemLike();

    // Prepare payloads
    const Particles ps1 = ParticleUtil::genParticles(kN1);
    const Particles ps2 = ParticleUtil::genParticles(kN2);

    // Prepare temp files (unique enough for local runs)
    const auto file1 = std::filesystem::path("bench_frames_deser_n1.job");
    const auto file2 = std::filesystem::path("bench_frames_deser_n2.job");
    TempFileGuard cleanup1(file1);
    TempFileGuard cleanup2(file2);

    // Write test data once
    writeManyFramesToFile(file1, kFramesBig, ps1, disk, zones);
    writeManyFramesToFile(file2, kFramesSmall, ps2, disk, zones);

    // Warm up page cache + CPU paths (like your integrator warmup)
    for (int i = 0; i < 3; ++i) {
        REQUIRE(readSingleFrameOnce(file1));
        REQUIRE(readSingleFrameOnce(file2));
    }

    // ---- Throughput: read all frames in a file (dominant cost path)
    BENCHMARK("[FrameDeserializer] Read all frames (N=1024, frames=256)") {
        const auto got = readAllFramesOnce(file1, kFramesBig);
        // Keep a correctness check *outside* the timed portion? Catch benchmarks time the lambda.
        // But we do want to ensure we're not benchmarking a fast failure.
        return got;
    };

    BENCHMARK("[FrameDeserializer] Read all frames (N=8192, frames=64)") {
        const auto got = readAllFramesOnce(file2, kFramesSmall);
        return got;
    };

    // ---- Per-frame: read a single frame from start (includes open/autoOpen/first read)
    BENCHMARK("[FrameDeserializer] Read single frame (N=1024)") {
        const bool ok = readSingleFrameOnce(file1);
        return ok;
    };

    BENCHMARK("[FrameDeserializer] Read single frame (N=8192)") {
        const bool ok = readSingleFrameOnce(file2);
        return ok;
    };

    // Post-assertions (not benchmarked) to ensure the throughput benches are meaningful.
    REQUIRE(readAllFramesOnce(file1, kFramesBig) == kFramesBig);
    REQUIRE(readAllFramesOnce(file2, kFramesSmall) == kFramesSmall);
}

#endif // JOB_TEST_BENCHMARKS



