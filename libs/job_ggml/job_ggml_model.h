#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <ggml.h>
#include <ggml-cpp.h>

#include "job_ggml_device.h"
#include "job_ggml_weights.h"

namespace job::ggml {

class JobGgmlModel {
public:
    using Ptr = std::shared_ptr<JobGgmlModel>;
    JobGgmlModel() = default;
    JobGgmlModel(const JobGgmlModel &) = delete;
    JobGgmlModel &operator=(const JobGgmlModel &) = delete;

    [[nodiscard]] bool loadGGUF(const std::string &path, JobGgmlDevice &device);

    [[nodiscard]] const std::string &name() const noexcept;
    [[nodiscard]] const std::string &architecture() const noexcept;
    [[nodiscard]] std::string metadata(const std::string &key) const;

    [[nodiscard]] JobGgmlWeights &weights() noexcept;
    [[nodiscard]] const JobGgmlWeights &weights() const noexcept;
    [[nodiscard]] gguf_context *ggufCtx() noexcept;

private:
    gguf_context_ptr                                m_ggufCtx;
    std::unique_ptr<JobGgmlWeights>                 m_weights;
    std::string                                     m_name;
    std::string                                     m_architecture;
    std::unordered_map<std::string, std::string>    m_metadata;
};

}