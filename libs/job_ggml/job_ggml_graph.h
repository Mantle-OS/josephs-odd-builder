#pragma once
#include <memory>
#include <string>

#include <ggml.h>
#include <ggml-cpp.h>

#include "job_ggml_device_manager.h"

namespace job::ggml {

class JobGgmlGraph {
public:
    using Ptr = std::shared_ptr<JobGgmlGraph>;

    explicit JobGgmlGraph(size_t memBytes, size_t graphSize = kDefaultGraphSize);
    [[nodiscard]] ggml_context *context() noexcept;
    [[nodiscard]] ggml_cgraph  *graph() noexcept;

    // Tensor creation
    [[nodiscard]] ggml_tensor *tensor1d(int64_t ne0, const std::string &name = "");
    [[nodiscard]] ggml_tensor *tensor2d(int64_t ne0, int64_t ne1, const std::string &name = "");
    [[nodiscard]] ggml_tensor *tensor3d(int64_t ne0, int64_t ne1, int64_t ne2, const std::string &name = "");
    [[nodiscard]] ggml_tensor *tensor4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3, const std::string &name = "");

    // Mark tensor as trainable
    void markParam(ggml_tensor *t);

    // Graph building
    void addForward(ggml_tensor *result);
    void addBackward(ggml_tensor **gradAccs = nullptr);

    // Execute on a single device
    void compute(JobGgmlDevice &device);

    // Execute via scheduler (multi-device dispatch)
    void computeWithSched(JobGgmlDeviceManager &manager);

    void reset();

    [[nodiscard]] size_t usedMem() const noexcept;

    JobGgmlGraph(const JobGgmlGraph &) = delete;
    JobGgmlGraph &operator=(const JobGgmlGraph &) = delete;

private:
    static constexpr size_t kDefaultGraphSize = 8192;
    ggml_context_ptr m_ctx;
    ggml_cgraph      *m_graph{nullptr};
    size_t            m_memBytes{0};
};
}

