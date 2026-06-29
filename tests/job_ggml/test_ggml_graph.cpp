#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstring>

// core
#include <job_logger.h>
#include <real_type.h>

// ggml
#include <job_ggml_graph.h>
#include "test_ggml_utils.h"
using Catch::Approx;
using namespace job::ggml;

// ============================================================================
// Helpers
// ============================================================================

static JobGgmlDevice* requireCPU()
{
    auto m = JobGgmlDeviceManager();
    if (m.state() == ManagerState::Uninitialized)
        m.scan();


    auto *cpu = m.cpuDevice();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->type() == GGML_BACKEND_DEVICE_TYPE_CPU);
    return cpu;
}

// ============================================================================
// Block one: Usage / Examples
// ============================================================================

TEST_CASE("Simple tensor addition on CPU backend", "[ggml][graph][compute]")
{
    auto *cpu = requireCPU();

    JobGgmlGraph graph(1024 * 1024);

    auto *a = graph.tensor1d(4, "a");
    auto *b = graph.tensor1d(4, "b");

    float aData[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float bData[] = {10.0f, 20.0f, 30.0f, 40.0f};

    std::memcpy(ggml_get_data_f32(a), aData, sizeof(aData));
    std::memcpy(ggml_get_data_f32(b), bData, sizeof(bData));

    auto *result = ggml_add(graph.context(), a, b);
    graph.addForward(result);

    graph.compute(*cpu);

    const float *out = ggml_get_data_f32(result);
    REQUIRE(out[0] == Approx(11.0f));
    REQUIRE(out[1] == Approx(22.0f));
    REQUIRE(out[2] == Approx(33.0f));
    REQUIRE(out[3] == Approx(44.0f));
}

TEST_CASE("Backward pass with trainable parameters", "[ggml][graph][autodiff]")
{
    auto *cpu = requireCPU();

    // JobGgmlGraph graph(1024 * 1024);
    JobGgmlGraph graph(64 * 1024 * 1024);

    auto *x = graph.tensor1d(3, "x");
    auto *w = graph.tensor1d(3, "w");
    graph.markParam(w);

    float xData[] = {1.0f, 2.0f, 3.0f};
    float wData[] = {1.0f, 1.0f, 1.0f};

    std::memcpy(ggml_get_data_f32(x), xData, sizeof(xData));
    std::memcpy(ggml_get_data_f32(w), wData, sizeof(wData));

    auto *prod = ggml_mul(graph.context(), w, x);
    auto *loss = ggml_sum(graph.context(), prod);

    graph.addForward(loss);

    graph.addBackward();

    auto *lossGrad = ggml_graph_get_grad(graph.graph(), loss);
    REQUIRE(lossGrad != nullptr);

    float *lossGradData = ggml_get_data_f32(lossGrad);
    lossGradData[0] = 1.0f;

    graph.compute(*cpu);

    REQUIRE(ggml_get_data_f32(loss)[0] == Approx(6.0f));

    auto *wGradTensor = ggml_graph_get_grad(graph.graph(), w);
    REQUIRE(wGradTensor != nullptr);

    const float *wGrad = ggml_get_data_f32(wGradTensor);
    REQUIRE(wGrad[0] == Approx(1.0f));
    REQUIRE(wGrad[1] == Approx(2.0f));
    REQUIRE(wGrad[2] == Approx(3.0f));
}

TEST_CASE("Compute via scheduler", "[ggml][graph][sched]")
{
    auto manager = JobGgmlDeviceManager();
    manager.scan();

    REQUIRE(manager.state() == ManagerState::Ready);

    JobGgmlGraph graph(2 * 1024 * 1024);

    auto *A = graph.tensor2d(8, 8, "A");
    auto *B = graph.tensor2d(8, 8, "B");

    float aData[64] = {0};
    float bData[64] = {0};

    for (int i = 0; i < 64; ++i) {
        aData[i] = static_cast<float>(i) * 0.01f;
        bData[i] = static_cast<float>(63 - i) * 0.01f;
    }

    std::memcpy(ggml_get_data_f32(A), aData, sizeof(aData));
    std::memcpy(ggml_get_data_f32(B), bData, sizeof(bData));

    auto *C = ggml_mul_mat(graph.context(), A, B);
    graph.addForward(C);

    graph.computeWithSched(manager);

    const float *out = ggml_get_data_f32(C);

    float sum = 0.0f;
    for (int i = 0; i < 64; ++i) {
        REQUIRE(job::core::isSafeFinite(out[i]));
        sum += out[i];
    }

    REQUIRE(sum != Approx(0.0f));
}

// ============================================================================
// Block two: Edge cases
// ============================================================================

TEST_CASE("Graph reset clears nodes but context memory persists", "[ggml][graph][edge]")
{
    JobGgmlGraph graph(1024 * 1024);

    auto *t = graph.tensor1d(16, "t");
    auto *r = ggml_sum(graph.context(), t);
    graph.addForward(r);

    REQUIRE(ggml_graph_n_nodes(graph.graph()) > 0);

    size_t memBefore = graph.usedMem();
    REQUIRE(memBefore > 0);

    graph.reset();

    REQUIRE(ggml_graph_n_nodes(graph.graph()) == 0);
    REQUIRE(graph.usedMem() == memBefore);
}

TEST_CASE("usedMem returns the actual arena usage, not the total allocation", "[ggml][graph][edge]")
{
    constexpr size_t totalBytes = 2 * 1024 * 1024;
    JobGgmlGraph graph(totalBytes);

    size_t used = graph.usedMem();

    REQUIRE(used > 0);
    REQUIRE(used <= totalBytes);
}

TEST_CASE("Named tensors carry their names", "[ggml][graph][edge]")
{
    JobGgmlGraph graph(1024 * 1024);

    auto *t1d = graph.tensor1d(10, "my_1d_tensor");
    auto *t2d = graph.tensor2d(4, 5, "my_2d_tensor");
    auto *t3d = graph.tensor3d(2, 3, 4, "my_3d_tensor");
    auto *t4d = graph.tensor4d(2, 3, 4, 5, "my_4d_tensor");

    REQUIRE(t1d->name);
    REQUIRE(std::string(t1d->name) == "my_1d_tensor");
    REQUIRE(std::string(t2d->name) == "my_2d_tensor");
    REQUIRE(std::string(t3d->name) == "my_3d_tensor");
    REQUIRE(std::string(t4d->name) == "my_4d_tensor");
}

TEST_CASE("Tensor creation with empty name does not crash", "[ggml][graph][edge]")
{
    JobGgmlGraph graph(1024 * 1024);

    auto *t1d = graph.tensor1d(10);
    auto *t2d = graph.tensor2d(4, 5);
    auto *t3d = graph.tensor3d(2, 3, 4);
    auto *t4d = graph.tensor4d(2, 3, 4, 5);

    REQUIRE(t1d != nullptr);
    REQUIRE(t2d != nullptr);
    REQUIRE(t3d != nullptr);
    REQUIRE(t4d != nullptr);
}

// ============================================================================
// Block three: Benchmarks
// ============================================================================

#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("256x256 matrix multiply via ggml_backend_graph_compute", "[ggml][graph][benchmark]")
{
    auto *cpu = requireCPU();

    constexpr int N = 256;

    JobGgmlGraph graph(32 * 1024 * 1024);

    auto *A = graph.tensor2d(N, N, "A");
    auto *B = graph.tensor2d(N, N, "B");

    float *aData = ggml_get_data_f32(A);
    float *bData = ggml_get_data_f32(B);

    for (int i = 0; i < N * N; ++i) {
        aData[i] = static_cast<float>(i % 31) * 0.01f;
        bData[i] = static_cast<float>((i + 17) % 43) * 0.01f;
    }

    auto *C = ggml_mul_mat(graph.context(), A, B);
    graph.addForward(C);

    BENCHMARK("256x256 matmul (f32) on CPU backend") {

        JobGgmlGraph graph(64 * 1024 * 1024);

        auto *a = graph.tensor2d(N, N);
        auto *b = graph.tensor2d(N, N);

        std::memcpy(ggml_get_data_f32(a), aData, sizeof(float) * N * N);
        std::memcpy(ggml_get_data_f32(b), bData, sizeof(float) * N * N);

        auto *c = ggml_mul_mat(graph.context(), a, b);
        graph.addForward(c);

        graph.compute(*cpu);

        return ggml_get_data_f32(c)[0];
    };
}



#endif