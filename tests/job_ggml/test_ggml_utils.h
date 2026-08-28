#pragma once

#include <cuda_runtime_api.h>
#include <stdexcept>
#include <vector>
#include <fstream>
#include <filesystem>

#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif
#include "job_ggml_context.h"

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>

#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>

#include <job_ggml_opt_params.h>
#include <job_ggml_opt_dataset.h>

#include <job_ggml_tensor_op.h>
#include <job_ggml_tensor_op_graph.h>

#include <job_gguf.h>
#include <job_gguf_kv.h>


#include <job_ggml.h>

#include "../transient_test_file.h"

using namespace job::ggml;
extern job::ggml::JobGgml *g_jobGgml; // Borrowed from main() stack.

// void testGgmlLogCallback(ggml_log_level level, const char *text, void *userData)
// {
//     (void)level;
//     (void)text;
//     (void)userData;
//     // SHUT UP

//     // switch (level) {
//     // case GGML_LOG_LEVEL_DEBUG:
//     // case GGML_LOG_LEVEL_NONE:
//     // case GGML_LOG_LEVEL_INFO:
//     // case GGML_LOG_LEVEL_WARN:
//     // case GGML_LOG_LEVEL_ERROR:
//     // case GGML_LOG_LEVEL_CONT:
//     //     return;
//     // }
// }

// // Super stupid
// static inline job::ggml::JobGgmlDeviceManager &testDeviceManager()
// {
//     if (!g_deviceManager)
//         throw std::runtime_error{ "The shared JobGgmlDeviceManager has not been initialized" };

//     return *g_deviceManager;
// }

// //// Manager TEST
// [[nodiscard]] inline job::ggml::JobGgmlDeviceManager &readyDeviceManager()
// {
//     job::ggml::JobGgmlDeviceManager &manager = testDeviceManager();
//     // manager.scan();
//     return manager;
// }
// //// END MANAGER


#ifndef JOB_CI_BUILD
inline JobGgmlVulkan *testVulkanDevice()
{
    auto *manager = g_jobGgml->deviceManager();
    REQUIRE(manager != nullptr);

    if (!manager->hasVulkan())
        return nullptr;

    const auto devices = manager->vulkanDevices();

    if (devices.isEmpty())
        return nullptr;

    return devices.at(0);
}
#endif

// really stupid ....
inline JobGgmlCuda *testCudaDevice(std::size_t idx = 0)
{
    auto *manager = g_jobGgml->deviceManager();
    if (!manager->hasCuda())
        return nullptr;

    const auto devices = manager->cudaDevices();
    if (devices.isEmpty())
        return nullptr;

    return devices.at(idx);
}

struct CudaDeviceAllocation
{
    int   device{-1};
    void *ptr{nullptr};

    CudaDeviceAllocation(int deviceIndex, std::size_t size) :
        device{deviceIndex}
    {
        if (cudaSetDevice(device) != cudaSuccess)
            return;

        if (cudaMalloc(&ptr, size) != cudaSuccess)
            ptr = nullptr;
    }

    ~CudaDeviceAllocation()
    {
        if (!ptr)
            return;

        cudaSetDevice(device);
        cudaFree(ptr);
    }

    CudaDeviceAllocation(const CudaDeviceAllocation &) = delete;
    CudaDeviceAllocation &operator=(const CudaDeviceAllocation &) = delete;

    [[nodiscard]] bool isValid() const noexcept
    {
        return ptr != nullptr;
    }
};



// THIS IS STUPID and has been replaced with JobGgmlInitParams::createMetadata and  JobGgmlInitParams::createMetadataFor
// [[nodiscard]] inline std::size_t testGgmlContextMetadataSize(std::size_t tensorCount, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool gradients = false) noexcept
// {
//     constexpr std::size_t metadataSafetyBytes = 4096;
//     return tensorCount * ggml_tensor_overhead() +
//            job::ggml::JobGgmlCGraph::overheadCustom(graphSize, gradients) +
//            metadataSafetyBytes;
// }
// // THIS IS STUPID and has been replaced with JobGgmlInitParams::createUniqMetadataFor
// [[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createHostContext(std::size_t tensorCount, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool gradients = false)
// {
//     // you again
//     job::ggml::JobGgmlInitParams initParams{
//         testGgmlContextMetadataSize(tensorCount, graphSize, gradients),
//         nullptr,
//         false
//     };

//     return job::ggml::JobGgmlContext::createUniq(initParams);
// }

// // THIS IS STUPID and has been replaced with JobGgmlInitParams::createUniqMetadata
// [[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createMetadataContext(std::size_t tensorCount,
//                                                                            std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
//                                                                            bool gradients = false)
// {
//     job::ggml::JobGgmlInitParams initParams {
//         testGgmlContextMetadataSize(tensorCount, graphSize, gradients),
//     };
//     return job::ggml::JobGgmlContext::createUniq(initParams);
// }

// // END test_ggml_context_tensor.cpp


// // THIS IS STUPID  ..... replaced with JobGgmlContext::createUniqHostContext
// [[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createAllocatedHostContext(std::size_t tensorCount,
//                                                                                 std::size_t payloadBytes,
//                                                                                 std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
//                                                                                 bool gradients = false)
// {
//     constexpr std::size_t allocationSafetyBytes = 4096;
//     job::ggml::JobGgmlInitParams initParams {
//         testGgmlContextMetadataSize(tensorCount, graphSize, gradients) + payloadBytes + allocationSafetyBytes,
//         nullptr,
//         false
//     };
//     return job::ggml::JobGgmlContext::createUniq(initParams);
// }

// // THIS IS STUPID   replaced -> JobGgmlTensor::createUniqNamedTensor2d
// [[nodiscard]] inline job::ggml::JobGgmlTensor::UPtr createNamedTensor2d(job::ggml::JobGgmlContext &context,
//                                                                         const std::string &name,
//                                                                         job::ggml::JobGgmlType type = job::ggml::JobGgmlType::F32,
//                                                                         std::int64_t ne0 = 8,
//                                                                         std::int64_t ne1 = 4)
// {
//     auto tensor = context.newTensor2d(type, ne0, ne1);
//     if (tensor)
//         tensor->setName(name);

//     return tensor;
// }

// Find home REMOVEME
[[nodiscard]] inline std::size_t testGgmlStaticOptContextMetadataSize(std::size_t tensorCount, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE) noexcept
{
    constexpr std::size_t optimizerGraphCount = 3;
    constexpr std::size_t metadataSafetyBytes = 64 * 1024;
    return tensorCount * ggml_tensor_overhead() +
           optimizerGraphCount * JobGgmlCGraph::overheadCustom(graphSize, true) +
           metadataSafetyBytes;
}
// Find home REMOVEME
[[nodiscard]] inline JobGgmlContext::UPtr createAllocatedStaticOptContext(std::size_t tensorCount,
                                                                                     std::size_t payloadBytes,
                                                                                     std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE )
{
    constexpr std::size_t allocationSafetyBytes = 64 * 1024;
    JobGgmlInitParams initParams{
        testGgmlStaticOptContextMetadataSize( tensorCount, graphSize ) + payloadBytes + allocationSafetyBytes,
        nullptr,
        false
    };

    return JobGgmlContext::createUniq(initParams);
}
// Find home REMOVEME
[[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createMetadataStaticOptContext(std::size_t tensorCount, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE)
{
    job::ggml::JobGgmlInitParams initParams{
        testGgmlStaticOptContextMetadataSize(tensorCount, graphSize)
    };
    return job::ggml::JobGgmlContext::createUniq(initParams);
}


/// START GGUF

// STAYS
[[nodiscard]] inline std::string transientPath(const std::string &fileName)
{
    return (std::filesystem::temp_directory_path() / fileName).string();
}

// STAYS
[[nodiscard]] inline std::vector<std::byte> readFileBytes(const std::filesystem::path &filePath)
{
    std::ifstream stream{ filePath, std::ios::binary | std::ios::ate};

    if (!stream)
        return {};

    const std::streampos endPosition = stream.tellg();

    if (endPosition <= 0)
        return {};

    const std::size_t size = static_cast<std::size_t>(endPosition);
    std::vector<std::byte> data(size);

    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));

    if (!stream)
        return {};

    return data;
}

// STAYS
void inline populateExampleMetadata(job::ggml::JobGguf &gguf)
{
    gguf.setKeyValue(JobGgufKv{"general.architecture",    std::string{"job-test"} } );
    gguf.setKeyValue(JobGgufKv{"general.name",            std::string{ "Joseph's Odd Builder GGUF Test" } });
    gguf.setKeyValue(JobGgufKv{"job.context_length",      std::uint32_t{4096} });
    gguf.setKeyValue(JobGgufKv{"job.scale",               0.75f });
    gguf.setKeyValue(JobGgufKv{"job.enabled",             true });
    gguf.setKeyValue(JobGgufKv{"job.dimensions",          std::vector<std::uint32_t>{ 64, 128, 256 } });
    gguf.setKeyValue(JobGgufKv{"job.labels",              std::vector<std::string>{ "context", "tensor", "gguf" } });
}

// END  GGUF


// START GGUF CONTEXT
inline void populateContext(JobGgufContext &context)
{
    context.setKeyValue(JobGgufKv{"general.architecture",    std::string{"job-test"}                                 });
    context.setKeyValue(JobGgufKv{"general.name",            std::string{"JobGgufContext Test"}                      });
    context.setKeyValue(JobGgufKv{"job.context_length",      std::uint32_t{4096}                                     });
    context.setKeyValue(JobGgufKv{"job.scale",               0.75f                                                   });
    context.setKeyValue(JobGgufKv{"job.enabled",             true                                                    });
    context.setKeyValue(JobGgufKv{"job.dimensions",          std::vector<std::uint32_t>{64, 128, 256}                });
    context.setKeyValue(JobGgufKv{"job.labels",              std::vector<std::string>{"context", "tensor", "gguf"}   });
}
/// ENDGGUF CONTEXT


// STUPID ....
[[nodiscard]] inline constexpr std::size_t floatByteCount( std::size_t elementCount ) noexcept
{
    return elementCount * sizeof(float);
}

// KEEP
inline void populateLabeledDataset(job::ggml::JobGgmlOptDataset &dataset )
{
    JobGgmlTensor *data = dataset.data();
    JobGgmlTensor *labels = dataset.labels();
    if (!data || !labels || !data->dataPointer() || !labels->dataPointer())
        throw std::runtime_error{ "Failed to access labeled test dataset storage" };

    auto *dataValues  = static_cast<float *>(data->dataPointer());
    auto *labelValues = static_cast<float *>(labels->dataPointer());
    for (std::int64_t index = 0; index < data->elementCount(); ++index)
        dataValues[index] = static_cast<float>(index + 1);

    for (std::int64_t index = 0; index < labels->elementCount(); ++index)
        labelValues[index] = static_cast<float>(100 + index);

}

// KEEP
inline void populateUnlabeledDataset(JobGgmlOptDataset &dataset )
{
    JobGgmlTensor *data = dataset.data();
    if (!data || !data->dataPointer())
        throw std::runtime_error{ "Failed to access unlabeled test dataset storage" };

    auto *dataValues = static_cast<float *>(data->dataPointer());
    for (std::int64_t index = 0; index < data->elementCount(); ++index)
        dataValues[index] = static_cast<float>(index + 1);

}

// THIS is stupid and needs a Update here was no JobGGmlCpu back then
// Again really stupid with the way that it is using SchedulerDevices its really stupid
class CpuSchedulerFixture
{
public:
    CpuSchedulerFixture() :
        m_manager{g_jobGgml->deviceManager()}
    {
        m_manager->resetScheduler();

        m_cpu = m_manager->cpu();
        if (!m_cpu)
            throw std::runtime_error{ "No CPU device was discovered" };

        m_backend = m_cpu->backend();
        if (!m_backend || !m_backend->isValid())
            throw std::runtime_error{ "The discovered CPU device does not expose a valid backend" };

        m_scheduler = m_manager->buildScheduler(m_cpu->uid(),
                                               GGML_DEFAULT_GRAPH_SIZE,
                                               false,
                                               true);

        if (!m_scheduler || !m_scheduler->isValid())
            throw std::runtime_error{ "Failed to construct a CPU-only GGML scheduler" };
    }

    ~CpuSchedulerFixture() = default;

    CpuSchedulerFixture(const CpuSchedulerFixture &) = delete;
    CpuSchedulerFixture &operator=(const CpuSchedulerFixture &) = delete;
    CpuSchedulerFixture(CpuSchedulerFixture &&) = delete;
    CpuSchedulerFixture &operator=(CpuSchedulerFixture &&) = delete;

    [[nodiscard]] JobGgmlDeviceManager *manager() noexcept               { return m_manager; }
    [[nodiscard]] JobGgmlCpu *cpu() noexcept                             { return m_cpu; }
    [[nodiscard]] JobGgmlBackend::Ptr backend() const noexcept           { return m_backend; }
    [[nodiscard]] JobGgmlBackendSched::Ptr scheduler() const noexcept    { return m_scheduler; }

private:
    JobGgmlDeviceManager    *m_manager;
    JobGgmlCpu              *m_cpu{nullptr};
    JobGgmlBackend::Ptr      m_backend;
    JobGgmlBackendSched::Ptr m_scheduler;
};

class CpuExecutionFixture : public CpuSchedulerFixture
{
public:
    explicit CpuExecutionFixture(std::int64_t elementCount = 4096, int tensorCount = 1024)
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(tensorCount);
        if (!initParams)
            throw std::runtime_error{ "CpuExecutionFixture failed to create GGML init params" };

        m_context = JobGgmlContext::createUniq(*initParams);
        if (!m_context || !m_context->isValid())
            throw std::runtime_error{ "CpuExecutionFixture failed to create GGML context" };

        m_input = m_context->newTensor1d(JobGgmlType::F32, elementCount);
        if (!m_input || !m_input->isValid())
            throw std::runtime_error{ "CpuExecutionFixture failed to create input tensor" };

        auto root = JobGgmlTensorOp::createUniq(m_input->tensor(), m_context.get());

        auto scaled = root->scale(2.0f);
        auto activated = scaled->silu();

        m_expression = JobGgmlTensorOpGraph::wrap(std::move(activated));
        if (!m_expression || !m_expression->isValid())
            throw std::runtime_error{ "CpuExecutionFixture failed to create expression" };

        m_graph = m_expression->buildGraph();
        if (!m_graph || !m_graph->isValid())
            throw std::runtime_error{ "CpuExecutionFixture failed to build graph" };

        scheduler()->setTensorBackend(*m_input, *backend());
        scheduler()->setTensorBackend(*m_expression, *backend());
        scheduler()->splitGraph(*m_graph);
        if (!scheduler()->allocateGraph(*m_graph))
            throw std::runtime_error{ "CpuExecutionFixture failed to allocate graph" };
    }

    ~CpuExecutionFixture()
    {
        if (cpu()) {
            cpu()->setAbortCallback(nullptr);
            cpu()->setThreadPool(nullptr);
        }

        manager()->resetScheduler();
    }

    CpuExecutionFixture(const CpuExecutionFixture &) = delete;
    CpuExecutionFixture &operator=(const CpuExecutionFixture &) = delete;
    CpuExecutionFixture(CpuExecutionFixture &&) = delete;
    CpuExecutionFixture &operator=(CpuExecutionFixture &&) = delete;

    [[nodiscard]] JobGgmlStatus compute() { return scheduler()->computeGraph(*m_graph); }
    [[nodiscard]] JobGgmlContext *context() noexcept { return m_context.get(); }
    [[nodiscard]] JobGgmlTensor *input() noexcept { return m_input.get(); }
    [[nodiscard]] JobGgmlTensorOpGraph *expression() noexcept { return m_expression.get(); }
    [[nodiscard]] JobGgmlCGraph *graph() noexcept { return m_graph.get(); }

private:
    JobGgmlContext::UPtr       m_context;
    JobGgmlTensor::UPtr        m_input;
    JobGgmlTensorOpGraph::UPtr m_expression;
    JobGgmlCGraph::UPtr        m_graph;
};

class CpuAdditionGraph
{
public:
    static constexpr std::size_t ElementCount = 4;
    static constexpr std::size_t TensorCount  = 3;
    static constexpr std::size_t ByteCount    = ElementCount * sizeof(float);

    explicit CpuAdditionGraph(CpuSchedulerFixture &schedulerFixture) :
        m_fixture{schedulerFixture},
        m_initParams{JobGgmlInitParams::estCtxCost(TensorCount), nullptr, true},
        m_context{JobGgmlContext::createUniq(m_initParams)}
    {
        if (!m_context || !m_context->isValid())
            throw std::runtime_error{ "Failed to create the test GGML context" };

        m_left  = m_context->newTensor1d(JobGgmlType::F32, static_cast<std::int64_t>(ElementCount));
        m_right = m_context->newTensor1d(JobGgmlType::F32, static_cast<std::int64_t>(ElementCount));
        if (!m_left || !m_right)
            throw std::runtime_error{ "Failed to create test input tensors" };

        auto op = JobGgmlTensorOp::createUniq(m_left->tensor(), m_context.get());
        if (!op || !op->isValid())
            throw std::runtime_error{ "Failed to create the test addition root operation" };

        auto result = op->add(*m_right);
        if (!result || !result->isValid())
            throw std::runtime_error{ "Failed to create the test addition operation" };

        m_result = std::move(result);
        m_graph = m_context->newGraph();

        if (!m_result || !m_graph)
            throw std::runtime_error{ "Failed to create the test result tensor or graph" };

        m_graph->buildForwardExpand(*m_result);

        m_fixture.scheduler()->setTensorBackend(*m_left, *m_fixture.backend());
        m_fixture.scheduler()->setTensorBackend(*m_right, *m_fixture.backend());
        m_fixture.scheduler()->setTensorBackend(*m_result, *m_fixture.backend());
        m_fixture.scheduler()->splitGraph(*m_graph);
        if (!m_fixture.scheduler()->allocateGraph(*m_graph))
            throw std::runtime_error{ "Failed to allocate the test computation graph" };
    }

    ~CpuAdditionGraph() = default;

    CpuAdditionGraph(const CpuAdditionGraph &) = delete;
    CpuAdditionGraph &operator=(const CpuAdditionGraph &) = delete;
    CpuAdditionGraph(CpuAdditionGraph &&) = delete;
    CpuAdditionGraph &operator=(CpuAdditionGraph &&) = delete;

    void uploadInputs(const std::array<float, ElementCount> &leftValues,
                      const std::array<float, ElementCount> &rightValues)
    {
        m_fixture.backend()->setTensorAsync(*m_left,
                                            leftValues.data(),
                                            0,
                                            ByteCount);

        m_fixture.backend()->setTensorAsync(*m_right,
                                            rightValues.data(),
                                            0,
                                            ByteCount);

        m_fixture.backend()->synchronize();
    }

    [[nodiscard]] std::array<float, ElementCount> downloadResult()
    {
        std::array<float, ElementCount> values{};
        m_fixture.backend()->getTensorAsync(*m_result,
                                            values.data(),
                                            0,
                                            ByteCount);

        m_fixture.backend()->synchronize();
        return values;
    }

    [[nodiscard]] JobGgmlTensor *left() noexcept   { return m_left.get(); }
    [[nodiscard]] JobGgmlTensor *right() noexcept  { return m_right.get(); }
    [[nodiscard]] JobGgmlTensor *result() noexcept { return m_result.get(); }
    [[nodiscard]] JobGgmlCGraph *graph() noexcept  { return m_graph.get(); }

private:
    CpuSchedulerFixture     &m_fixture;
    JobGgmlInitParams       m_initParams;
    JobGgmlContext::UPtr    m_context;
    JobGgmlTensor::UPtr     m_left;
    JobGgmlTensor::UPtr     m_right;
    JobGgmlTensor::UPtr     m_result;
    JobGgmlCGraph::UPtr     m_graph;
};


// FIND HOME IF CALLED MORE THAN 1 time
[[nodiscard]] inline JobGgmlOptParams::UPtr createDefaultOptParams(CpuSchedulerFixture &fixture,
                                                                              JobGgmlOptLossType lossType = JobGgmlOptLossType::Mean)
{
    return JobGgmlOptParams::createUniq(fixture.scheduler().get(), lossType);
}


///// START OP
template<typename Function>
void verifyUnaryOperation(JobGgmlTensorOp &source,
                          JobGgmlOp expectedOperation,
                          Function &&function)
{
    auto result = function(source);
    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation() == expectedOperation);
    REQUIRE(operation->sourceCount() == 1);
    REQUIRE(operation->source(0) == source.tensor());
}

template<typename Function>
void verifyUnarySubtype(JobGgmlTensorOp &source,
                        JobGgmlUnaryOp expectedUnaryOperation,
                        Function &&function)
{
    auto result = function(source);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation() == JobGgmlOp::Unary);
    REQUIRE(operation->unaryOperation() == expectedUnaryOperation);
    REQUIRE(operation->sourceCount() == 1);
    REQUIRE(operation->source(0) == source.tensor());
}

template<typename Function>
void verifyGluOperation(JobGgmlTensorOp &source,
                        JobGgmlGluOp expectedGluOperation,
                        Function &&function)
{
    auto result = function(source);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();

    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());

    REQUIRE(operation->operation()      == JobGgmlOp::Glu);
    REQUIRE(operation->gluOperation()   == expectedGluOperation);
    REQUIRE(operation->sourceCount()    == 1);
    REQUIRE(operation->source(0)        == source.tensor());
}

template<typename Function>
void verifyMultiOperation(JobGgmlTensorOp &source,
                          JobGgmlOp expectedOperation,
                          std::initializer_list<const struct ggml_tensor *> expectedSources,
                          Function &&function)
{
    auto result = function(source);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation() == expectedOperation);
    REQUIRE(operation->sourceCount() == expectedSources.size());

    std::size_t index = 0;
    for (const struct ggml_tensor *expectedSource : expectedSources) {
        REQUIRE(operation->source(index) == expectedSource);
        ++index;
    }
}

template<typename Function>
void verifySplitGluOperation(JobGgmlTensorOp &source,
                             JobGgmlTensor &other,
                             JobGgmlGluOp expectedGluOperation,
                             Function &&function)
{
    auto result = function(source, other);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation()      == JobGgmlOp::Glu);
    REQUIRE(operation->gluOperation()   == expectedGluOperation);
    REQUIRE(operation->sourceCount()    == 2);
    REQUIRE(operation->source(0)        == source.tensor());
    REQUIRE(operation->source(1)        == other.tensor());
}

template<typename Function>
void verifyTransformOperation(JobGgmlTensorOp &source,
                              JobGgmlOp expectedOperation,
                              std::initializer_list<const struct ggml_tensor *> expectedSources,
                              Function &&function)
{
    auto result = function(source);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation() == expectedOperation);
    REQUIRE(operation->sourceCount() == expectedSources.size());

    std::size_t index = 0;
    for (const auto *expectedSource : expectedSources)
        REQUIRE(operation->source(index++) == expectedSource);
}

template<typename Function>
void verifySpecialOperation(JobGgmlTensorOp &source,
                            JobGgmlOp expectedOperation,
                            std::initializer_list<const struct ggml_tensor *> expectedSources,
                            Function &&function)
{
    auto result = function(source);

    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());
    REQUIRE(result->context() == source.context());

    auto *operation = result->operation();
    REQUIRE(operation != nullptr);
    REQUIRE(operation->isValid());
    REQUIRE(operation->operation()   == expectedOperation);
    REQUIRE(operation->sourceCount() == expectedSources.size());
    std::size_t index = 0;
    for (const auto *expectedSource : expectedSources)
        REQUIRE(operation->source(index++) == expectedSource);
}

// STUCTS
struct CpuOptDatasetFixture
{
    static constexpr std::int64_t NeDatapoint = 2;
    static constexpr std::int64_t NeLabel     = 1;
    static constexpr std::int64_t Ndata       = 8;
    static constexpr std::int64_t NdataShard  = 4;

    JobGgmlOptDataset dataset {
        JobGgmlType::F32,
        JobGgmlType::F32,
        NeDatapoint,
        NeLabel,
        Ndata,
        NdataShard
    };
};

struct UnaryOpFixture
{
    static constexpr std::int64_t       Ne0 = 8;
    static constexpr std::int64_t       Ne1 = 4;
    JobGgmlContext::UPtr     context;
    JobGgmlTensor::UPtr      tensor;
    JobGgmlTensorOp::UPtr    op;

    UnaryOpFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(256);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create unary operation test context" };

        tensor = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        if (!tensor || !tensor->isValid())
            throw std::runtime_error{ "Failed to create unary operation test tensor" };

        op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
        if (!op || !op->isValid())
            throw std::runtime_error{ "Failed to create unary operation wrapper" };
    }
};


struct MultiOpFixture
{
    static constexpr std::int64_t       Ne0 = 8;
    static constexpr std::int64_t       Ne1 = 4;
    JobGgmlContext::UPtr     context;
    JobGgmlTensor::UPtr      left;
    JobGgmlTensor::UPtr      right;
    JobGgmlTensorOp::UPtr    op;

    MultiOpFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(1024);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create multi-operation test context" };

        left  = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        right = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        if (!left || !left->isValid() || !right || !right->isValid())
            throw std::runtime_error{ "Failed to create multi-operation test tensors" };

        op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
        if (!op || !op->isValid())
            throw std::runtime_error{ "Failed to create multi-operation wrapper" };
    }
};

struct TransformOpFixture
{
    static constexpr std::int64_t       Ne0 = 8;
    static constexpr std::int64_t       Ne1 = 4;
    JobGgmlContext::UPtr                context;
    JobGgmlTensor::UPtr                 tensor;
    JobGgmlTensorOp::UPtr               op;

    TransformOpFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(1024);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create transformation test context" };

        tensor = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        if (!tensor || !tensor->isValid())
            throw std::runtime_error{ "Failed to create transformation test tensor" };

        op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
        if (!op || !op->isValid())
            throw std::runtime_error{ "Failed to create transformation operation wrapper" };
    }
};

struct SpecialOpFixture
{
    static constexpr std::int64_t   Ne0 = 16;
    static constexpr std::int64_t   Ne1 = 8;
    static constexpr std::int64_t   Ne2 = 4;
    static constexpr std::int64_t   Ne3 = 2;
    JobGgmlContext::UPtr            context;
    JobGgmlTensor::UPtr             tensor;
    JobGgmlTensorOp::UPtr           op;

    SpecialOpFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(1024);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create special operation test context" };

        tensor = context->newTensor4d(JobGgmlType::F32, Ne0, Ne1, Ne2, Ne3);
        if (!tensor || !tensor->isValid())
            throw std::runtime_error{ "Failed to create special operation test tensor" };

        op = JobGgmlTensorOp::createUniq(tensor->tensor(), context.get());
        if (!op || !op->isValid())
            throw std::runtime_error{ "Failed to create special operation wrapper" };
    }
};

struct CustomOpFixture
{
    static constexpr std::int64_t       Ne0 = 8;
    static constexpr std::int64_t       Ne1 = 4;
    JobGgmlContext::UPtr     context;
    JobGgmlTensor::UPtr      tensorA;
    JobGgmlTensor::UPtr      tensorB;
    JobGgmlTensor::UPtr      tensorC;
    JobGgmlTensorOp::UPtr    opA;

    CustomOpFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(1024);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create custom operation test context" };

        tensorA = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        tensorB = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        tensorC = context->newTensor2d(JobGgmlType::F32, Ne0, Ne1);
        if (!tensorA || !tensorB || !tensorC)
            throw std::runtime_error{ "Failed to create custom operation test tensors" };

        opA = JobGgmlTensorOp::createUniq(tensorA->tensor(), context.get());
        if (!opA || !opA->isValid())
            throw std::runtime_error{ "Failed to create custom operation wrapper" };
    }
};

struct TensorOpGraphFixture
{
    static constexpr std::size_t    ElementCount = 8;
    static constexpr std::size_t    ByteCount = ElementCount * sizeof(float);
    CpuSchedulerFixture             schedulerFixture;
    JobGgmlContext::UPtr            context;
    JobGgmlTensor::UPtr             input;
    JobGgmlTensor::UPtr             weights;
    JobGgmlTensorOpGraph::UPtr      expression;

    TensorOpGraphFixture()
    {
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(256);
        context = JobGgmlContext::createUniq(*initParams);

        if (!context || !context->isValid())
            throw std::runtime_error{ "Failed to create tensor operation graph context" };

        input   = context->newTensor1d(JobGgmlType::F32, ElementCount);
        weights = context->newTensor1d(JobGgmlType::F32, ElementCount);
        if (!input || !weights)
            throw std::runtime_error{ "Failed to create tensor operation graph inputs" };

        auto inputOp = JobGgmlTensorOp::createUniq(input->tensor(), context.get());
        if (!inputOp || !inputOp->isValid())
            throw std::runtime_error{ "Failed to create tensor operation graph root" };

        expression = JobGgmlTensorOpGraph::wrap(inputOp->mul(*weights));
        if (!expression || !expression->isValid())
            throw std::runtime_error{ "Failed to create tensor operation graph expression" };
    }
};
// END OP STRUCTS




#ifdef JOB_TEST_BENCHMARKS
template<typename DeviceType>
void benchMatMul(DeviceType *device,
                 std::size_t deviceIdx = 0,
                 std::int64_t M = 1024,
                 std::int64_t K = 1024,
                 std::int64_t N = 1024,
                 int iter = 16,
                 std::size_t tensorCount = 1024,
                 JobGgmlBackendSched::Ptr scheduler = nullptr,
                 JobGgmlBackend::Ptr backend = nullptr)
{
    REQUIRE(device != nullptr);
    REQUIRE(scheduler != nullptr);
    REQUIRE(backend != nullptr);

    auto initParams = JobGgmlInitParams::createUniqMetadataFor(tensorCount);
    auto context = JobGgmlContext::createUniq(*initParams);
    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());

    constexpr JobGgmlType PrecisionType = JobGgmlType::F16;

    auto left  = context->newTensor2d(PrecisionType, K, M);
    auto right = context->newTensor2d(PrecisionType, K, N);
    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);
    REQUIRE(left->isValid());
    REQUIRE(right->isValid());

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    auto mulMat = op->mulMat(*right);
    REQUIRE(mulMat != nullptr);
    REQUIRE(mulMat->isValid());

    auto expression = JobGgmlTensorOpGraph::wrap(std::move(mulMat));
    REQUIRE(expression != nullptr);
    REQUIRE(expression->isValid());

    auto graph = expression->buildGraph();
    REQUIRE(graph != nullptr);
    REQUIRE(graph->isValid());

    scheduler->setTensorBackend(*left, *backend);
    scheduler->setTensorBackend(*right, *backend);
    scheduler->setTensorBackend(*expression, *backend);
    scheduler->splitGraph(*graph);

    REQUIRE(scheduler->allocateGraph(*graph));

    const std::size_t leftElements  = static_cast<std::size_t>(M * K);
    const std::size_t rightElements = static_cast<std::size_t>(K * N);

    const ggml_fp16_t value = JobGgmlTypeTraits::fp32ToFp16(0.01f);
    std::vector<ggml_fp16_t> a(leftElements, value);
    std::vector<ggml_fp16_t> b(rightElements, value);

    ggml_backend_tensor_set(left->tensor(),  a.data(), 0, a.size() * sizeof(ggml_fp16_t));
    ggml_backend_tensor_set(right->tensor(), b.data(), 0, b.size() * sizeof(ggml_fp16_t));

    for (int i = 0; i < iter; ++i)
        REQUIRE(scheduler->computeGraph(*graph) == JobGgmlStatus::Success);

    backend->synchronize();

    std::string benchName = "GGML mul_mat F16 ";
    if (M == K && K == N)
        benchName += std::to_string(M) + "^3";
    else
        benchName += std::to_string(M) + "x" + std::to_string(K) + "x" + std::to_string(N);

    benchName += " (batched x" + std::to_string(iter) + ", dev " + std::to_string(deviceIdx) + ")";

    BENCHMARK(benchName.c_str()) {
        for (int i = 0; i < iter; ++i)
            if (scheduler->computeGraph(*graph) != JobGgmlStatus::Success)
                FAIL("Graph execution failed during benchmark iteration");

        backend->synchronize();
    };
}
#endif