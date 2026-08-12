#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ctx/job_stealing_ctx.h>

#include <data/forces.h>
#include <data/particle.h>
#include <data/vec3f.h>

#include <solvers/job_fmm_integrator.h>

#include <job_ggml_tensor_op.h>
#include <job_ggml_tensor_op_graph.h>

#include "test_ggml_utils.h"

using namespace job::ggml;
using namespace job::threads;
using namespace job::science::data;
using Catch::Approx;

namespace {

// ============================================================================
// FMM -> GGML bridge
// ============================================================================

struct GgmlFmmTraits
{
    static Vec3f position(const Particle &particle)
    {
        return particle.position;
    }

    static float mass(const Particle &particle)
    {
        return particle.mass;
    }

    static void applyForce(Particle &particle, const Vec3f &acceleration)
    {
        particle.acceleration = particle.acceleration + acceleration;
    }
};

using GgmlFmmEngine = job::science::JobFmmEngine<Particle, Vec3f, float, GgmlFmmTraits>;

struct GgmlFmmState
{
    JobStealerCtx threads{4};

    float theta{0.5f};
    int maxLeafSize{64};
    int maxDepth{16};

    std::atomic<std::uint64_t> callbackCount{0};
    std::atomic<void *> seenUserdata{nullptr};
};

void runFmmCustom3(JobGgmlTensor &dst,
                   const JobGgmlTensor &positions,
                   const JobGgmlTensor &masses,
                   const JobGgmlTensor &gains,
                   int ith,
                   int nth,
                   void *userdata)
{
    auto *state = static_cast<GgmlFmmState *>(userdata);
    if (!state)
        return;

    state->seenUserdata.store(userdata, std::memory_order_relaxed);
    state->callbackCount.fetch_add(1, std::memory_order_relaxed);

    // This test deliberately requests one GGML custom task.
    if (ith != 0 || nth != 1)
        return;

    auto *dstTensor      = dst.tensor();
    auto *positionTensor = positions.tensor();
    auto *massTensor     = masses.tensor();
    auto *gainTensor     = gains.tensor();

    if (!dstTensor ||
        !positionTensor ||
        !massTensor ||
        !gainTensor ||
        !dstTensor->data ||
        !positionTensor->data ||
        !massTensor->data ||
        !gainTensor->data)
        return;

    const std::int64_t dimensions = positionTensor->ne[0];
    const std::int64_t particles  = positionTensor->ne[1];
    if (dimensions < 3 || particles <= 0)
        return;

    const auto *positionData = static_cast<const float *>(positionTensor->data);
    const auto *massData     = static_cast<const float *>(massTensor->data);
    const auto *gainData     = static_cast<const float *>(gainTensor->data);
    auto *outputData         = static_cast<float *>(dstTensor->data);

    std::vector<Particle> bodies(static_cast<std::size_t>(particles));
    for (std::int64_t i = 0; i < particles; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i * dimensions);
        auto &body = bodies[static_cast<std::size_t>(i)];

        body.position = {
            positionData[offset + 0],
            positionData[offset + 1],
            positionData[offset + 2]
        };
        body.mass         = massData[offset];
        body.acceleration = {0.0f, 0.0f, 0.0f};
    }

    GgmlFmmEngine::Params params;
    params.theta       = state->theta;
    params.maxLeafSize = state->maxLeafSize;
    params.maxDepth    = state->maxDepth;

    GgmlFmmEngine engine(state->threads.pool, params);
    engine.compute(bodies);

    for (std::int64_t i = 0; i < particles; ++i) {
        const std::size_t offset = static_cast<std::size_t>(i * dimensions);
        const auto &body = bodies[static_cast<std::size_t>(i)];
        const float gain = gainData[offset];

        outputData[offset + 0] = body.acceleration.x * gain;
        outputData[offset + 1] = body.acceleration.y * gain;
        outputData[offset + 2] = body.acceleration.z * gain;

        for (std::int64_t d = 3; d < dimensions; ++d)
            outputData[offset + static_cast<std::size_t>(d)] = 0.0f;
    }
}

// ============================================================================
// Fixture
// ============================================================================

struct GgmlFmmFixture
{
    static constexpr std::int64_t Dimensions    = 4;
    static constexpr std::int64_t ParticleCount = 2;
    static constexpr std::size_t ElementCount   = static_cast<std::size_t>(Dimensions * ParticleCount);
    static constexpr std::size_t ByteCount      = ElementCount * sizeof(float);

    CpuSchedulerFixture schedulerFixture;

    JobGgmlContext::UPtr context;

    JobGgmlTensor::UPtr positions;
    JobGgmlTensor::UPtr masses;
    JobGgmlTensor::UPtr gains;

    JobGgmlTensorOpGraph::UPtr expression;
    JobGgmlCGraph::UPtr graph;

    GgmlFmmState state;

    GgmlFmmFixture()
    {
        context = JobGgmlContext::createUniqMetadata(256);
        if (!context || !context->isValid())
            throw std::runtime_error{"Failed to create GGML FMM context"};

        positions = context->newTensor2d(JobGgmlType::F32, Dimensions, ParticleCount);
        masses    = context->newTensor2d(JobGgmlType::F32, Dimensions, ParticleCount);
        gains     = context->newTensor2d(JobGgmlType::F32, Dimensions, ParticleCount);
        if (!positions || !masses || !gains)
            throw std::runtime_error{"Failed to create GGML FMM tensors"};

        auto positionOp = JobGgmlTensorOp::createUniq(positions->tensor(), context.get());
        if (!positionOp)
            throw std::runtime_error{"Failed to create GGML FMM operation root"};

        auto custom = positionOp->mapCustom3(*masses, *gains, runFmmCustom3, 1, &state);
        if (!custom || !custom->isValid())
            throw std::runtime_error{"Failed to create GGML FMM custom operation"};

        expression = JobGgmlTensorOpGraph::wrap(std::move(custom));
        if (!expression)
            throw std::runtime_error{"Failed to wrap GGML FMM expression"};

        graph = expression->buildGraph();
        if (!graph || !graph->isValid())
            throw std::runtime_error{"Failed to build GGML FMM graph"};

        schedulerFixture.scheduler()->setTensorBackend(*positions, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*masses, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*gains, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*expression, *schedulerFixture.backend());
        schedulerFixture.scheduler()->splitGraph(*graph);
        if (!schedulerFixture.scheduler()->allocateGraph(*graph))
            throw std::runtime_error{"Failed to allocate GGML FMM graph"};
    }

    void upload(const std::array<float, ElementCount> &positionValues,
                const std::array<float, ElementCount> &massValues,
                const std::array<float, ElementCount> &gainValues)
    {
        schedulerFixture.backend()->setTensorAsync(*positions, positionValues.data(), 0, ByteCount);
        schedulerFixture.backend()->setTensorAsync(*masses, massValues.data(), 0, ByteCount);
        schedulerFixture.backend()->setTensorAsync(*gains, gainValues.data(), 0, ByteCount);
        schedulerFixture.backend()->synchronize();
    }

    [[nodiscard]] std::array<float, ElementCount> download()
    {
        std::array<float, ElementCount> values{};
        schedulerFixture.backend()->getTensorAsync(*expression, values.data(), 0, ByteCount);
        schedulerFixture.backend()->synchronize();
        return values;
    }
};

#ifdef JOB_TEST_BENCHMARKS

struct DirectFmmBench
{
    JobStealerCtx threads{4};

    std::vector<Particle> bodies;
    GgmlFmmEngine::Params params;
    std::unique_ptr<GgmlFmmEngine> engine;

    explicit DirectFmmBench(std::int64_t particleCount)
    {
        bodies.resize(static_cast<std::size_t>(particleCount));

        std::mt19937 gen{42};
        std::uniform_real_distribution<float> dist{-50.0f, 50.0f};

        for (auto &body : bodies) {
            body.position = {
                dist(gen),
                dist(gen),
                dist(gen)
            };
            body.mass         = 1.0f;
            body.acceleration = {0.0f, 0.0f, 0.0f};
        }

        params.theta       = 0.5f;
        params.maxLeafSize = 128;
        params.maxDepth    = 16;

        engine = std::make_unique<GgmlFmmEngine>(threads.pool, params);
    }

    void resetAccelerations() noexcept
    {
        for (auto &body : bodies)
            body.acceleration = {0.0f, 0.0f, 0.0f};
    }

    void run()
    {
        resetAccelerations();
        engine->compute(bodies);
    }
};

struct GgmlFmmBench
{
    static constexpr std::int64_t Dimensions = 4;

    CpuSchedulerFixture schedulerFixture;

    JobGgmlContext::UPtr       context;
    JobGgmlTensor::UPtr        positions;
    JobGgmlTensor::UPtr        masses;
    JobGgmlTensor::UPtr        gains;
    JobGgmlTensorOpGraph::UPtr expression;
    JobGgmlCGraph::UPtr        graph;

    GgmlFmmState state;

    std::vector<float> positionData;
    std::vector<float> massData;
    std::vector<float> gainData;

    explicit GgmlFmmBench(std::int64_t particleCount)
    {
        context = JobGgmlContext::createUniqMetadata(256);
        if (!context || !context->isValid())
            throw std::runtime_error{"Failed to create persistent GGML FMM benchmark context"};

        positions = context->newTensor2d(JobGgmlType::F32, Dimensions, particleCount);
        masses    = context->newTensor2d(JobGgmlType::F32, Dimensions, particleCount);
        gains     = context->newTensor2d(JobGgmlType::F32, Dimensions, particleCount);
        if (!positions || !masses || !gains)
            throw std::runtime_error{"Failed to create persistent GGML FMM benchmark tensors"};

        state.theta       = 0.5f;
        state.maxLeafSize = 128;
        state.maxDepth    = 16;

        auto root = JobGgmlTensorOp::createUniq(positions->tensor(), context.get());
        if (!root)
            throw std::runtime_error{"Failed to create persistent GGML FMM benchmark root"};

        auto custom = root->mapCustom3(*masses, *gains, runFmmCustom3, 1, &state);
        if (!custom || !custom->isValid())
            throw std::runtime_error{"Failed to create persistent GGML FMM custom operation"};

        expression = JobGgmlTensorOpGraph::wrap(std::move(custom));
        if (!expression || !expression->isValid())
            throw std::runtime_error{"Failed to create persistent GGML FMM expression"};

        graph = expression->buildGraph();
        if (!graph || !graph->isValid())
            throw std::runtime_error{"Failed to build persistent GGML FMM graph"};

        schedulerFixture.scheduler()->setTensorBackend(*positions, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*masses, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*gains, *schedulerFixture.backend());
        schedulerFixture.scheduler()->setTensorBackend(*expression, *schedulerFixture.backend());
        schedulerFixture.scheduler()->splitGraph(*graph);
        if (!schedulerFixture.scheduler()->allocateGraph(*graph))
            throw std::runtime_error{"Failed to allocate persistent GGML FMM graph"};

        const std::size_t elementCount = static_cast<std::size_t>(Dimensions * particleCount);
        const std::size_t byteCount    = elementCount * sizeof(float);

        positionData.assign(elementCount, 0.0f);
        massData.assign(elementCount, 0.0f);
        gainData.assign(elementCount, 0.0f);

        std::mt19937 gen{42};
        std::uniform_real_distribution<float> dist{-50.0f, 50.0f};

        for (std::int64_t i = 0; i < particleCount; ++i) {
            const std::size_t offset = static_cast<std::size_t>(i * Dimensions);
            positionData[offset + 0] = dist(gen);
            positionData[offset + 1] = dist(gen);
            positionData[offset + 2] = dist(gen);
            massData[offset + 0] = 1.0f;
            gainData[offset + 0] = 1.0f;
        }

        schedulerFixture.backend()->setTensorAsync(*positions, positionData.data(), 0, byteCount);
        schedulerFixture.backend()->setTensorAsync(*masses, massData.data(), 0, byteCount);
        schedulerFixture.backend()->setTensorAsync(*gains, gainData.data(), 0, byteCount);
        schedulerFixture.backend()->synchronize();
    }

    [[nodiscard]] JobGgmlStatus run()
    {
        return schedulerFixture.scheduler()->computeGraph(*graph);
    }
};

#endif

} // namespace

TEST_CASE("GGML mapCustom3 executes the FMM science engine",
          "[ggml][tensor][op][science][fmm][custom3][usage]")
{
    GgmlFmmFixture fixture;

    // Particle A: (0,0,0)
    // Particle B: (2,0,0)
    //
    // The remaining embedding lane is intentionally unused.
    std::array<float, GgmlFmmFixture::ElementCount> positions{
        0.0f, 0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> masses{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> gains{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    fixture.upload(positions, masses, gains);
    REQUIRE(fixture.schedulerFixture.scheduler()->computeGraph(*fixture.graph) == JobGgmlStatus::Success);

    auto output = fixture.download();
    INFO("Particle A Fx: " << output[0]);
    INFO("Particle B Fx: " << output[4]);

    REQUIRE(output[0] > 0.0f);
    REQUIRE(output[4] < 0.0f);
    REQUIRE(output[0] == Approx(-output[4]).margin(1.0e-4f));
    REQUIRE(output[1] == Approx(0.0f).margin(1.0e-5f));
    REQUIRE(output[2] == Approx(0.0f).margin(1.0e-5f));
    REQUIRE(fixture.state.callbackCount.load(std::memory_order_relaxed) == 1);
    REQUIRE(fixture.state.seenUserdata.load(std::memory_order_relaxed) == &fixture.state);
}

TEST_CASE("GGML FMM custom operation applies per-particle output gain",
          "[ggml][tensor][op][science][fmm][custom3][gain]")
{
    GgmlFmmFixture fixture;

    std::array<float, GgmlFmmFixture::ElementCount> positions{
        0.0f, 0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> masses{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> gains{
        2.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.0f, 0.0f, 0.0f
    };

    fixture.upload(positions, masses, gains);
    REQUIRE(fixture.schedulerFixture.scheduler()->computeGraph(*fixture.graph) == JobGgmlStatus::Success);

    auto output = fixture.download();
    REQUIRE(output[0] > 0.0f);
    REQUIRE(output[4] < 0.0f);

    // Same physical acceleration, different output gains:
    // A gain = 2.0
    // B gain = 0.5
    //
    // Magnitude ratio should therefore be 4:1.
    REQUIRE(std::abs(output[0]) == Approx(std::abs(output[4]) * 4.0f).margin(1.0e-3f));
}

TEST_CASE("GGML context retains FMM callback payload after operation wrappers are destroyed",
          "[ggml][tensor][op][science][fmm][custom3][lifetime]")
{
    CpuSchedulerFixture schedulerFixture;

    auto context = JobGgmlContext::createUniqMetadata(256);
    REQUIRE(context != nullptr);

    auto positions = context->newTensor2d(JobGgmlType::F32, 4, 2);
    auto masses    = context->newTensor2d(JobGgmlType::F32, 4, 2);
    auto gains     = context->newTensor2d(JobGgmlType::F32, 4, 2);
    REQUIRE(positions != nullptr);
    REQUIRE(masses != nullptr);
    REQUIRE(gains != nullptr);

    GgmlFmmState state;

    auto root = JobGgmlTensorOp::createUniq(positions->tensor(), context.get());
    auto custom = root->mapCustom3(*masses, *gains, runFmmCustom3, 1, &state);
    REQUIRE(custom != nullptr);

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(custom));
    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);

    schedulerFixture.scheduler()->setTensorBackend(*positions, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*masses, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*gains, *schedulerFixture.backend());
    schedulerFixture.scheduler()->setTensorBackend(*expression, *schedulerFixture.backend());
    schedulerFixture.scheduler()->splitGraph(*graph);
    REQUIRE(schedulerFixture.scheduler()->allocateGraph(*graph));

    std::array<float, 8> positionValues{
        0.0f, 0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, 8> massValues{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, 8> gainValues{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    schedulerFixture.backend()->setTensorAsync(*positions, positionValues.data(), 0, sizeof(positionValues));
    schedulerFixture.backend()->setTensorAsync(*masses, massValues.data(), 0, sizeof(massValues));
    schedulerFixture.backend()->setTensorAsync(*gains, gainValues.data(), 0, sizeof(gainValues));
    schedulerFixture.backend()->synchronize();

    // Kill the construction wrapper.
    //
    // The native graph still exists and JobGgmlContext must keep the
    // callback payload alive.
    root.reset();

    REQUIRE(schedulerFixture.scheduler()->computeGraph(*graph) == JobGgmlStatus::Success);
    REQUIRE(state.callbackCount.load(std::memory_order_relaxed) == 1);
    REQUIRE(state.seenUserdata.load(std::memory_order_relaxed) == &state);
}

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("GGML custom FMM execution overhead benchmark",
          "[ggml][tensor][op][science][fmm][custom3][benchmark]")
{
    GgmlFmmFixture fixture;

    std::array<float, GgmlFmmFixture::ElementCount> positions{
        0.0f, 0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> masses{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    std::array<float, GgmlFmmFixture::ElementCount> gains{
        1.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f
    };

    fixture.upload(positions, masses, gains);

    BENCHMARK("execute FMM through GGML mapCustom3") {
        return fixture.schedulerFixture.scheduler()->computeGraph(*fixture.graph);
    };
}

TEST_CASE("GGML custom FMM scaling against direct science execution",
          "[ggml][tensor][op][science][fmm][custom3][benchmark][scaling]")
{
    DirectFmmBench direct512{512};
    DirectFmmBench direct1024{1024};
    DirectFmmBench direct4096{4096};
    DirectFmmBench direct16384{16384};
    DirectFmmBench direct32768{32768};
    DirectFmmBench direct65536{65536};

    GgmlFmmBench ggml512{512};
    GgmlFmmBench ggml1024{1024};
    GgmlFmmBench ggml4096{4096};
    GgmlFmmBench ggml16384{16384};
    GgmlFmmBench ggml32768{32768};
    GgmlFmmBench ggml65536{65536};

    BENCHMARK("Direct FMM N=1024") {
        direct1024.run();
    };

    BENCHMARK("GGML mapCustom3 FMM N=1024") {
        return ggml1024.run();
    };

    BENCHMARK("Direct FMM N=4096") {
        direct4096.run();
    };

    BENCHMARK("GGML mapCustom3 FMM N=4096") {
        return ggml4096.run();
    };

    BENCHMARK("Direct FMM N=16384") {
        direct16384.run();
    };

    BENCHMARK("GGML mapCustom3 FMM N=16384") {
        return ggml16384.run();
    };

    BENCHMARK("Direct FMM N=32768") {
        direct32768.run();
    };

    BENCHMARK("GGML mapCustom3 FMM N=32768") {
        return ggml32768.run();
    };
}

#endif