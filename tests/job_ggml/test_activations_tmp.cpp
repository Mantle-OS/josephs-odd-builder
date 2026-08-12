#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>
#include <string>
#include <vector>

#include <catch2/benchmark/catch_benchmark.hpp>

#include <job_ggml_abort_callback.h>
#include <job_ggml_cpu.h>
#include <job_ggml_threadpool.h>
#include <job_ggml_threadpool_params.h>

#include <activation.h>
#include <activation_types.h>

#include "test_ggml_utils.h"

using namespace job::ai::comp;



TEST_CASE(
    "JOB AI Swish versus GGML SiLU CPU execution",
    "[ggml][cpu][activation][benchmark][job_ai]"
    )
{
    constexpr std::size_t N = 1'000'000;
    constexpr float Alpha = 1.0f;

    //
    // Generate the exact same deterministic input for every implementation.
    //

    std::vector<float> source(N);

    std::mt19937 gen(42);
    std::uniform_real_distribution<float> dist(-5.0f, 5.0f);

    for (float &value : source)
        value = dist(gen);

    //
    // JOB AI working buffers.
    //

    std::vector<float> jobScalarNormal = source;
    std::vector<float> jobScalarEstrin = source;
    std::vector<float> jobVectorNormal = source;
    std::vector<float> jobVectorEstrin = source;

    //
    // GGML.
    //
    // IMPORTANT:
    // CpuExecutionFixture needs to represent:
    //
    //      input -> SiLU
    //
    // and NOT the earlier:
    //
    //      input -> scale -> SiLU
    //
    // otherwise this is not the same operation.
    //

    CpuExecutionFixture ggmlFixture{
        static_cast<std::int64_t>(N)
    };

    REQUIRE(ggmlFixture.input != nullptr);
    REQUIRE(ggmlFixture.input->isValid());
    REQUIRE(ggmlFixture.backend != nullptr);
    REQUIRE(ggmlFixture.backend->isValid());

    //
    // Upload once. SiLU does not modify the input tensor.
    //

    ggml_backend_tensor_set(
        ggmlFixture.input->tensor(),
        source.data(),
        0,
        source.size() * sizeof(float)
        );

    //
    // Warm everything once before Catch starts measuring it.
    //

    activateBufferNaive<false>(
        jobScalarNormal.data(),
        jobScalarNormal.size(),
        ActivationType::Swish,
        Alpha
        );

    jobScalarNormal = source;

    activateBufferNaive<true>(
        jobScalarEstrin.data(),
        jobScalarEstrin.size(),
        ActivationType::Swish,
        Alpha
        );

    jobScalarEstrin = source;

    activate<false>(
        jobVectorNormal.data(),
        jobVectorNormal.size(),
        ActivationType::Swish,
        Alpha
        );

    jobVectorNormal = source;

    activate<true>(
        jobVectorEstrin.data(),
        jobVectorEstrin.size(),
        ActivationType::Swish,
        Alpha
        );

    jobVectorEstrin = source;

    REQUIRE(
        ggmlFixture.compute() ==
        job::ggml::JobGgmlStatus::Success
        );

    // ------------------------------------------------------------------------
    // Copy baseline.
    //
    // JOB activation is in-place, so preserving identical input between Catch
    // iterations requires restoring the buffer. Measure that cost explicitly
    // instead of pretending it does not exist.
    // ------------------------------------------------------------------------

    BENCHMARK("std::copy baseline N=1M")
    {
        std::copy(
            source.begin(),
            source.end(),
            jobVectorNormal.begin()
            );

        return jobVectorNormal[0];
    };

    // ------------------------------------------------------------------------
    // JOB AI scalar/reference path.
    // ------------------------------------------------------------------------

    BENCHMARK("JOB AI Swish scalar")
    {
        std::copy(
            source.begin(),
            source.end(),
            jobScalarNormal.begin()
            );

        activateBufferNaive<false>(
            jobScalarNormal.data(),
            jobScalarNormal.size(),
            ActivationType::Swish,
            Alpha
            );

        return jobScalarNormal[0];
    };

    BENCHMARK("JOB AI Swish scalar Estrin")
    {
        std::copy(
            source.begin(),
            source.end(),
            jobScalarEstrin.begin()
            );

        activateBufferNaive<true>(
            jobScalarEstrin.data(),
            jobScalarEstrin.size(),
            ActivationType::Swish,
            Alpha
            );

        return jobScalarEstrin[0];
    };

    // ------------------------------------------------------------------------
    // JOB AI SIMD buffer path.
    // ------------------------------------------------------------------------

    BENCHMARK("JOB AI Swish SIMD")
    {
        std::copy(
            source.begin(),
            source.end(),
            jobVectorNormal.begin()
            );

        activate<false>(
            jobVectorNormal.data(),
            jobVectorNormal.size(),
            ActivationType::Swish,
            Alpha
            );

        return jobVectorNormal[0];
    };

    BENCHMARK("JOB AI Swish SIMD Estrin")
    {
        std::copy(
            source.begin(),
            source.end(),
            jobVectorEstrin.begin()
            );

        activate<true>(
            jobVectorEstrin.data(),
            jobVectorEstrin.size(),
            ActivationType::Swish,
            Alpha
            );

        return jobVectorEstrin[0];
    };

    // ------------------------------------------------------------------------
    // GGML graph execution.
    //
    // The graph/context/scheduler/allocation and tensor upload are deliberately
    // outside the timed body.
    // ------------------------------------------------------------------------

    BENCHMARK("GGML SiLU")
    {
        return ggmlFixture.compute();
    };
}

namespace {

struct MulMatBenchFixture
{
    job::ggml::JobGgmlDeviceManager    &manager;
    job::ggml::JobGgmlCpu              *cpu{nullptr};
    job::ggml::JobGgmlBackend::Ptr      backend;
    job::ggml::JobGgmlBackendSched::Ptr scheduler;

    job::ggml::JobGgmlContext::UPtr       context;
    job::ggml::JobGgmlTensor::UPtr        left;
    job::ggml::JobGgmlTensor::UPtr        right;
    job::ggml::JobGgmlTensorOpGraph::UPtr expression;
    job::ggml::JobGgmlCGraph::UPtr        graph;

    MulMatBenchFixture(
        std::int64_t m,
        std::int64_t k,
        std::int64_t n
        ) :
        manager{testDeviceManager()}
    {
        manager.scan();

        cpu = manager.cpuDevice();

        if (!cpu || !cpu->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture requires a valid JobGgmlCpu"
            };
        }

        backend = cpu->backend();

        if (!backend || !backend->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture requires a valid CPU backend"
            };
        }

        manager.resetScheduler();

        scheduler = manager.buildScheduler(
            job::ggml::JobGgmlDeviceManager::Devices{
                cpu
            }
            );

        if (!scheduler || !scheduler->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to create CPU scheduler"
            };
        }

        //
        // Metadata only. Tensor storage is allocated by the backend scheduler.
        //

        context = createMetadataContext(1024);

        if (!context || !context->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to create context"
            };
        }

        //
        // GGML mul_mat convention:
        //
        //     left  = [K, M]
        //     right = [K, N]
        //
        // logical result = M x N
        //

        left = context->newTensor2d(
            job::ggml::JobGgmlType::F32,
            k,
            m
            );

        right = context->newTensor2d(
            job::ggml::JobGgmlType::F32,
            k,
            n
            );

        if (!left || !left->isValid() ||
            !right || !right->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to create tensors"
            };
        }

        auto source =
            job::ggml::JobGgmlTensorOp::createUniq(
                left->tensor(),
                context.get()
                );

        if (!source || !source->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to create tensor operation"
            };
        }

        auto mulMat =
            source->mulMat(
                *right
                );

        if (!mulMat || !mulMat->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to create mul_mat operation"
            };
        }

        expression =
            job::ggml::JobGgmlTensorOpGraph::wrap(
                std::move(mulMat)
                );

        if (!expression || !expression->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to wrap expression"
            };
        }

        graph =
            expression->buildGraph();

        if (!graph || !graph->isValid()) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to build graph"
            };
        }

        scheduler->setTensorBackend(
            *left,
            *backend
            );

        scheduler->setTensorBackend(
            *right,
            *backend
            );

        scheduler->setTensorBackend(
            *expression,
            *backend
            );

        scheduler->splitGraph(
            *graph
            );

        if (!scheduler->allocateGraph(*graph)) {
            throw std::runtime_error{
                "MulMatBenchFixture failed to allocate graph"
            };
        }
    }

    void upload(
        const std::vector<float> &a,
        const std::vector<float> &b
        )
    {
        ggml_backend_tensor_set(
            left->tensor(),
            a.data(),
            0,
            a.size() * sizeof(float)
            );

        ggml_backend_tensor_set(
            right->tensor(),
            b.data(),
            0,
            b.size() * sizeof(float)
            );
    }

    [[nodiscard]] job::ggml::JobGgmlStatus compute()
    {
        return scheduler->computeGraph(
            *graph
            );
    }
};

} // namespace


TEST_CASE(
    "GGML CPU mul_mat 1024 cubed",
    "[ggml][cpu][mul_mat][benchmark]"
    )
{
    constexpr std::int64_t M = 1024;
    constexpr std::int64_t K = 1024;
    constexpr std::int64_t N = 1024;

    //
    // Same basic data shape as the JOB_AI SGEMM benchmark.
    //

    std::vector<float> a(
        static_cast<std::size_t>(M * K),
        0.01f
        );

    std::vector<float> b(
        static_cast<std::size_t>(K * N),
        0.01f
        );

    MulMatBenchFixture fixture{
        M,
        K,
        N
    };

    fixture.upload(
        a,
        b
        );

    //
    // Warm the graph before timing.
    //

    REQUIRE(
        fixture.compute() ==
        job::ggml::JobGgmlStatus::Success
        );

    //
    // JOB_AI reference from test_gemm.cpp:
    //
    // Raw Pointer SGEMM (1024^3)        ~34.7 ms
    // Matrix Object SGEMM (1024^3)      ~32.0 ms
    // Matrix Parallel(4) SGEMM          ~ 9.56 ms
    //

    fixture.cpu->setNThreads(1);

    REQUIRE(
        fixture.compute() ==
        job::ggml::JobGgmlStatus::Success
        );

    BENCHMARK("GGML mul_mat F32 1024^3 - 1 thread")
    {
        return fixture.compute();
    };

    //
    // Match the existing JOB_AI Parallel(4) comparison.
    //

    fixture.backend->synchronize();

    fixture.cpu->setNThreads(4);

    REQUIRE(
        fixture.compute() ==
        job::ggml::JobGgmlStatus::Success
        );

    BENCHMARK("GGML mul_mat F32 1024^3 - 4 threads")
    {
        return fixture.compute();
    };

    //
    // And let GGML use the whole machine as an extra data point.
    //

    fixture.backend->synchronize();

    const int recommendedThreads =
        job::ggml::JobGgmlThreadPoolParams::recommendedThreadCount();

    fixture.cpu->setNThreads(
        recommendedThreads
        );

    REQUIRE(
        fixture.compute() ==
        job::ggml::JobGgmlStatus::Success
        );

    WARN(
        "GGML recommended thread count: "
        << recommendedThreads
        );

    BENCHMARK("GGML mul_mat F32 1024^3 - recommended threads")
    {
        return fixture.compute();
    };
}