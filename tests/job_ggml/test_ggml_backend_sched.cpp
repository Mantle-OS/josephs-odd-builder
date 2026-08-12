#include <array>
#include <cstddef>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <ggml.h>

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_cgraph.h>
#include <job_ggml_context.h>
#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>
#include <job_ggml_init_params.h>
#include <job_ggml_tensor.h>

#include "test_ggml_utils.h"

using Catch::Approx;

// ============================================================================
// Block one: usage / examples
// ============================================================================

TEST_CASE("Backend scheduler computes a CPU tensor addition graph", "[ggml][backend_sched][usage][forward]")
{
    CpuSchedulerFixture fixture{};

    REQUIRE(fixture.cpu() != nullptr);
    REQUIRE(fixture.cpu()->isValid());
    REQUIRE(fixture.backend() != nullptr);
    REQUIRE(fixture.backend()->isValid());
    REQUIRE(fixture.backend()->isCpu());
    REQUIRE(fixture.scheduler() != nullptr);
    REQUIRE(fixture.scheduler()->isValid());
    REQUIRE(fixture.scheduler()->backendCount() == 1);

    /*
     * The scheduler owns graph allocation, so the GGML context only stores
     * tensor and graph metadata.
     */
    JobGgmlInitParams initParams{JobGgmlInitParams::estCtxCost(3)};
    auto context = JobGgmlContext::createUniq(initParams);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());
    REQUIRE(context->noAlloc());

    constexpr std::int64_t elementCount = 4;
    auto left  = context->newTensor1d(JobGgmlType::F32,  static_cast<std::int64_t>(elementCount));
    auto right = context->newTensor1d(JobGgmlType::F32, static_cast<std::int64_t>(elementCount));
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->data() != nullptr);
    REQUIRE(right->data() != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());
    auto result = op->add(*right);
    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->data() != nullptr);

    auto graph = context->newGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    graph->buildForwardExpand(*result);
    REQUIRE(graph->nodeCount() >= 1);

    /*
     * Explicitly assign all three tensors to the CPU backend. This exercises
     * both setTensorBackend() and tensorBackend().
     */
    fixture.scheduler()->setTensorBackend(*left, *fixture.backend());
    fixture.scheduler()->setTensorBackend(*right, *fixture.backend());
    fixture.scheduler()->setTensorBackend(*result, *fixture.backend());
    REQUIRE(fixture.scheduler()->tensorBackend(*left) == fixture.backend());
    REQUIRE(fixture.scheduler()->tensorBackend(*right) == fixture.backend());
    REQUIRE(fixture.scheduler()->tensorBackend(*result) == fixture.backend());

    /*
     * splitGraph() decides backend placement and prepares the scheduler's
     * internal split representation. allocateGraph() then creates the actual
     * backend storage for the tensors.
     */
    fixture.scheduler()->splitGraph(*graph);
    REQUIRE(fixture.scheduler()->allocateGraph(*graph));
    const std::array<float, elementCount> leftValues{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, elementCount> rightValues{ 10.0f, 20.0f, 30.0f, 40.0f };

    std::array<float, elementCount> resultValues{};

    const std::size_t byteCount = leftValues.size() * sizeof(float);
    fixture.backend()->setTensorAsync(*left, leftValues.data(), 0, byteCount);
    fixture.backend()->setTensorAsync(*right, rightValues.data(), 0, byteCount);

    /*
     * Ensure the input uploads are complete before graph execution.
     */
    fixture.backend()->synchronize();

    const JobGgmlStatus status = fixture.scheduler()->computeGraph(*graph);

    REQUIRE(status == JobGgmlStatus::Success);

    fixture.scheduler()->synchronize();
    fixture.backend()->getTensorAsync(*result, resultValues.data(), 0, byteCount);

    fixture.backend()->synchronize();

    REQUIRE(resultValues[0] == Approx(11.0f));
    REQUIRE(resultValues[1] == Approx(22.0f));
    REQUIRE(resultValues[2] == Approx(33.0f));
    REQUIRE(resultValues[3] == Approx(44.0f));
}



// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Backend scheduler preserves explicit tensor backend assignments", "[ggml][backend_sched][edge][assignment]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    REQUIRE(fixture.scheduler()->tensorBackend(*computation.left()) == fixture.backend());
    REQUIRE(fixture.scheduler()->tensorBackend(*computation.right()) == fixture.backend());
    REQUIRE(fixture.scheduler()->tensorBackend(*computation.result()) == fixture.backend());

    fixture.scheduler()->reset();
    REQUIRE(fixture.scheduler()->isValid());

    /*
     * Reset clears scheduler allocation and split state. Explicit backend
     * assignment can be established again before rebuilding the graph.
     */
    fixture.scheduler()->setTensorBackend(*computation.left(), *fixture.backend());

    REQUIRE(fixture.scheduler()->tensorBackend(*computation.left()) == fixture.backend());
}

TEST_CASE("Backend scheduler executes the same allocated graph repeatedly", "[ggml][backend_sched][edge][repeat]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> firstLeft{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> firstRight{ 10.0f, 20.0f, 30.0f, 40.0f };

    computation.uploadInputs(firstLeft, firstRight);

    REQUIRE(fixture.scheduler()->computeGraph(*computation.graph()) == JobGgmlStatus::Success);
    fixture.scheduler()->synchronize();

    const auto firstResult = computation.downloadResult();

    REQUIRE(firstResult[0] == Approx(11.0f));
    REQUIRE(firstResult[1] == Approx(22.0f));
    REQUIRE(firstResult[2] == Approx(33.0f));
    REQUIRE(firstResult[3] == Approx(44.0f));

    const std::array<float, CpuAdditionGraph::ElementCount> secondLeft{ -1.0f, -2.0f, -3.0f, -4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> secondRight{ 0.5f, 1.5f, 2.5f, 3.5f };

    computation.uploadInputs(secondLeft, secondRight);

    REQUIRE(fixture.scheduler()->computeGraph(*computation.graph()) == JobGgmlStatus::Success);
    fixture.scheduler()->synchronize();

    const auto secondResult = computation.downloadResult();

    REQUIRE(secondResult[0] == Approx(-0.5f));
    REQUIRE(secondResult[1] == Approx(-0.5f));
    REQUIRE(secondResult[2] == Approx(-0.5f));
    REQUIRE(secondResult[3] == Approx(-0.5f));
}

TEST_CASE("Backend scheduler computes a graph asynchronously", "[ggml][backend_sched][edge][async]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 3.0f, 6.0f, 9.0f, 12.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 1.0f, 2.0f, 3.0f, 4.0f };

    computation.uploadInputs(leftValues, rightValues);
    const JobGgmlStatus status = fixture.scheduler()->computeGraphAsync(*computation.graph());

    REQUIRE(status == JobGgmlStatus::Success);
    /*
     * Async computation is not complete until the scheduler is explicitly
     * synchronized.
     */
    fixture.scheduler()->synchronize();

    const auto values = computation.downloadResult();

    REQUIRE(values[0] == Approx(4.0f));
    REQUIRE(values[1] == Approx(8.0f));
    REQUIRE(values[2] == Approx(12.0f));
    REQUIRE(values[3] == Approx(16.0f));
}

TEST_CASE("Backend scheduler evaluation callback observes graph nodes", "[ggml][backend_sched][edge][callback]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 1.0f, 1.0f, 1.0f, 1.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 2.0f, 2.0f, 2.0f, 2.0f };

    computation.uploadInputs(leftValues, rightValues);

    std::size_t askCount = 0;
    std::size_t observationCount = 0;

    fixture.scheduler()->setEvalCallback([&askCount, &observationCount](JobGgmlTensor &tensor, bool ask) {
        if (!tensor.isValid())
            return false;

        if (ask) {
            ++askCount;
            // Ask to observe every graph node.
            return true;
        }

        ++observationCount;
        // Continue graph execution after observing the tensor.
        return true;
    });

    REQUIRE( fixture.scheduler()->computeGraph(*computation.graph() ) == JobGgmlStatus::Success);

    fixture.scheduler()->synchronize();

    REQUIRE(askCount >= 1);
    REQUIRE(observationCount >= 1);

    fixture.scheduler()->clearEvalCallback();

    const auto values = computation.downloadResult();

    REQUIRE(values[0] == Approx(3.0f));
    REQUIRE(values[1] == Approx(3.0f));
    REQUIRE(values[2] == Approx(3.0f));
    REQUIRE(values[3] == Approx(3.0f));
}

TEST_CASE("Backend scheduler callback may decline node observation", "[ggml][backend_sched][edge][callback]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 4.0f, 3.0f, 2.0f, 1.0f };

    computation.uploadInputs(leftValues, rightValues);

    std::size_t askCount = 0;
    std::size_t observationCount = 0;

    fixture.scheduler()->setEvalCallback([&askCount, &observationCount]( JobGgmlTensor &tensor, bool ask) {
        if (!tensor.isValid())
            return false;

        if (ask) {
            ++askCount;
            // Do not request observation. This does not cancel compute it only permits the scheduler to batch the node normally.
            return false;
        }

        ++observationCount;
        return true;
    }
                                         );

    REQUIRE(fixture.scheduler()->computeGraph( *computation.graph() ) == JobGgmlStatus::Success);

    fixture.scheduler()->synchronize();
    fixture.scheduler()->clearEvalCallback();

    REQUIRE(askCount >= 1);
    REQUIRE(observationCount == 0);

    const auto values = computation.downloadResult();

    REQUIRE(values[0] == Approx(5.0f));
    REQUIRE(values[1] == Approx(5.0f));
    REQUIRE(values[2] == Approx(5.0f));
    REQUIRE(values[3] == Approx(5.0f));
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Backend scheduler CPU graph compute performance", "[ggml][backend_sched][benchmark][compute]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 10.0f, 20.0f, 30.0f, 40.0f };
    computation.uploadInputs(leftValues, rightValues);

    BENCHMARK("compute allocated CPU addition graph") {
        const JobGgmlStatus status = fixture.scheduler()->computeGraph(*computation.graph());
        fixture.scheduler()->synchronize();
        return status;
    };
}

TEST_CASE("Backend scheduler asynchronous CPU graph performance", "[ggml][backend_sched][benchmark][async]")
{
    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 4.0f, 3.0f, 2.0f, 1.0f };
    computation.uploadInputs(leftValues, rightValues);

    BENCHMARK("submit and synchronize CPU addition graph") {
        const JobGgmlStatus status = fixture.scheduler()->computeGraphAsync(*computation.graph());
        fixture.scheduler()->synchronize();
        return status;
    };
}

TEST_CASE("Backend scheduler repeatedly executes an allocated graph", "[ggml][backend_sched][stress]")
{
    constexpr std::size_t iterationCount = 1000;

    CpuSchedulerFixture fixture;
    CpuAdditionGraph computation{fixture};

    const std::array<float, CpuAdditionGraph::ElementCount> leftValues{ 1.0f, 2.0f, 3.0f, 4.0f };
    const std::array<float, CpuAdditionGraph::ElementCount> rightValues{ 10.0f, 20.0f, 30.0f, 40.0f };
    for (std::size_t i = 0; i < iterationCount; ++i) {
        // NOTE: Scheduler allocation may reuse input storage for intermediate or output tensors. Restore graph inputs before each execution.
        computation.uploadInputs(leftValues, rightValues);
        REQUIRE(fixture.scheduler()->computeGraph( *computation.graph() ) == JobGgmlStatus::Success);
    }

    fixture.scheduler()->synchronize();

    const auto values = computation.downloadResult();

    REQUIRE(values[0] == Approx(11.0f));
    REQUIRE(values[1] == Approx(22.0f));
    REQUIRE(values[2] == Approx(33.0f));
    REQUIRE(values[3] == Approx(44.0f));
}

#endif