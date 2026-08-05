#pragma once

#include "job_ggml_context.h"
#include <stdexcept>
#include <vector>

#include <job_ggml_backend.h>
#include <job_ggml_backend_sched.h>
#include <job_ggml_device.h>
#include <job_ggml_device_manager.h>

extern job::ggml::JobGgmlDeviceManager *g_deviceManager;

inline job::ggml::JobGgmlDeviceManager &testDeviceManager()
{
    if (!g_deviceManager) {
        throw std::runtime_error{
            "The shared JobGgmlDeviceManager has not been initialized"
        };
    }

    return *g_deviceManager;
}


[[nodiscard]] inline std::size_t testGgmlContextMetadataSize(std::size_t tensorCount, std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE, bool gradients = false) noexcept
{
    constexpr std::size_t metadataSafetyBytes = 4096;
    return tensorCount * ggml_tensor_overhead() +
           job::ggml::JobGgmlCGraph::overheadCustom(graphSize, gradients) +
           metadataSafetyBytes;
}

[[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createMetadataContext(std::size_t tensorCount,
                                                                           std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                                                           bool gradients = false)
{
    job::ggml::JobGgmlInitParams initParams {
        ggml_init_params{
            testGgmlContextMetadataSize(tensorCount, graphSize, gradients),
            nullptr,
            true
        }
    };

    return job::ggml::JobGgmlContext::createUniq(initParams);
}

[[nodiscard]] inline job::ggml::JobGgmlContext::UPtr createAllocatedHostContext(std::size_t tensorCount,
                                                                                std::size_t payloadBytes,
                                                                                std::size_t graphSize = GGML_DEFAULT_GRAPH_SIZE,
                                                                                bool gradients = false)
{
    constexpr std::size_t allocationSafetyBytes = 4096;
    job::ggml::JobGgmlInitParams initParams {
        ggml_init_params{
            testGgmlContextMetadataSize(tensorCount, graphSize, gradients) + payloadBytes + allocationSafetyBytes,
            nullptr,
            false
        }
    };

    return job::ggml::JobGgmlContext::createUniq(initParams);
}
struct CpuSchedulerFixture
{
    job::ggml::JobGgmlDeviceManager &manager;
    job::ggml::JobGgmlDevice        *cpu{nullptr};
    job::ggml::JobGgmlBackend::Ptr   backend;
    job::ggml::JobGgmlBackendSched::Ptr scheduler;

    CpuSchedulerFixture() :
        manager{testDeviceManager()}
    {
        manager.scan();
        manager.resetScheduler();

        cpu = manager.cpuDevice();

        if (!cpu) {
            throw std::runtime_error{
                "No CPU device was discovered"
            };
        }

        backend = cpu->backend();

        if (!backend || !backend->isValid()) {
            throw std::runtime_error{
                "The discovered CPU device does not expose a valid backend"
            };
        }

        scheduler = manager.buildScheduler(
            std::vector<job::ggml::JobGgmlDevice *>{
                cpu
            }
            );

        if (!scheduler || !scheduler->isValid()) {
            throw std::runtime_error{
                "Failed to construct a CPU-only GGML scheduler"
            };
        }
    }
};

struct CpuAdditionGraph
{
    static constexpr std::size_t ElementCount = 4;
    static constexpr std::size_t TensorCount  = 3;
    static constexpr std::size_t ByteCount = ElementCount * sizeof(float);
    explicit CpuAdditionGraph(CpuSchedulerFixture &schedulerFixture) :
        fixture{schedulerFixture},
        initParams{ggml_init_params{testGgmlContextMetadataSize(TensorCount), nullptr, true}},
        context{job::ggml::JobGgmlContext::createUniq(initParams)}
    {
        if (!context || !context->isValid()) {
            throw std::runtime_error{
                "Failed to create the test GGML context"
            };
        }

        left = context->newTensor1d(job::ggml::JobGgmlType::F32, static_cast<std::int64_t>(ElementCount));
        right = context->newTensor1d(job::ggml::JobGgmlType::F32, static_cast<std::int64_t>(ElementCount));

        if (!left || !right) {
            throw std::runtime_error{
                "Failed to create test input tensors"
            };
        }

        ggml_tensor *nativeResult = ggml_add(context->context(), left->tensor(), right->tensor());

        if (!nativeResult) {
            throw std::runtime_error{
                "Failed to create the test addition operation"
            };
        }

        result = job::ggml::JobGgmlTensor::createUniq(nativeResult);

        graph = context->newGraph();

        if (!result || !graph) {
            throw std::runtime_error{
                "Failed to create the test result tensor or graph"
            };
        }

        graph->buildForwardExpand( *result );

        fixture.scheduler->setTensorBackend(*left, *fixture.backend);
        fixture.scheduler->setTensorBackend(*right, *fixture.backend);
        fixture.scheduler->setTensorBackend(*result, *fixture.backend);
        fixture.scheduler->splitGraph(*graph);
        if (!fixture.scheduler->allocateGraph(*graph)) {
            throw std::runtime_error{
                "Failed to allocate the test computation graph"
            };
        }
    }

    void uploadInputs(const std::array<float, ElementCount> &leftValues, const std::array<float, ElementCount> &rightValues)
    {
        fixture.backend->setTensorAsync(*left, leftValues.data(), 0, ByteCount);
        fixture.backend->setTensorAsync(*right, rightValues.data(), 0, ByteCount);
        fixture.backend->synchronize();
    }

    [[nodiscard]] std::array<float, ElementCount> downloadResult()
    {
        std::array<float, ElementCount> values{};
        fixture.backend->getTensorAsync(*result, values.data(), 0, ByteCount);
        fixture.backend->synchronize();

        return values;
    }

    CpuSchedulerFixture &fixture;

    job::ggml::JobGgmlInitParams    initParams;
    job::ggml::JobGgmlContext::UPtr context;

    job::ggml::JobGgmlTensor::UPtr  left;
    job::ggml::JobGgmlTensor::UPtr  right;
    job::ggml::JobGgmlTensor::UPtr  result;

    job::ggml::JobGgmlCGraph::UPtr  graph;
};



