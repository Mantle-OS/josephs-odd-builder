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
    auto source  = std::make_shared<FrameSourceIo>(io_read, true); // autoOpen

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
    auto source  = std::make_shared<FrameSourceIo>(io_read, true); // autoOpen

    REQUIRE(source->isReady()); // it should open the file

    FrameDeserializer reader(source);

    DiskModel d{};
    Zones z{};
    Particles p{};

    REQUIRE_FALSE(reader.readFrame(d, z, p)); // invalid header is expected

    std::filesystem::remove(temp_file);
}
