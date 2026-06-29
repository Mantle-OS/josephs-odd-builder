#pragma once
#include <vector>
#include <string>
#include <unordered_map>

#include<ggml-alloc.h>
#include<ggml-cpp.h>

#include "job_ggml_device.h"

namespace job::ggml {
class JobGgmlWeights {
public:
    using Ptr = std::shared_ptr<JobGgmlWeights>;

    explicit JobGgmlWeights(JobGgmlDevice &device);

    // Load tensors from a GGUF file — mmap's the data region
    bool loadFromGGUF(gguf_context_ptr &ggufCtx);

    [[nodiscard]] ggml_tensor *tensor(const std::string &name) noexcept;
    [[nodiscard]] const ggml_tensor *tensor(const std::string &name) const noexcept;
    [[nodiscard]] size_t tensorCount() const noexcept;
    [[nodiscard]] std::vector<std::string> names() const;
    [[nodiscard]] size_t totalSize() const noexcept;

    JobGgmlWeights(const JobGgmlWeights &) = delete;
    JobGgmlWeights &operator=(const JobGgmlWeights &) = delete;

private:
    JobGgmlDevice                                   &m_device;
    ggml_context_ptr                                m_ctx;          // no_alloc mode
    ggml_backend_buffer_t                           m_buffer;       // mmap'd CPU buffer
    std::unordered_map<std::string, ggml_tensor*>   m_tensors;
};
}